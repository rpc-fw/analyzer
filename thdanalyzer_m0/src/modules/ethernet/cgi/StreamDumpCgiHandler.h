#ifndef STREAMDUMPCGIHANDLER_H_
#define STREAMDUMPCGIHANDLER_H_

#include "../CgiCallback.h"

class StreamDumpCgiHandler : public ICgiCallbackHandler
{
public:
	StreamDumpCgiHandler();
	virtual ~StreamDumpCgiHandler();

	virtual error_t Header(HttpConnection *connection, HttpResponse *response);
	virtual error_t Request(HttpConnection *connection);
};

#endif /* STREAMDUMPCGIHANDLER_H_ */
