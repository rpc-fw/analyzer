#ifndef MEMORYDUMPCGIHANDLER_H_
#define MEMORYDUMPCGIHANDLER_H_

#include "../CgiCallback.h"

class MemoryDumpCgiHandler : public ICgiCallbackHandler
{
public:
	MemoryDumpCgiHandler();
	virtual ~MemoryDumpCgiHandler();

	virtual error_t Header(HttpConnection *connection, HttpResponse *response);
	virtual error_t Request(HttpConnection *connection);
};

#endif
