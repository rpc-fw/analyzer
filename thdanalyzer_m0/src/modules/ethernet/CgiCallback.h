#ifndef CGICALLBACK_H_
#define CGICALLBACK_H_

extern "C" {

#include "tcp_ip_stack.h"
#include "dhcp_client.h"
#include "http_server.h"

};

extern "C" error_t HttpCgiHeaderCallback(HttpConnection *connection, HttpResponse *response, const char_t *path);
extern "C" error_t HttpCgiCallback(HttpConnection *connection, const char_t *param);

class ICgiCallbackHandler
{
public:
	virtual ~ICgiCallbackHandler() {}

	virtual error_t Header(HttpConnection *connection, HttpResponse *response) = 0;
	virtual error_t Request(HttpConnection *connection) = 0;
};

void SetCgiHandler(const char *tag, ICgiCallbackHandler& handler);

#endif /* CGICALLBACK_H_ */
