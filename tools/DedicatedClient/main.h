#include <map>
#include <string>

ClientSetup settings;
boost::scoped_ptr<const GameData> gameData;
bool gameOver;
bool hasStartedPlaying;
bool isReplay;
typedef std::map<int,std::string> ActivePlayersMap; // playerid -> playername
typedef ActivePlayersMap::iterator ActivePlayersMapIter;
typedef std::map<int,int> ActiveTeamsMap; // teamid -> numplayers
typedef ActiveTeamsMap::iterator ActiveTeamsMapIter;
typedef std::map<int,int> ActiveAllyTeamsMap; // allyteamid -> numplayers
typedef ActiveAllyTeamsMap::iterator ActiveAllyTeamsMapIter;
std::vector<PlayerBase> playerData;
ActivePlayersMap active_players;
ActiveTeamsMap active_teams;
ActiveAllyTeamsMap active_allyteams;
IsSpectatorMap is_spectator;
int winner;
int serverframenum;
unsigned int gameStartTime;
CDemoReader* demoReader;

bool Update();
bool UpdateClientNet();
void GameDataReceived(boost::shared_ptr<const netcode::RawPacket> packet);
void GameOver();
