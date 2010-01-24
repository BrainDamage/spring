#include "ArchiveMemory.h"

CArchiveMemory::CArchiveMemory(const std::string& _name) : CArchiveBase(_name)
{
	name = _name;
}

CArchiveMemory::~CArchiveMemory()
{
	for(int x = 0; x < files.size(); x++)
	{
		delete[] files[x].buffer;
	}
}

bool CArchiveMemory::IsOpen()
{
	return true;
}

int CArchiveMemory::OpenFile(const std::string& fileName)
{
	for(int x = 0; x < files.size(); x++)
	{
		if(files[x].name == fileName) 
		{
			files[x].offset = 0;
			return files[x].handle;
		}
	}
	return 0;
}

int CArchiveMemory::ReadFile(int handle, void* buffer, int numBytes)
{
	MemoryFile& mf = getByHandle(handle);
	
	int bytesLeft = mf.size - mf.offset - 1;
	int bytesToRead = std::min(numBytes, bytesLeft);
	if(bytesToRead > 0)
	{
		memcpy(buffer, mf.buffer, bytesToRead);
		mf.offset += bytesToRead;
		return bytesToRead;
	}
	return 0;
}

void CArchiveMemory::CloseFile(int handle)
{
	MemoryFile& mf = getByHandle(handle);

	mf.offset = -1;
}

void CArchiveMemory::Seek(int handle, int pos)
{
	MemoryFile& mf = getByHandle(handle);

	if(pos >= 0 && pos < mf.size)
		mf.offset = pos;
	else
		throw std::runtime_error("Invalid location to seek to");
}

int CArchiveMemory::Peek(int handle)
{
	MemoryFile& mf = getByHandle(handle);

	if(mf.offset >= mf.size)
		return -1;
	return ((char*)mf.buffer)[mf.offset];	
}

bool CArchiveMemory::Eof(int handle)
{
	MemoryFile& mf = getByHandle(handle);
	return mf.offset >= mf.size;
}

int CArchiveMemory::FileSize(int handle)
{
	return getByHandle(handle).size;
}

int CArchiveMemory::FindFiles(int cur, std::string* name, int* size)
{
	*name = files[cur].name;
	*size = files[cur].size;
	if(cur < files.size() - 1)
		return cur + 1;
	return 0;
}

void CArchiveMemory::AddFile(const std::string& name, char* buffer, int size)
{
	static int handle;

	MemoryFile mf;
	mf.name = name;
	mf.buffer = buffer;
	mf.size = size;
	mf.handle = ++handle;
	mf.offset = -1;

	files.push_back(mf);
}

MemoryFile& CArchiveMemory::getByHandle(int handle)
{
	for(int x = 0; x < files.size(); x++)
	{
		if(files[x].handle == handle) 
		{
			if(files[x].offset == -1) throw new std::runtime_error("Can't get handle from closed file");
			return files[x];
		}
	}
	throw std::runtime_error("Unregistered handle. Pass a handle returned by CArchiveMemory::OpenFile.");
}