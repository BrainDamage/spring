#include <map>
#include <string>

ClientSetup settings;
boost::scoped_ptr<const GameData> gameData;
bool gameOver;
typedef std::map<int,std::string> ActivePlayersMap; // playerid -> playername
typedef ActivePlayersMap::iterator ActivePlayersMapIter;
typedef std::map<int,int> ActiveTeamsMap; // teamid -> numplayers
typedef ActiveTeamsMap::iterator ActiveTeamsMapIter;
typedef std::map<int,int> ActiveAllyTeamsMap; // teamid -> numplayers
typedef ActiveAllyTeamsMap::iterator ActiveAllyTeamsMapIter;
ActivePlayersMap active_players;
ActiveTeamsMap active_teams;
ActiveAllyTeamsMap active_allyteams;
int winner;
int serverframenum;
unsigned int gameStartTime;

bool Update();
bool UpdateClientNet();
void GameDataReceived(boost::shared_ptr<const netcode::RawPacket> packet);
void GameOver();
