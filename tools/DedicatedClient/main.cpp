#include "System/Net/RawPacket.h"
#include "System/NetProtocol.h"
#include "System/DemoRecorder.h"
#include "System/TdfParser.h"
#include "System/LogOutput.h"
#include "System/ConfigHandler.h"
#include "System/Exceptions.h"
#include "Game/GameVersion.h"
#include "System/GlobalUnsynced.h"

#include "main.h"

#include <string>
#include <iostream>
#include <fstream>
#include <SDL.h>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
#ifdef _WIN32
	try
	{
#endif
	SDL_Init(SDL_INIT_TIMER);
	std::cout << "If you find any errors, report them to mantis or the forums." << std::endl << std::endl;
	if (argc > 1)
	{
		const std::string script(argv[1]);
		std::cout << "Loading script from file: " << script << std::endl;

		std::ifstream fh(argv[1]);
		if (!fh.is_open())
			throw content_error("Setupscript doesn't exists in given location: "+script);

		std::string buf;
    while (!fh.eof() )
    {
    	std::string line;
      std::getline(fh,line);
      buf += line + "\n";
    }
		fh.close();
		settings.Init(buf);

		if ( settings.isHost ) // won't work with host mode
		{
			std::cout << "Host mode enabled in Setupscript, quitting." << std::endl;
			return 1;
		}

		std::cout << "Starting client..." << std::endl;
		// Create the client
		gu = new CGlobalUnsyncedStuff();
		net = new CNetProtocol();
		gameOver = false;
		winner = -1;
		serverframenum = 0;
		net->InitClient(settings.hostip.c_str(), settings.hostport, settings.sourceport, settings.myPlayerName, settings.myPasswd, SpringVersion::GetFull());
		gameStartTime = SDL_GetTicks();

		boost::thread thread(boost::bind<void, CNetProtocol, CNetProtocol*>(&CNetProtocol::UpdateLoop, net));

		while( Update() ) // don't quit as long as connection is active
#ifdef _WIN32
			Sleep(1000);
#else
			sleep(1);	// if so, wait 1  second
#endif
	}
	else
	{
		std::cout << "usage: spring-dedicated-client <full_path_to_script>" << std::endl;
	}

#ifdef _WIN32
	}
	catch (const std::exception& err)
	{
		std::cout << "Exception raised: " << err.what() << std::endl;
		return 1;
	}
#endif
	return 0;
}

bool Update()
{
	net->Update();
	return UpdateClientNet();
}

bool UpdateClientNet()
{
	if (!net->Active())
	{
		logOutput.Print("Server not reachable");
		return false;
	}

	boost::shared_ptr<const netcode::RawPacket> packet;
	while ((packet = net->GetData()))
	{
		const unsigned char* inbuf = packet->data;
		switch (inbuf[0])
		{
			case NETMSG_GAMEDATA: // server first sends this to let us know about teams, allyteams etc.
			{
				GameDataReceived(packet);
				break;
			}
			case NETMSG_SETPLAYERNUM: // this is sent afterwards to let us know which playernum we have
			{
				gu->SetMyPlayer(inbuf[1]);
				logOutput.Print("User number %i (team %i, allyteam %i)", gu->myPlayerNum, gu->myTeam, gu->myAllyTeam);

				// send myPlayerName to let the server know you finished loading
				net->Send(CBaseNetProtocol::Get().SendPlayerName(gu->myPlayerNum, settings.myPlayerName));

				break;
			}
			case NETMSG_KEYFRAME:
			{
				serverframenum = *(int*)(inbuf+1);
				net->Send(CBaseNetProtocol::Get().SendKeyFrame(serverframenum));
				break;
			}
			case NETMSG_NEWFRAME:
			{
				serverframenum++;
				//net->Send(CBaseNetProtocol::Get().SendSyncResponse(serverframenum, 0));
				break;
			}
			case NETMSG_QUIT:
			{
				logOutput.Print("Server shutdown");
				GameOver();
				logOutput.Print("Quitting");
				return false;
				break;
			}
			case NETMSG_GAMEID:
			{
				const unsigned char* p = &inbuf[1];
				CDemoRecorder* record = net->GetDemoRecorder();
				if (record != NULL) {
					record->SetGameID(p);
				}
				logOutput.Print(
				  "GameID: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				  p[ 0], p[ 1], p[ 2], p[ 3], p[ 4], p[ 5], p[ 6], p[ 7],
				  p[ 8], p[ 9], p[10], p[11], p[12], p[13], p[14], p[15]);
				break;
			}
			case NETMSG_PLAYERSTAT:
			{
				int player=inbuf[1];
				if(player >= active_players.size() || player<0){
					logOutput.Print("Got invalid player num %i in playerstat msg",player);
					break;
				}
				PlayerStatistics playerstats = *(CPlayer::Statistics*)&inbuf[2];
				if (gameOver) {
					CDemoRecorder* record = net->GetDemoRecorder();
					if (record != NULL) {
						record->SetPlayerStats(player, playerstats);
					}
				}
				break;
			}
			default:
			{
				logOutput.Print("Unknown net-msg recieved : %i", int(packet->data[0]));
				break;
			}
		}
	}
	return true;
}


void GameDataReceived(boost::shared_ptr<const netcode::RawPacket> packet)
{
	gameData.reset(new GameData(packet));

	CGameSetup* temp = new CGameSetup();

	if (temp->Init(gameData->GetSetup()))
	{
		gameSetup = const_cast<const CGameSetup*>(temp);
		// gs->LoadFromSetup(gameSetup); TODO: load just team infos
		//CPlayer::UpdateControlledTeams();
	}
	else
	{
		throw content_error("Server sent us incorrect script");
	}

	if (net && net->GetDemoRecorder())
	{
		net->GetDemoRecorder()->SetName(gameSetup->mapName);
		LogObject() << "Recording demo " << net->GetDemoRecorder()->GetName() << "\n";
	}
}


void GameOver()
{
	if (net && net->GetDemoRecorder())
	{
		CDemoRecorder* record = net->GetDemoRecorder();
		int numteams = active_teams.size();
		if (gameSetup->useLuaGaia) numteams -= 1;
		record->SetTime(serverframenum / 30, (SDL_GetTicks()-gameStartTime)/1000);
		record->InitializeStats(active_players.size(), numteams, winner);
		/*
		for (size_t i = 0; i < ais.size(); ++i)
		{
			demoRecorder->SetSkirmishAIStats(i, ais[i].lastStats);
		}
		for (int i = 0; i < numTeams; ++i)
		{
			record->SetTeamStats(i, teamHandler->Team(i)->statHistory);
		}
		*/
	}
	gameOver = true;
}

