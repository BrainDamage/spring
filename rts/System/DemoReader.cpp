#include "StdAfx.h"
#include "DemoReader.h"

#include <limits.h>
#include <stdexcept>
#include <assert.h>
#include "mmgr.h"

#include "Net/RawPacket.h"
#include "Game/GameVersion.h"
#ifndef DEDICATED_CLIENT
#include "FileSystem/FileHandler.h"
#else
#include <fstream>
#endif

/////////////////////////////////////
// CDemoReader implementation

CDemoReader::CDemoReader(const std::string& filename, float curTime)
{
	#ifndef DEDICATED_CLIENT
	std::string firstTry = "demos/" + filename;

	playbackDemo = new CFileHandler(firstTry);

	if (!playbackDemo->FileExists()) {
		delete playbackDemo;
		playbackDemo = new CFileHandler(filename);
	}

	if (!playbackDemo->FileExists()) {
		// file not found -> exception
		delete playbackDemo;
		playbackDemo = NULL;
		throw std::runtime_error(std::string("Demofile not found: ")+filename);
	}
	playbackDemo->Read((void*)&fileHeader, sizeof(fileHeader));
	#else
	playbackDemo = new std::ifstream( filename.c_str() );
	if (!playbackDemo->is_open())
	{
		delete playbackDemo;
		playbackDemo = NULL;
		throw std::runtime_error(std::string("Demofile not found: ")+filename);
	}

  playbackDemo->seekg(0, std::ios::end);
  size_t fileLength = playbackDemo->tellg();
  playbackDemo->seekg(0, std::ios::beg);

	playbackDemo->read((char*)&fileHeader, sizeof(fileHeader));

	#endif


	fileHeader.swab();

	if (memcmp(fileHeader.magic, DEMOFILE_MAGIC, sizeof(fileHeader.magic))
		|| fileHeader.version != DEMOFILE_VERSION
		|| fileHeader.headerSize != sizeof(fileHeader)
		// Don't compare spring version in debug mode: we don't want to make
		// debugging SVN demos impossible (because VERSION_STRING is different
		// each build.)
#ifndef _DEBUG
		|| (SpringVersion::Get().find("+") == std::string::npos && strcmp(fileHeader.versionString, SpringVersion::Get().c_str()))
#endif
	) {
		delete playbackDemo;
		playbackDemo = NULL;
		throw std::runtime_error(std::string("Demofile corrupt or created by a different version of Spring: ")+filename);
	}

	if (fileHeader.scriptSize != 0) {
		char* buf = new char[fileHeader.scriptSize];
		#ifndef DEDICATED_CLIENT
		playbackDemo->Read(buf, fileHeader.scriptSize);
		#else
		playbackDemo->read(buf, fileHeader.scriptSize);
		#endif
		setupScript = std::string(buf);
		delete[] buf;
	}
	#ifndef DEDICATED_CLIENT
	playbackDemo->Read((void*)&chunkHeader, sizeof(chunkHeader));
	#else
	playbackDemo->read((char*)&chunkHeader, sizeof(chunkHeader));
	#endif
	chunkHeader.swab();

	demoTimeOffset = curTime - chunkHeader.modGameTime - 0.1f;
	nextDemoRead = curTime - 0.01f;

	if (fileHeader.demoStreamSize != 0) {
		bytesRemaining = fileHeader.demoStreamSize;
	} else {
		// Spring crashed while recording the demo: replay until EOF,
		// but at most filesize bytes to block watching demo of running game.
		#ifndef DEDICATED_CLIENT
		bytesRemaining = playbackDemo->FileSize() - fileHeader.headerSize - fileHeader.scriptSize;
		#else
		bytesRemaining = fileLength - fileHeader.headerSize - fileHeader.scriptSize;
		#endif
	}
	bytesRemaining -= sizeof(chunkHeader);
}

netcode::RawPacket* CDemoReader::GetData(float curTime)
{
	if (ReachedEnd())
		return 0;

	// when paused, modGameTime wont increase so no seperate check needed
	if (nextDemoRead < curTime) {
		netcode::RawPacket* buf = new netcode::RawPacket(chunkHeader.length);
		#ifndef DEDICATED_CLIENT
		playbackDemo->Read((void*)(buf->data), chunkHeader.length);
		#else
		playbackDemo->read((char*)(buf->data), chunkHeader.length);
		#endif
		bytesRemaining -= chunkHeader.length;

		#ifndef DEDICATED_CLIENT
		playbackDemo->Read((void*)&chunkHeader, sizeof(chunkHeader));
		#else
		playbackDemo->read((char*)&chunkHeader, sizeof(chunkHeader));
		#endif
		chunkHeader.swab();
		nextDemoRead = chunkHeader.modGameTime + demoTimeOffset;
		bytesRemaining -= sizeof(chunkHeader);

		return buf;
	} else {
		return 0;
	}
}

bool CDemoReader::ReachedEnd() const
{
	#ifndef DEDICATED_CLIENT
	if (bytesRemaining <= 0 || playbackDemo->Eof())
	#else
	if (bytesRemaining <= 0 || playbackDemo->eof())
	#endif
		return true;
	else
		return false;
}

float CDemoReader::GetNextReadTime() const
{
	return chunkHeader.modGameTime;
}
