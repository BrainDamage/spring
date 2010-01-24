#ifndef __ARCHIVEMEMORY_H_
#define __ARCHIVEMEMORY_H_

#include <string>
#include <vector>

#include "ArchiveBase.h"

struct MemoryFile
{
	std::string name;
	char* buffer;
	int offset;
	int size;
	int handle;
};

class CArchiveMemory : public CArchiveBase
{
public:
	CArchiveMemory(const std::string& archiveName);
	virtual ~CArchiveMemory();
	virtual bool IsOpen();
	virtual int OpenFile(const std::string& fileName);
	virtual int ReadFile(int handle, void* buffer, int numBytes);
	virtual void CloseFile(int handle);
	virtual void Seek(int handle, int pos);
	virtual int Peek(int handle);
	virtual bool Eof(int handle);
	virtual int FileSize(int handle);
	virtual int FindFiles(int cur, std::string* name, int* size);
	//virtual unsigned int GetCrc32 (const std::string& fileName);

	void AddFile(const std::string& name, char* buffer, int size);

	inline const std::string& getName() const
	{ return name; }

private:
	MemoryFile& getByHandle(int handle);

	std::string name;
	std::vector<MemoryFile> files;
};

#endif //__ARCHIVEMEMORY_H_