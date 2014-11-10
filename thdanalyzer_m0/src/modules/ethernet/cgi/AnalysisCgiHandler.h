#ifndef ANALYSISCGIHANDLER_H_
#define ANALYSISCGIHANDLER_H_

#include "../CgiCallback.h"

class AnalysisCgiHandler : public ICgiCallbackHandler
{
public:
	AnalysisCgiHandler();
	virtual ~AnalysisCgiHandler();

	virtual error_t Header(HttpConnection *connection, HttpResponse *response);
	virtual error_t Request(HttpConnection *connection);
};

#endif
