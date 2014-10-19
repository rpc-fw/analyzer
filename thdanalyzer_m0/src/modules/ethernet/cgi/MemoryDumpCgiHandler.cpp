#include "../CgiCallback.h"

#include "MemoryDumpCgiHandler.h"
#include "sharedtypes.h"

MemoryDumpCgiHandler::MemoryDumpCgiHandler()
{
}

MemoryDumpCgiHandler::~MemoryDumpCgiHandler()
{
}

error_t MemoryDumpCgiHandler::Header(HttpConnection *connection, HttpResponse *response)
{
	static const char mimeType[] = "application/octet-stream";
	response->contentType = mimeType;

	return NO_ERROR;
}

error_t MemoryDumpCgiHandler::Request(HttpConnection *connection)
{
	const int32_t* startPtr = *oldestPtr;
	const int32_t bufferend = 16*1048576;
	int32_t startpos = (int32_t)startPtr & 0xFFFFFF;
	int32_t endpos = startpos + 16*1048576/2;

	int firstpartlen = 0;
	int secondpartlen = 0;
	if (endpos > bufferend) {
		secondpartlen = endpos - bufferend;
		firstpartlen = bufferend - startpos;
	}
	else {
		firstpartlen = endpos - startpos;
		secondpartlen = 0;
	}

	int secondstart = 0x28000000;
	httpWriteStream(connection, &firstpartlen, 4);
	httpWriteStream(connection, &startPtr, 4);
	httpWriteStream(connection, &secondpartlen, 4);
	httpWriteStream(connection, &secondstart, 4);

	if (firstpartlen > 0) {
		httpWriteStream(connection, (void*)startPtr, firstpartlen);
	}
	if (secondpartlen > 0) {
		httpWriteStream(connection, (void*)0x28000000, secondpartlen);
	}

	return NO_ERROR;
}
