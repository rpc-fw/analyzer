#include "../CgiCallback.h"

#include "AnalysisCgiHandler.h"
#include "sharedtypes.h"

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

	AnalysisCommand cmd;
	cmd.commandType = AnalysisCommand::BLOCK;
	analysisCommandMailbox.Write(cmd);
	while (!analysisAckMailbox.Read());

	error_t e = httpWriteStream(connection, bufferPtr, 4 * 65536 * sizeof(float));

	cmd.commandType = AnalysisCommand::DONE;
	analysisCommandMailbox.Write(cmd);
	while (!analysisAckMailbox.Read());

	return e;
}
