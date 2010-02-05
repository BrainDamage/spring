#ifndef __ADVGENERATOR_H_
#define __ADVGENERATOR_H_

#include "MapGenerator.h"
#include <boost/random/linear_congruential.hpp>

class CAdvGenerator : public IMapGenerator
{
public:
	CAdvGenerator(CMapGenerator* mg);
	virtual ~CAdvGenerator();

	virtual void Generate();

private:
	void MakePlayerLand(int player);
	void SetArea(int2 pos, float height, float size);
	void MakeBranch(int2 pos, float direction, int branches, float stepsize, float width);

	CMapGenerator* mg;
	boost::minstd_rand rng;
};

#endif //__ADVGENERATOR_H_