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
	std::cout << "If you find any errors, report them to mantis or the forums." << std::endl << std::endl;
	if (argc > 1)
	{
		const std::string script(argv[1]);
		if ( script.rfind(".txt") != std::string::npos )
		{
			isReplay = false;
			std::cout << "Loading script from file: " << script << std::endl;
		}
		else if ( script.rfind(".sdf") != std::string::npos )
		{
			isReplay = true;
			std::cout << "Loading demo file: " << script << std::endl;
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
		std::cout << "Starting client..." << std::endl;
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
		logOutput.Print("Quitting");
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
			logOutput.Print("Player %d died", itor->first );
		}
	}
	players_to_teams = tempcopy;
	teams_to_ally.erase(team);
	active_teams.erase(team);
	logOutput.Print("team %d died", team );
	active_allyteams[ally] = active_allyteams[ally] - 1;
	if ( active_allyteams.count(ally) == 0 )
	{
		logOutput.Print("ally %d died", ally );
		active_allyteams.erase(ally);
	}
}

bool UpdateClientNet()
{
	if (!isReplay)
	{
		if (!net->Active())
		{
			logOutput.Print("Server not reachable");
			return false;
		}
	}
	else
	{
		if (demoReader->ReachedEnd())
		{
			logOutput.Print("End of demo reached");
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
				logOutput.Print("User number %i (team %i, allyteam %i)", gu->myPlayerNum, gu->myTeam, gu->myAllyTeam);

				if (!isReplay)
				{
					// send myPlayerName to let the server know you finished loading
					net->Send(CBaseNetProtocol::Get().SendPlayerName(gu->myPlayerNum, settings.myPlayerName));
				}

				break;
			}
			case NETMSG_KEYFRAME:
			{
				serverframenum = *(int*)(inbuf+1);
				gu->modGameTime = serverframenum/(float)GAME_SPEED;
				if (!isReplay)
				{
					net->Send(CBaseNetProtocol::Get().SendKeyFrame(serverframenum));
				}
				break;
			}
			case NETMSG_NEWFRAME:
			{
				serverframenum++;
				//logOutput.Print("Frame %d", serverframenum);
				gu->modGameTime = serverframenum/(float)GAME_SPEED;
				if (!isReplay)
				{
					net->Send(CBaseNetProtocol::Get().SendSyncResponse(serverframenum, 0));
				}
				break;
			}
			case NETMSG_QUIT:
			{
				logOutput.Print("Server shutdown");
				GameOver();
				return false;
				break;
			}
			case NETMSG_GAMEID:
			{
				const unsigned char* p = &inbuf[1];
				if (!isReplay)
				{
					CDemoRecorder* record = net->GetDemoRecorder();
					if (record != NULL)
					{
						record->SetGameID(p);
					}
				}
				logOutput.Print(
				  "GameID: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				  p[ 0], p[ 1], p[ 2], p[ 3], p[ 4], p[ 5], p[ 6], p[ 7],
				  p[ 8], p[ 9], p[10], p[11], p[12], p[13], p[14], p[15]);
				break;
			}

			case NETMSG_PLAYERLEFT:
			{
				int player = inbuf[1];
				if (!active_players.count(player))
				{
					logOutput.Print("Got invalid player num (%i) in NETMSG_PLAYERLEFT", player);
					break;
				}
				const char *type = gameSetup->playerStartingData[player].GetType();
				std::string playername = active_players[player].c_str();
				switch (inbuf[2])
				{
					case 1:
						logOutput.Print("%s %s left", type, playername.c_str());
						break;
					case 2:
						logOutput.Print("%s %s has been kicked", type, playername.c_str());
						break;
					case 0:
						logOutput.Print("%s %s dropped (connection lost)", type, playername.c_str());
						break;
					default:
						logOutput.Print("%s %s left the game (reason unknown: %i)", type, playername.c_str(), inbuf[2]);
				}
				active_players.erase(player);
				break;
			}
			case NETMSG_PLAYERNAME:
			{
				int player = inbuf[2];
				active_players[player] = (char*)(&inbuf[3]);
				logOutput.Print("Player %s connected as id %d", active_players[player].c_str(), player );
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
								logOutput.Print("team %d died", fromTeam );
								active_teams.erase(fromTeam);
								teams_to_ally.erase(fromTeam);
								active_allyteams[fromAlly] = active_allyteams[fromAlly] - 1;
								if ( active_allyteams.count(fromAlly) == 0 )
								{
									logOutput.Print("ally %d died", fromAlly );
									active_allyteams.erase(fromAlly);
								}
							}
							logOutput.Print("Player %d changed team %d ally %d to team %d ally %d", player, fromTeam, fromAlly, newTeam, newAlly );
						}
						else
						{
							logOutput.Print("Player %d joined team %d ally %d", player, newTeam, newAlly );
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
					logOutput.Print("Game started");
					hasStartedPlaying = true;
				}
				else logOutput.Print("Starting game in %d", timeToStart);
				break;
			}

			case NETMSG_GAMEOVER:
			{
				logOutput.Print("Game over");
				for ( ActivePlayersToTeamMapIter itor = players_to_teams.begin(); itor != players_to_teams.end(); itor++  ) logOutput.Print( "player %d team %d", itor->first, itor->second );
				for ( ActiveTeamsToAllyMapIter itor = teams_to_ally.begin(); itor != teams_to_ally.end(); itor++  ) logOutput.Print( "team %d ally %d", itor->first, itor->second );
				GameOver();
			}

			case NETMSG_PLAYERSTAT:
			{
				int player=inbuf[1];
				if(!active_players.count(player)){
					logOutput.Print("Got invalid player num %i in playerstat msg",player);
					break;
				}
				PlayerStatistics playerstats = *(CPlayer::Statistics*)&inbuf[2];
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
				int team=inbuf[1];
				int frameCount = inbuf[2];
				if( team > gameSetup->teamStartingData.size() ){
					logOutput.Print("Got invalid team num %i in teamstat msg",team);
					break;
				}
				TeamStatistics stathistory = teams_stats[team];
				if ( frameCount < stathistory.size() )
				{
					logOutput.Print("Recieved duplicated stat frame %d for team %d",frameCount, team);
					break;
				}
				CTeam::Statistics statframe = *(CTeam::Statistics*)&inbuf[3];
				stathistory.push_back(statframe);
				teams_stats[team] = stathistory;
				break;
			}

			/*
			default:
			{
				logOutput.Print("Unknown net-msg recieved : %i", int(packet->data[0]));
				break;
			}
			*/
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
	if (gameOver) return;
	if (net && net->GetDemoRecorder())
	{
		CDemoRecorder* record = net->GetDemoRecorder();
		int gamelenght = serverframenum / GAME_SPEED;
		int wallclocklenght = (SDL_GetTicks()-gameStartTime)/1000;
		logOutput.Print("Game lenght: %d Wall clock lenght: %d", gamelenght, wallclocklenght );
		record->SetTime( gamelenght, wallclocklenght);
		record->InitializeStats(active_players.size(), teams_stats.size(), winner);
		for ( size_t team = 0; team < teams_stats.size(); team++ )
		{
			record->SetTeamStats( team, teams_stats[team] );
		}
	}
	gameOver = true;
}

