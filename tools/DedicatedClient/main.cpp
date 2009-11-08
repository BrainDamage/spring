#include "System/Net/RawPacket.h"
#include "System/NetProtocol.h"
#include "System/DemoRecorder.h"
#include "System/TdfParser.h"
#include "System/LogOutput.h"
#include "System/ConfigHandler.h"
#include "System/Exceptions.h"
#include "Game/GameVersion.h"

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
		net = new CNetProtocol();
		net->InitClient(settings.hostip.c_str(), settings.hostport, settings.sourceport, settings.myPlayerName, settings.myPasswd, SpringVersion::GetFull());

		boost::thread thread(boost::bind<void, CNetProtocol, CNetProtocol*>(&CNetProtocol::UpdateLoop, net));

		timer = SDL_GetTicks();

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
	UpdateClientNet();

	return net->Active();
}

void UpdateClientNet()
{
	if (!net->Active())
	{
		logOutput.Print("Server not reachable");
		return;
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
				SetMyPlayer(packet->data[1]);
				logOutput.Print("User number %i (team %i, allyteam %i)", myPlayerNum, myTeam, myAllyTeam);

				// send myPlayerName to let the server know you finished loading
				net->Send(CBaseNetProtocol::Get().SendPlayerName(myPlayerNum, settings.myPlayerName));

				return;
			}
			default:
			{
				logOutput.Print("Unknown net-msg recieved : %i", int(packet->data[0]));
				break;
			}
		}
	}
}

void SetMyPlayer(const int mynumber)
{
	myPlayerNum = mynumber;
	if (gameSetup && gameSetup->playerStartingData.size() > mynumber)
	{
		myTeam = gameSetup->playerStartingData[myPlayerNum].team;
		myAllyTeam = gameSetup->teamStartingData[myTeam].teamAllyteam;

		spectating = gameSetup->playerStartingData[myPlayerNum].spectator;
		spectatingFullView   = gameSetup->playerStartingData[myPlayerNum].spectator;

		assert(myPlayerNum >= 0
				&& gameSetup->playerStartingData.size() >= static_cast<size_t>(myPlayerNum)
				&& myTeam >= 0
				&& gameSetup->teamStartingData.size() >= myTeam);
	}
}

void GameDataReceived(boost::shared_ptr<const netcode::RawPacket> packet)
{
	gameData.reset(new GameData(packet));

	CGameSetup* temp = new CGameSetup();

	if (temp->Init(gameData->GetSetup()))
	{
		if (settings.isHost)
		{
			const std::string& setupTextStr = gameData->GetSetup();
			std::fstream setupTextFile("_script.txt", std::ios::out);

			setupTextFile.write(setupTextStr.c_str(), setupTextStr.size());
			setupTextFile.close();
		}
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

/*
void GameOver()
{

	if (net && net->GetDemoRecorder())
	{
		net->GetDemoRecorder()->SetTime(serverframenum / 30, spring_tomsecs(spring_gettime()-serverStartTime)/1000);
		net->GetDemoRecorder()->InitializeStats(players.size(), numTeams, winner);
		for ()
		{

			net->GetDemoRecorder()->SetPlayerStats(i, playerHandler->players[i].lastStats);
		}
		for (size_t i = 0; i < ais.size(); ++i)
		{
			demoRecorder->SetSkirmishAIStats(i, ais[i].lastStats);
		}
		for (int i = 0; i < numTeams; ++i)
		{
			record->SetTeamStats(i, teamHandler->Team(i)->statHistory);
		}
	}
}
*/
