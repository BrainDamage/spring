#ifndef __MAPGENERATOR_H_
#define __MAPGENERATOR_H_

#include "System/FileSystem/ArchiveMemory.h"

class CMapGenerator
{
public:
	CMapGenerator();
	virtual ~CMapGenerator();

	void Seed(int seed);
	void Generate();
	CArchiveMemory* CreateArchive(const std::string& name);

private:
	int seed;

	char* smd;
	char* smt;
	char* smf;
	int smdSize;
	int smtSize;
	int smfSize;
};

#endif //__MAPGENERATOR_H_