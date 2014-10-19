#include "tcp_ip_stack.h"
#include "resource_manager.h"

#include "cgi/MemoryDumpCgiHandler.h"
#include "cgi/StreamDumpCgiHandler.h"
#include "cgi/GeneratorParameterCgiHandler.h"
#include "CgiCallback.h"

uint8_t res[2048];
static size_t resAllocated = sizeof(ResHeader);

static uint8_t* AllocBytes(size_t numbytes)
{
	uint8_t* bytes = &res[resAllocated];
	resAllocated += numbytes;
	return bytes;
}

static size_t ResOffset(void *resptr)
{
	return (uint8_t*)resptr - res;
}

static uint8_t* ResPtr(size_t offset)
{
	return &res[offset];
}

static ResEntry* AllocEntry(size_t& dirsize, int type, const char* name)
{
	size_t namelen = strlen(name);
	size_t entrylen = sizeof(ResEntry) + namelen;

	ResEntry* e = (ResEntry*)AllocBytes(entrylen);
	memcpy((char*)&e[1], name, namelen);
	e->nameLength = namelen;

	//e->dataStart = ResOffset(AllocBytes(datalen));
	//e->dataLength = datalen;
	e->dataStart = 0;
	e->dataLength = 0;
	e->type = type;

	dirsize += entrylen;

	return e;
}

static void AllocDataString(ResEntry* entry, const char* content)
{
	size_t datalen = strlen(content);
	entry->dataStart = ResOffset(AllocBytes(datalen));
	entry->dataLength = datalen;

	memcpy((char*)ResPtr(entry->dataStart), content, datalen);
}

class HttpResourceManager
{
public:
	HttpResourceManager()
	{
	}

	void InitResources()
	{
		ResHeader* rootHeader = (ResHeader*)res;
		rootHeader->totalSize = sizeof(ResHeader);
		rootHeader->rootEntry.type = RES_TYPE_FILE;
		rootHeader->rootEntry.nameLength = 0;

		size_t dirsize = 0;
		ResEntry* indexEntry = AllocEntry(dirsize, RES_TYPE_FILE, "index.htm");
		ResEntry* memdumpEntry = AllocEntry(dirsize, RES_TYPE_CGI, "memory.raw");
		ResEntry* streamEntry = AllocEntry(dirsize, RES_TYPE_CGI, "stream.raw");
		ResEntry* genEntry = AllocEntry(dirsize, RES_TYPE_CGI, "gen");
		rootHeader->rootEntry.dataStart = ResOffset(indexEntry);
		rootHeader->rootEntry.dataLength = dirsize;

		AllocDataString(indexEntry, "Hello, world!\n");
		AllocDataString(memdumpEntry, "<!--#execcgi=memory.raw-->");
		AllocDataString(streamEntry, "<!--#execcgi=stream.raw-->");
		AllocDataString(genEntry, "<!--#execcgi=gen-->");

		SetCgiHandler("memory.raw", _memdump);
		SetCgiHandler("stream.raw", _stream);
		SetCgiHandler("gen", _genparam);
	}

private:
	MemoryDumpCgiHandler _memdump;
	StreamDumpCgiHandler _stream;
	GeneratorParameterCgiHandler _genparam;
};

static HttpResourceManager httpResources;

void InitHttpResources()
{
	httpResources.InitResources();
}
