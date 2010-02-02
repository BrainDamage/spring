#include "MapGenerator.h"
#include "Map/Smf/mapfile.h"
#include <fstream>

#include "AdvGenerator.h"

IMapGenerator::IMapGenerator()
{
	heightMap = 0;
	metalMap = 0;
}

IMapGenerator::~IMapGenerator()
{
	if(heightMap) delete[] heightMap;
	if(metalMap) delete[] metalMap;
}

void IMapGenerator::CreateMaps()
{
	int dimensions = (gridSize.x + 1) * (gridSize.y + 1);
	heightMap = new float[dimensions];
	metalMap = new float[dimensions];
	for(int x = 0; x < dimensions; x++)
	{
		heightMap[x] = 0.0f;
		metalMap[x] = 0.0f;
	}
}

CMapGenerator::CMapGenerator()
{
	seed = 0;

	generator = new CAdvGenerator(this);
}

CMapGenerator::~CMapGenerator()
{
	delete generator;
}

void CMapGenerator::SetSeed(int _seed)
{
	seed = _seed;
}

//Generate the heightmap, start positions, metal, geo, etc
void CMapGenerator::Generate()
{
	generator->Generate();
}

//Create an archive from the files generated
CArchiveMemory* CMapGenerator::CreateArchive(const std::string& name)
{
	std::string mapName = "duck";

	CArchiveMemory* am = new CArchiveMemory(name);

	FileBuffer file;

	file = CreateSMD();
	am->AddFile("maps/" + mapName + ".smd", file.buffer, file.size);
	file = CreateSMF();
	am->AddFile("maps/" + mapName + ".smf", file.buffer, file.size);
	file = CreateSMT();
	am->AddFile("maps/duck.smt", file.buffer, file.size);

	return am;
}

///---------------------------------------------------------------------------
///-------           Functions to generate SMF files             -------------
///---------------------------------------------------------------------------
CMapGenerator::FileBuffer CMapGenerator::CreateSMF()
{
	FileBuffer b;

	SMFHeader smfHeader;
	MapTileHeader smfTile;
	MapFeatureHeader smfFeature;

	//--- Make SMFHeader ---
	strcpy(smfHeader.magic, "spring map file");
	smfHeader.version = 1;
	smfHeader.mapid = 0x524d4746;

	//Set settings
	smfHeader.mapx = generator->GetMapSize().x * 64;
	smfHeader.mapy = generator->GetMapSize().y * 64;
	smfHeader.squareSize = 8;
	smfHeader.texelPerSquare = 8;
	smfHeader.tilesize = 32;
	smfHeader.minHeight = -100;
	smfHeader.maxHeight = 0xFFFF; //So its pretty much 1:1
	
	int numTiles = 2087; //32 * 32 * (mapSize.x  / 2) * (mapSize.y / 2);
	const char smtFileName[] = "duck.smt";

	//--- Extra headers ---
	ExtraHeader vegHeader;
	vegHeader.type = MEH_Vegetation;
	vegHeader.size = sizeof(int);
	
	smfHeader.numExtraHeaders =  1;

	//Make buffers for each map
	int heightmapDimensions = (smfHeader.mapx + 1) * (smfHeader.mapy + 1);
	int typemapDimensions = (smfHeader.mapx / 2) * (smfHeader.mapy / 2);
	int metalmapDimensions = (smfHeader.mapx / 2) * (smfHeader.mapy / 2);
	int tilemapDimensions =  (smfHeader.mapx * smfHeader.mapy) / 16;
	int vegmapDimensions = (smfHeader.mapx / 4) * (smfHeader.mapy / 4);

	int heightmapSize = heightmapDimensions * sizeof(short);
	int typemapSize = typemapDimensions * sizeof(unsigned char);
	int metalmapSize = metalmapDimensions * sizeof(unsigned char);
	int tilemapSize = tilemapDimensions * sizeof(int);
	int tilemapTotalSize = sizeof(MapTileHeader) + sizeof(numTiles) + sizeof(smtFileName) + tilemapSize;
	int vegmapSize = vegmapDimensions * sizeof(unsigned char);

	short* heightmapPtr = new short[heightmapDimensions];
	unsigned char* typemapPtr = new unsigned char[typemapDimensions];
	unsigned char* metalmapPtr = new unsigned char[metalmapDimensions];
	int* tilemapPtr = new int[tilemapDimensions];
	unsigned char* vegmapPtr = new unsigned char[vegmapDimensions];
	
	//--- Set offsets, increment each member with the previous one ---
	int vegmapOffset = sizeof(smfHeader) + sizeof(vegHeader) + sizeof(int);
	smfHeader.heightmapPtr = vegmapOffset + vegmapSize;
	smfHeader.typeMapPtr = smfHeader.heightmapPtr + heightmapSize;
	smfHeader.tilesPtr = smfHeader.typeMapPtr + typemapSize;
	smfHeader.minimapPtr = 0; //smfHeader.tilesPtr + sizeof(MapTileHeader);
	smfHeader.metalmapPtr = smfHeader.tilesPtr + tilemapTotalSize;  //smfHeader.minimapPtr + minimapSize;
	smfHeader.featurePtr = smfHeader.metalmapPtr + metalmapSize;
	
	//--- Make MapTileHeader ---
	smfTile.numTileFiles = 1;
	smfTile.numTiles = numTiles;

	//--- Make MapFeatureHeader ---
	smfFeature.numFeatures = 0;
	smfFeature.numFeatureType = 0;

	//--- Update Ptrs and write to buffer ---
	memset(vegmapPtr, 0, vegmapSize);
	for(int x = 0; x < heightmapDimensions; x++)
		heightmapPtr[x] = (short)generator->GetHeights()[x];
	memset(typemapPtr, 0, typemapSize); 
	memset(tilemapPtr, 0, tilemapSize);
	memset(metalmapPtr, 0, metalmapSize); 

	b.size = smfHeader.featurePtr + sizeof(smfFeature);
	b.buffer = new char[b.size];

	memcpy(b.buffer, &smfHeader, sizeof(smfHeader));
	memcpy(b.buffer + sizeof(smfHeader), &vegHeader, sizeof(vegHeader));
	memcpy(b.buffer + sizeof(smfHeader) + sizeof(vegHeader), &vegmapOffset, sizeof(vegmapOffset));
	memcpy(b.buffer + vegmapOffset, vegmapPtr, vegmapSize);
	memcpy(b.buffer + smfHeader.heightmapPtr, heightmapPtr, heightmapSize);
	memcpy(b.buffer + smfHeader.typeMapPtr, typemapPtr, typemapSize);
	memcpy(b.buffer + smfHeader.tilesPtr, &smfTile, sizeof(smfTile));
	memcpy(b.buffer + smfHeader.tilesPtr + sizeof(smfTile), &numTiles, sizeof(numTiles));
	strcpy(b.buffer + smfHeader.tilesPtr + sizeof(smfTile) + sizeof(numTiles), smtFileName);
	memcpy(b.buffer + smfHeader.tilesPtr + sizeof(smfTile) + sizeof(numTiles) + sizeof(smtFileName), tilemapPtr, tilemapSize);
	//memcpy(b.buffer + smfHeader.minimapPtr, , );
	memcpy(b.buffer + smfHeader.metalmapPtr, metalmapPtr, metalmapSize);
	memcpy(b.buffer + smfHeader.featurePtr, &smfFeature, sizeof(smfFeature));

	/*std::ifstream s;
	s.open("maps/mapses/duck.smf", std::ios_base::in | std::ios_base::binary);
	s.seekg(0, std::ios_base::end);
	b.size = s.tellg();
	s.seekg(0, std::ios_base::beg);
	b.buffer = new char[b.size];
	s.read(b.buffer, b.size);
	s.close();*/

	return b;
}

CMapGenerator::FileBuffer CMapGenerator::CreateSMT()
{
	//--- Make TileFileHeader ---
	/*TileFileHeader smtHeader;
	strcpy(smtHeader.magic, "spring tilefile");
	smtHeader.version = 1;
	smtHeader.numTiles = 32 * 32 * mapx/2 * mapy/2;
	smtHeader.tileSize = 32;
	smtHeader.compressionType = 1;*/

	FileBuffer b;

	std::ifstream s;
	s.open("maps/mapses/duck.smt", std::ios_base::in | std::ios_base::binary);
	s.seekg(0, std::ios_base::end);
	b.size = s.tellg();
	s.seekg(0, std::ios_base::beg);
	b.buffer = new char[b.size];
	s.read(b.buffer, b.size);
	s.close();

	return b;
}

CMapGenerator::FileBuffer CMapGenerator::CreateSMD()
{
	FileBuffer b;

	std::ifstream s;
	s.open("maps/mapses/duck.smd", std::ios_base::in | std::ios_base::binary);
	s.seekg(0, std::ios_base::end);
	b.size = s.tellg();
	s.seekg(0, std::ios_base::beg);
	b.buffer = new char[b.size];
	s.read(b.buffer, b.size);
	s.close();

	return b;
}

///---------------------------------------------------------------------------
///-------           Functions to generate SM3 files             -------------
///---------------------------------------------------------------------------

//todo 