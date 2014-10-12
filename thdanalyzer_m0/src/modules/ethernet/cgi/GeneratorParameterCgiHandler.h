#ifndef GENERATORPARAMETERCGIHANDLER_H_
#define GENERATORPARAMETERCGIHANDLER_H_

#include "../CgiCallback.h"

class GeneratorParameterCgiHandler : public ICgiCallbackHandler
{
public:
	GeneratorParameterCgiHandler();
	virtual ~GeneratorParameterCgiHandler();

	virtual error_t Header(HttpConnection *connection, HttpResponse *response);
	virtual error_t Request(HttpConnection *connection);
};

#endif
