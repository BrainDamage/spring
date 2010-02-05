#include "AdvGenerator.h"
#include "streflop.h"

CAdvGenerator::CAdvGenerator(CMapGenerator* _mg)
{
	mg = _mg;
}

CAdvGenerator::~CAdvGenerator()
{

}

void CAdvGenerator::Generate()
{
	rng.seed(mg->GetSeed());

	mapSize.x = 4;
	mapSize.y = 4;
	gridSize.x = mapSize.x * 64;
	gridSize.y = mapSize.y * 64;

	int players = 8;
	int teams = 2;
	int playersPerTeam = players / teams;

	for(int x = 0; x < playersPerTeam; x++)
	{
		int2 borderDistance = int2(gridSize.x / 10, gridSize.y / 10);
		int2 pos((rng() % (gridSize.x - borderDistance.x * 2)) + borderDistance.x,
				(rng() % (gridSize.y - borderDistance.y * 2)) + borderDistance.y);
		startPositions.push_back(pos);
	}

	CreateMaps();

	//Spawn land for each player
	//for(int x = 0; x < startPositions.size(); x++)
	//{
	//	MakePlayerLand(x);
	//}

}

void CAdvGenerator::MakePlayerLand(int player)
{
	int branches = 8;
	float direction = rng() % 360;
	int2 pos(startPositions[player]);
	MakeBranch(pos, direction, branches, 10.0f, 13.0f);
}

void CAdvGenerator::SetArea(int2 pos, float height, float size)
{
	int isize = (int)size / 2;
	int2 minpos(pos.x - isize, pos.y - isize);
	int2 maxpos(pos.x + isize, pos.y + isize);
	if(minpos.x < 0) minpos.x = 0;
	if(minpos.y < 0) minpos.y = 0;
	if(maxpos.x > gridSize.x) maxpos.x = gridSize.x;
	if(maxpos.y > gridSize.y) maxpos.y = gridSize.y;
	for(int x = minpos.x; x < maxpos.x; x++)
	{
		for(int y = minpos.y; y < maxpos.y; y++)
		{
			float oldHeight = GetHeight(x, y);
			SetHeight(x, y, 50.0f); //(oldHeight * 9.0f + height * 1.0f) * 0.1f);
		}
	}
}

void CAdvGenerator::MakeBranch(int2 pos, float direction, int branches, float stepsize, float width)
{
	//SetArea(pos, 100.0f, width);

	branches--;
	
	//Make sub-branches
	pos.x += streflop::sin(direction * 0.0174532925f) * stepsize;
	pos.y += streflop::cos(direction * 0.0174532925f) * stepsize;
	float deviation = (float)(rng() % 60 - 30) * 0.1f;
	float stepchange = 1.0f + (float)(rng() % 30 - 15) * 0.1f;
	stepsize = stepsize * stepchange;
	for(int x = 0; x < branches; x++)
	{
		direction += deviation;
		MakeBranch(pos, direction, branches, stepsize, stepsize * 0.4f);
	}
}