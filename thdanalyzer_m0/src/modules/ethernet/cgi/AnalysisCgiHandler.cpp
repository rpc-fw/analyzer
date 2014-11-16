#include "../CgiCallback.h"

#include "AnalysisCgiHandler.h"
#include "../../analyzercontrol.h"

AnalysisCgiHandler::AnalysisCgiHandler()
{
}

AnalysisCgiHandler::~AnalysisCgiHandler()
{
}

error_t AnalysisCgiHandler::Header(HttpConnection *connection, HttpResponse *response)
{
	static const char mimeType[] = "application/octet-stream";
	response->contentType = mimeType;

	return NO_ERROR;
}

error_t AnalysisCgiHandler::Request(HttpConnection *connection)
{
	const float* bufferPtr = (const float*)(0x28000000 + 15*1048576);

	analyzercontrol.AnalysisStart();
	analyzercontrol.AnalysisRead();
	error_t e = httpWriteStream(connection, bufferPtr, 4 * 65536 * sizeof(float));
	analyzercontrol.AnalysisFinish();

	return e;
}
