#include <map>
#include <string>
#include <list>

ClientSetup settings;
boost::scoped_ptr<const GameData> gameData;
bool gameOver;
bool hasStartedPlaying;
bool isReplay;
typedef std::map<int,std::string> ActivePlayersMap; // playerid -> playername
typedef ActivePlayersMap::iterator ActivePlayersMapIter;
typedef std::map<int,int> ActivePlayersToTeamMap; // playerid -> teamid
typedef ActivePlayersToTeamMap::iterator ActivePlayersToTeamMapIter;
typedef std::map<int,int> ActiveTeamsMap; // teamid -> numplayers
typedef ActiveTeamsMap::iterator ActiveTeamsMapIter;
typedef std::map<int,int> ActiveTeamsToAllyMap; // teamid -> allyid
typedef ActiveTeamsToAllyMap::iterator ActiveTeamsToAllyMapIter;
typedef std::map<int,int> ActiveAllyTeamsMap; // allyteamid -> numplayers
typedef ActiveAllyTeamsMap::iterator ActiveAllyTeamsMapIter;
typedef std::list<CTeam::Statistics> TeamStatistics;
typedef TeamStatistics::iterator TeamStatisticsIter;
typedef std::vector<TeamStatistics> TeamStatisticsVector;
typedef TeamStatisticsVector::iterator TeamStatisticsVectorIter;
std::vector<PlayerBase> playerData;
ActivePlayersMap active_players;
ActivePlayersToTeamMap players_to_teams;
ActiveTeamsMap active_teams;
ActiveTeamsToAllyMap teams_to_ally;
ActiveAllyTeamsMap active_allyteams;
TeamStatisticsVector teams_stats;
int winner;
int serverframenum;
unsigned int gameStartTime;
CDemoReader* demoReader;

bool Update();
bool UpdateClientNet();
void GameDataReceived(boost::shared_ptr<const netcode::RawPacket> packet);
void GameOver();
