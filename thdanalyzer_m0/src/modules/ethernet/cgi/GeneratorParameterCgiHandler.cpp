#include <stdio.h>
#include <stdlib.h>
#include "../CgiCallback.h"

#include "GeneratorParameterCgiHandler.h"


enum ParameterId
{
	UNKNOWN = 0,
	FREQUENCY,
	LEVEL
};

ParameterId ParseRequest(const char* request_uri)
{
	if (!strcmp(request_uri, "/gen/frequency")) {
		return FREQUENCY;
	}

	if (!strcmp(request_uri, "/gen/level")) {
		return LEVEL;
	}

	return UNKNOWN;
}

bool ParseFloat(float& value, const char* query)
{
	int n = 0;

	// parse in parts
	while (query[n] != '\0') {
		// find beginning of next entry
		int nextn = n;
		int length = 0;
		while (query[nextn] != '\0' && query[nextn] != '&') {
			nextn++;
		}
		length = nextn - n;
		if (query[nextn] == '&') {
			nextn++;
		}
		if (length > 3) {
			if (query[n] == 'f' && query[n + 1] == '=') {
				// wow, found a suitable parameter
				char* endptr;
				float result = strtof(&query[n+2], &endptr);
				// parsed length must match found entry length!
				if (endptr != &query[n + length]) {
					return false;
				}
				// everything ok
				value = result;
				return true;
			}
		}

		n = nextn;
	}

	return false;
}

GeneratorParameterCgiHandler::GeneratorParameterCgiHandler()
{
}

GeneratorParameterCgiHandler::~GeneratorParameterCgiHandler()
{
}

error_t GeneratorParameterCgiHandler::Header(HttpConnection *connection, HttpResponse *response)
{
	static const char mimeType[] = "text/plain";
	response->contentType = mimeType;

	// Check that the target parameter is valid
	ParameterId paramid = ParseRequest(connection->request.uri);
	if (paramid == UNKNOWN) {
		return ERROR_NOT_FOUND;
	}

	return NO_ERROR;
}

struct GeneratorParameters
{
	int32_t update;
	float frequency;
	float level;
};

const char invalidParameterReply[] = "Invalid parameters\n";

error_t GeneratorParameterCgiHandler::Request(HttpConnection *connection)
{
	volatile GeneratorParameters* params = (volatile GeneratorParameters*) 0x2000C010;

	ParameterId paramid = ParseRequest(connection->request.uri);

	char reply[256];
	int n = 0;

	switch (paramid)
	{
	case FREQUENCY:
	{
		float frequency = 0.0;
		bool parsed = ParseFloat(frequency, connection->request.queryString);
		if (!parsed) {
			httpWriteStream(connection, invalidParameterReply, sizeof(invalidParameterReply));
			return NO_ERROR;
		}
		if (frequency > 23000.0) frequency = 23000.0;
		else if (frequency < -1.0) frequency = 1.0;
		params->frequency = frequency;
		params->update++;
		n = snprintf(reply, sizeof(reply), "Frequency set to %f\n", params->frequency);
		break;
	}
	case LEVEL:
	{
		float level = 0.0;
		bool parsed = ParseFloat(level, connection->request.queryString);
		if (!parsed) {
			httpWriteStream(connection, invalidParameterReply, sizeof(invalidParameterReply));
			return NO_ERROR;
		}
		if (level > 20.0) level = 20.0;
		else if (level < -80.0) level = -80.0;
		params->level = level;
		params->update++;
		n = snprintf(reply, sizeof(reply), "Level set to %f dBu\n", params->level);
		break;
	}
	default:
		// Shouldn't reach here, handled in Header()
		return ERROR_NOT_FOUND;
	}

	httpWriteStream(connection, reply, n);

	return NO_ERROR;
}
