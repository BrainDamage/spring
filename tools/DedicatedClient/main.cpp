#include "System/Net/RawPacket.h"
#include "System/NetProtocol.h"
#include "System/DemoRecorder.h"
#include "System/DemoReader.h"
#include "System/TdfParser.h"
#include "System/LogOutput.h"
#include "System/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/GlobalUnsynced.h"

#include "Game/GameVersion.h"
#include "Game/GameSetup.h"
#include "Game/ClientSetup.h"
#include "Game/GameData.h"
#include "Game/PlayerStatistics.h"
#include "Game/Server/MsgStrings.h"

#include "Sim/Misc/GlobalConstants.h"

#include "main.h"

#include <string>
#include <iostream>
#include <fstream>
#include <SDL.h>

#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

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
	if (argc > 1)
	{
		const std::string script(argv[1]);
		if ( script.rfind(".txt") != std::string::npos )
		{
			isReplay = false;
		}
		else if ( script.rfind(".sdf") != std::string::npos )
		{
			isReplay = true;
		}
		else
		{
			 std::cout << "Invalid command line input: " << script << std::endl;
			 return 1;
		}

		if (!isReplay)
		{
			std::ifstream fh(argv[1]);
			if (!fh.is_open())
			{
				 std::cout << "Setupscript doesn't exists in given location: " << script << std::endl;
				 return 1;
			}

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
		}
		// Create the client
		gu = new CGlobalUnsyncedStuff();
		gameOver = false;
		winner = -1;
		hasStartedPlaying = false;
		serverframenum = 0;
		net = 0;
		demoReader = 0;
		if (!isReplay)
		{
			net = new CNetProtocol();
			net->InitClient(settings.hostip.c_str(), settings.hostport, settings.sourceport, settings.myPlayerName, settings.myPasswd, SpringVersion::GetFull());

			boost::thread thread(boost::bind<void, CNetProtocol, CNetProtocol*>(&CNetProtocol::UpdateLoop, net));
		}
		else
		{
			demoReader = new CDemoReader( script, 0 );

			CGameSetup* temp = new CGameSetup();
			if (temp->Init(demoReader->GetSetupScript()))
			{
				gameSetup = const_cast<const CGameSetup*>(temp);
				// gs->LoadFromSetup(gameSetup); TODO: load just team infos
				//CPlayer::UpdateControlledTeams();
			}
			else
			{
				throw content_error("Demo contains incorrect script");
			}
		}
		gameStartTime = SDL_GetTicks();

		while( Update() ) // don't quit as long as connection is active
#ifdef _WIN32
			Sleep(1000);
#else
			sleep(1);	// if so, wait 1  second
#endif
	}
	else
	{
		std::cout << "usage:\nspring-dedicated-client <full_path_to_script>\nspring-dedicated-client <full_path_to_demo_file>" << std::endl;
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
	if (!isReplay) net->Update();
	return UpdateClientNet();
}

boost::shared_ptr<const netcode::RawPacket> GetData()
{
	if (!isReplay) return net->GetData();
	else
	{
		boost::shared_ptr<const netcode::RawPacket> ret;
		ret.reset(demoReader->GetData(demoReader->GetNextReadTime()));
		return ret;
	}
}

void TeamDied( int team )
{
	if ( teams_to_ally.count(team) == 0 ) teams_to_ally[team] = gameSetup->teamStartingData[team].teamAllyteam;
	const int ally = teams_to_ally[team];
	// make all players in the team be spectators
	ActivePlayersToTeamMap tempcopy = players_to_teams;
	for ( ActivePlayersToTeamMapIter itor = players_to_teams.begin(); itor != players_to_teams.end(); itor++ )
	{
		if ( itor->second == team )
		{
			tempcopy.erase(itor->first);
		}
	}
	players_to_teams = tempcopy;
	teams_to_ally.erase(team);
	active_teams.erase(team);
	logOutput.Print("TEAMDIED %d %d", serverframenum,team );
	active_allyteams[ally] = active_allyteams[ally] - 1;
	if ( active_allyteams.count(ally) == 0 )
	{
		active_allyteams.erase(ally);
	}
}

bool UpdateClientNet()
{
	if (!isReplay)
	{
		if (!net->Active())
		{
			return false;
		}
	}
	else
	{
		if (demoReader->ReachedEnd())
		{
			return false;
		}
	}

	boost::shared_ptr<const netcode::RawPacket> packet;
	while ((packet = GetData()))
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

				if (!isReplay)
				{
					// send myPlayerName to let the server know you finished loading
					net->Send(CBaseNetProtocol::Get().SendPlayerName(gu->myPlayerNum, settings.myPlayerName));
					// register for interesting messages
					net->Send(CBaseNetProtocol::Get().SendRegisterNetMsg(gu->myPlayerNum, NETMSG_TEAMSTAT));
					net->Send(CBaseNetProtocol::Get().SendRegisterNetMsg(gu->myPlayerNum, NETMSG_TEAM));
				}

				break;
			}
			case NETMSG_KEYFRAME:
			{
				serverframenum = *(int*)(inbuf+1);
				modGameTime = serverframenum/(float)GAME_SPEED;
				if (!isReplay)
				{
					net->Send(CBaseNetProtocol::Get().SendKeyFrame(serverframenum));
				}
				break;
			}
			case NETMSG_NEWFRAME:
			{
				serverframenum++;
				modGameTime = serverframenum/(float)GAME_SPEED;
				if (!isReplay)
				{
					net->Send(CBaseNetProtocol::Get().SendSyncResponse(serverframenum, 0));
				}
				break;
			}
			case NETMSG_QUIT:
			{
				GameOver();
				return false;
				break;
			}
			case NETMSG_GAMEID:
			{
				const unsigned char * gameID= (unsigned char*)&inbuf[1];
				if (!isReplay)
				{
					CDemoRecorder* record = net->GetDemoRecorder();
					if (record != NULL)
					{
						record->SetGameID(gameID);
					}
				}
				logOutput.Print(
				  "GAMEID %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				  gameID[ 0], gameID[ 1], gameID[ 2], gameID[ 3], gameID[ 4], gameID[ 5], gameID[ 6], gameID[ 7],
				  gameID[ 8], gameID[ 9], gameID[10], gameID[11], gameID[12], gameID[13], gameID[14], gameID[15]);
				break;
			}

			case NETMSG_PLAYERLEFT:
			{
				int player = inbuf[1];
				if (!active_players.count(player))
				{
					break;
				}
				const char *type = gameSetup->playerStartingData[player].GetType();
				std::string playername = active_players[player].c_str();
				logOutput.Print("DISCONNECT %d %s %s", serverframenum, type, playername.c_str());
				active_players.erase(player);
				break;
			}
			case NETMSG_PLAYERNAME:
			{
				int player = inbuf[2];
				active_players[player] = (char*)(&inbuf[3]);
				logOutput.Print("CONNECTED %s", active_players[player].c_str());
				break;
			}
			case NETMSG_SYSTEMMSG:
			{
				std::string text=(char*)(&inbuf[4]);
				int frame = -1;
				char playernames[200];
				int hash = -1;
				if ( sscanf( text.c_str(), playernames, frame, hash ) == 3 )
				{
					logOutput.Print("DESYNC %d %x %s", frame, hash, playernames);
				}
				break;
			}
			case NETMSG_TEAM:
			{
				int player = inbuf[1];
				const unsigned char action = inbuf[2];
				if ( players_to_teams.count(player) == 0 ) players_to_teams[player] = gameSetup->playerStartingData[player].team;
				const int fromTeam = players_to_teams[player];
				if ( teams_to_ally.count(fromTeam) == 0 ) teams_to_ally[fromTeam] = gameSetup->teamStartingData[fromTeam].teamAllyteam;
				const int fromAlly = teams_to_ally[fromTeam];
				switch (action)
				{
					case TEAMMSG_GIVEAWAY:
					{
						TeamDied( fromTeam );
						break;
					}
					case TEAMMSG_RESIGN:
					{
						TeamDied( fromTeam );
						break;
					}
					case TEAMMSG_JOIN_TEAM:
					{
						const int newTeam = int(inbuf[3]);
						if ( teams_to_ally.count(newTeam) == 0 ) teams_to_ally[newTeam] = gameSetup->teamStartingData[newTeam].teamAllyteam;
						const int newAlly = teams_to_ally[newTeam];
						active_teams[newTeam] = active_teams[newTeam] + 1;
						if ( fromTeam != newTeam ) // delete player from old team
						{
							active_teams[fromTeam] = active_teams[fromTeam] - 1;
							if ( active_teams[fromTeam] == 0 ) // remove team from allyteam if empty
							{
								active_teams.erase(fromTeam);
								teams_to_ally.erase(fromTeam);
								active_allyteams[fromAlly] = active_allyteams[fromAlly] - 1;
								if ( active_allyteams.count(fromAlly) == 0 )
								{
									active_allyteams.erase(fromAlly);
								}
							}
						}
						else
						{
						}
						players_to_teams[player] = newTeam;
						if ( teams_to_ally[newTeam] != newAlly )
						{
							active_allyteams[newAlly] = active_allyteams[newAlly] + 1;
						}
						teams_to_ally[newTeam] = newAlly;
						break;
					}
					case TEAMMSG_TEAM_DIED:
					{
						const unsigned team = inbuf[3];
						TeamDied( team );
						break;
					}
				}
			}

			case NETMSG_STARTPLAYING:
			{
				unsigned timeToStart = *(unsigned*)(inbuf+1);
				if (timeToStart < 1)
				{
					logOutput.Print("GAMESTART");
					hasStartedPlaying = true;
				}
				break;
			}

			case NETMSG_GAMEOVER:
			{
				logOutput.Print("GAMEOVER %d", serverframenum);
				logOutput.Print("ENDGAME");
				GameOver();
			}

			case NETMSG_PLAYERSTAT:
			{
				int player=inbuf[1];
				if(!active_players.count(player)){
					break;
				}
				PlayerStatistics playerstats = *(PlayerStatistics*)&inbuf[2];
				if (gameOver && !isReplay)
				{
					CDemoRecorder* record = net->GetDemoRecorder();
					if (record != NULL)
					{
						record->SetPlayerStats(player, playerstats);
					}
				}
				break;
			}

			case NETMSG_TEAMSTAT:
			{
				const unsigned char teamNum = *((unsigned char*)&inbuf[1]);
				const unsigned int statFrameNum = *((unsigned int*)&inbuf[2]);
				if( teamNum > gameSetup->teamStartingData.size() ){
					break;
				}
				TeamStatisticsList& stathistory = teams_stats[teamNum];
				if ( statFrameNum < stathistory.size() )
				{
					break;
				}
				TeamStatistics statframe = *(TeamStatistics*)&inbuf[6];
				stathistory.push_back(statframe);
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
		playerData = gameSetup->playerStartingData; // copy contents
		teams_stats.resize( gameSetup->teamStartingData.size() ); // allocate speace in team stats for all teams in script
				std::vector<SkirmishAIData> skirmishAI = gameSetup->GetSkirmishAIs();
				logOutput.Print("BEGINSETUP");
					logOutput.Print("BEGINTEAMS");
						for ( std::vector<PlayerBase>::iterator itor = playerData.begin(); itor != playerData.end(); itor++ )
						{
							if ( itor->spectator ) continue;
							logOutput.Print("%s=%d",itor->name.c_str(),itor->team);
						}
						for ( std::vector<SkirmishAIData>::iterator itor = skirmishAI.begin(); itor != skirmishAI.end(); itor++ )
						{
							logOutput.Print("%s=%d",itor->name.c_str(),itor->team);
						}
					logOutput.Print("ENDTEAMS");
					logOutput.Print("BEGINAIS");
						for ( std::vector<SkirmishAIData>::iterator itor = skirmishAI.begin(); itor != skirmishAI.end(); itor++ )
						{
							logOutput.Print("%s=%s %s",itor->name.c_str(),itor->shortName.c_str(),itor->version.c_str());
						}
					logOutput.Print("ENDAIS");
						std::vector<TeamBase> teamStartingData = gameSetup->teamStartingData;
						logOutput.Print("BEGINALLYTEAMS");
						int teamCounter = 0;
						for ( std::vector<TeamBase>::iterator itor = teamStartingData.begin(); itor != teamStartingData.end(); itor++ )
						{
							logOutput.Print("%d=%d",teamCounter,itor->teamAllyteam);
							teamCounter++;
						}
					logOutput.Print("ENDALLYTEAMS");
					logOutput.Print("BEGINOPTIONS");
						std::map<std::string, std::string> mapOptions = gameSetup->mapOptions;
						std::map<std::string, std::string> modOptions = gameSetup->modOptions;
						for ( std::map<std::string, std::string>::iterator itor = mapOptions.begin(); itor != mapOptions.end(); itor++ )
						{
							logOutput.Print("mapoptions/%s=%s",itor->first.c_str(),itor->second.c_str());
						}
						for ( std::map<std::string, std::string>::iterator itor = modOptions.begin(); itor != modOptions.end(); itor++ )
						{
							logOutput.Print("modoptions/%s=%s",itor->first.c_str(),itor->second.c_str());
						}
					logOutput.Print("ENDOPTIONS");
					logOutput.Print("BEGINRESTRICTIONS");
						std::map<std::string, int> restrictedUnits = gameSetup->restrictedUnits;
						for ( std::map<std::string, int>::iterator itor = restrictedUnits.begin(); itor != restrictedUnits.end(); itor++ )
						{
							logOutput.Print("%s=%d",itor->first.c_str(),itor->second);
						}
					logOutput.Print("ENDRESTRICTIONS");
				logOutput.Print("ENDSETUP");
				logOutput.Print("BEGINGAME");
	}
	else
	{
		throw content_error("Server sent us incorrect script");
	}

	if (net && net->GetDemoRecorder())
	{
		net->GetDemoRecorder()->SetName(gameSetup->mapName, gameSetup->modName);
	}
}


void GameOver()
{
	if (gameOver) return;
	if (net && net->GetDemoRecorder())
	{
		CDemoRecorder* record = net->GetDemoRecorder();
		int gamelenght = serverframenum / GAME_SPEED;
		int wallclocklenght = (SDL_GetTicks()-gameStartTime)/1000;
		record->SetTime( gamelenght, wallclocklenght);
		record->InitializeStats(active_players.size(), teams_stats.size(), winner);
		for ( size_t team = 0; team < teams_stats.size(); team++ )
		{
			record->SetTeamStats( team, teams_stats[team] );
		}
	}
	gameOver = true;
}
