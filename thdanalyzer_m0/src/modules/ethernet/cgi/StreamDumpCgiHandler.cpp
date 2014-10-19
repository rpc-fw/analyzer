#include "../CgiCallback.h"

#include "StreamDumpCgiHandler.h"
#include "sharedtypes.h"

StreamDumpCgiHandler::StreamDumpCgiHandler()
{
}

StreamDumpCgiHandler::~StreamDumpCgiHandler()
{
}

error_t StreamDumpCgiHandler::Header(HttpConnection *connection, HttpResponse *response)
{
	static const char mimeType[] = "application/octet-stream";
	response->contentType = mimeType;

	return NO_ERROR;
}

error_t StreamDumpCgiHandler::Request(HttpConnection *connection)
{
	const int32_t* startptr = *latestPtr;
	int32_t startpos = (int32_t)startptr & 0xFFFFFF;
	const int32_t bufferend = 0x1000000;
	const int32_t* bufferptr = (int32_t*)0x28000000;

	static const volatile int32_t* greatestptr = 0;

	error_t error;
	do {
		// wait for more data
		const int32_t* curptr = *latestPtr;
		while (curptr == startptr) {
			curptr = *latestPtr;
		}

		if (curptr > greatestptr) greatestptr = curptr;

		int32_t curpos = (int32_t)curptr & 0xFFFFFF;

		if (curpos < startpos) {
			if (startpos != bufferend) {
				error = httpWriteStream(connection, startptr, bufferend - startpos);
			}
			if (curpos != 0) {
				error = httpWriteStream(connection, bufferptr, curpos);
			}
		}
		else {
			error = httpWriteStream(connection, startptr, curpos - startpos);
		}
		startptr = curptr;
		startpos = curpos;

	} while(error == NO_ERROR);

	return NO_ERROR;
}
