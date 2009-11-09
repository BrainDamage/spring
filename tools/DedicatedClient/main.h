#include "Game/GameSetup.h"
#include "Game/ClientSetup.h"
#include "Game/GameData.h"
#include "Game/PlayerHandler.h"
#include "System/NetProtocol.h"

#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include <set>
#include <map>
#include <string>

ClientSetup settings;
boost::scoped_ptr<const GameData> gameData;
bool gameOver;
typedef std::map<int,std::string> ActivePlayersMap;
typedef ActivePlayersMap::iterator ActivePlayersMapIter;
ActivePlayersMap active_players;
std::vector<int> active_teams;
std::vector<int> active_allyteams;
int winner;
int serverframenum;
unsigned int gameStartTime;

bool Update();
bool UpdateClientNet();
void GameDataReceived(boost::shared_ptr<const netcode::RawPacket> packet);
void GameOver();
