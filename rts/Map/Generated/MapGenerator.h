#ifndef __MAPGENERATOR_H_
#define __MAPGENERATOR_H_

#include "System/FileSystem/ArchiveMemory.h"
#include "System/Vec2.h"
#include <vector>

class IMapGenerator
{
public:
	virtual ~IMapGenerator();

	virtual void Generate() = 0;

	inline int2& GetMapSize()
	{ return mapSize; }

	inline int2& GetGridSize()
	{ return gridSize; }

	inline float GetHeight(int x, int y)
	{ return heightMap[y * gridSize.x + x]; }

	inline float* GetHeights()
	{ return heightMap; }

	std::vector<int2>& GetStartPositions()
	{ return startPositions; }

protected:
	IMapGenerator();
	

	void CreateMaps();

	inline void SetHeight(int x, int y, float f)
	{ heightMap[y * gridSize.x + x] = f; }

	std::vector<int2> startPositions;

	float* heightMap;
	float* metalMap;

	int2 mapSize;
	int2 gridSize;
};

class CMapGenerator
{
public:
	CMapGenerator();
	virtual ~CMapGenerator();

	void SetSeed(int seed);
	void Generate();

	CArchiveMemory* CreateArchive(const std::string& name);

	inline int GetSeed()
	{ return seed; }

private:
	IMapGenerator* generator;
	int seed;

	struct FileBuffer
	{
		char* buffer;
		unsigned int size;
	};

	//For SMF
	FileBuffer CreateSMF();
	FileBuffer CreateSMT();
	FileBuffer CreateSMD();

	//For SM3
	//FileBuffer Create---();
	//FileBuffer Create---();
	//FileBuffer CreateSM3();
};

#endif //__MAPGENERATOR_H_
