#include "MapGenerator.h"
#include <fstream>

CMapGenerator::CMapGenerator()
{
	seed = 0;
	smf = 0;
	smd = 0;
	smt = 0;
}

CMapGenerator::~CMapGenerator()
{

}

void CMapGenerator::Seed(int _seed)
{
	seed = _seed;
}
void CMapGenerator::Generate()
{
	std::string pathName = "maps/mapses/Colors V3";
	std::ifstream s;

	s.open((pathName + ".smd").c_str(), std::ios_base::in | std::ios_base::binary);
	s.seekg(0, std::ios_base::end);
	smdSize = s.tellg();
	s.seekg(0, std::ios_base::beg);
	smd = new char[smdSize];
	s.read(smd, smdSize);
	
	s.close();

	s.open((pathName + ".smf").c_str(), std::ios_base::in | std::ios_base::binary);
	s.seekg(0, std::ios_base::end);
	smfSize = s.tellg();
	s.seekg(0, std::ios_base::beg);
	smf = new char[smfSize];
	s.read(smf, smfSize);
	s.close();

	s.open((pathName + ".smt").c_str(), std::ios_base::in | std::ios_base::binary);
	s.seekg(0, std::ios_base::end);
	smtSize = s.tellg();
	s.seekg(0, std::ios_base::beg);
	smt = new char[smtSize];
	s.read(smt, smtSize);
	s.close();
}

CArchiveMemory* CMapGenerator::CreateArchive(const std::string& name)
{
	std::string mapName = "Colors V3";

	CArchiveMemory* am = new CArchiveMemory(name);

	am->AddFile("maps/" + mapName + ".smd", smd, smdSize);
	am->AddFile("maps/" + mapName + ".smf", smf, smfSize);
	am->AddFile("maps/" + mapName + ".smt", smt, smtSize);

	return am;
}