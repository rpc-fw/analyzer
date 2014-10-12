#include <vector>

#include "CgiCallback.h"

class CgiCallbackDispatcher
{
public:
	CgiCallbackDispatcher() {}

	void SetHandler(const char *tag, ICgiCallbackHandler* handler);

	error_t HandleHeader(const char *tag, int taglen, HttpConnection *connection, HttpResponse *response);
	error_t HandleRequest(const char *tag, int taglen, HttpConnection *connection);

private:
	ICgiCallbackHandler* FindHandler(const char* tag, int taglen);

	typedef std::pair<const char*, ICgiCallbackHandler*> CgiCallbackHandlerEntry;
	std::vector<CgiCallbackHandlerEntry> handlers;
};

static CgiCallbackDispatcher cgiCallbackDispatcher;

int ParsePath(const char_t *path)
{
	int n = 0;
	for (; path[n] != '\0' && path[n] != '/' && path[n] != '#' && path[n] != '&'; n++) ;

	return n;
}

extern "C" error_t HttpCgiHeaderCallback(HttpConnection *connection, HttpResponse *response, const char_t *path)
{
	int namelen = ParsePath(path);

	return cgiCallbackDispatcher.HandleHeader(path, namelen, connection, response);
}

extern "C" error_t HttpCgiCallback(HttpConnection *connection, const char_t *param)
{
	int namelen = ParsePath(param);

	return cgiCallbackDispatcher.HandleRequest(param, namelen, connection);
}

void CgiCallbackDispatcher::SetHandler(const char *tag, ICgiCallbackHandler* handler)
{
	ICgiCallbackHandler* r = FindHandler(tag, strlen(tag));
	if (r != NULL)
	{
		// trying to register a duplicate
		return;
	}

	handlers.push_back(CgiCallbackHandlerEntry(tag, handler));
}

error_t CgiCallbackDispatcher::HandleHeader(const char *tag, int taglen, HttpConnection *connection, HttpResponse *response)
{
	ICgiCallbackHandler* handler = FindHandler(tag, taglen);
	if (handler == NULL) {
		return ERROR_NOT_FOUND;
	}

	return handler->Header(connection, response);
}

error_t CgiCallbackDispatcher::HandleRequest(const char *tag, int taglen, HttpConnection *connection)
{
	ICgiCallbackHandler* handler = FindHandler(tag, taglen);
	if (handler == NULL) {
		return ERROR_NOT_FOUND;
	}

	return handler->Request(connection);
}

ICgiCallbackHandler* CgiCallbackDispatcher::FindHandler(const char* tag, int taglen)
{
	for (size_t i = 0; i < handlers.size(); i++) {
		if (!strncmp(handlers[i].first, tag, taglen) && handlers[i].first[taglen] == '\0') {
			return handlers[i].second;
		}
	}

	return NULL;
}

void SetCgiHandler(const char *tag, ICgiCallbackHandler& handler)
{
	cgiCallbackDispatcher.SetHandler(tag, &handler);
}
