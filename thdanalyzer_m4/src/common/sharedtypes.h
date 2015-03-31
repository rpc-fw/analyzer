#ifndef SHAREDTYPES_H_
#define SHAREDTYPES_H_

#define COMMON_SHMEM_ADDRESS (0x2000C010)
//#define COMMON_SHMEM_SIZE (256)

struct GeneratorParameters
{
	float _frequency;
	float _level;
	bool _balancedio;

	enum OperationMode
	{
		OperationModeTHD = 0,
		OperationModeFrequencyAnalysis = 1,
		OperationModeDCVoltageControl = 2
	};

	OperationMode _analysismode;

	float _cv0;
	float _cv1;

	GeneratorParameters() {}

	GeneratorParameters(float frequency, float level, bool balancedio, OperationMode analysismode, float cv0, float cv1)
	: _frequency(frequency),
	  _level(level),
	  _balancedio(balancedio),
	  _analysismode(analysismode),
	  _cv0(cv0),
	  _cv1(cv1)
	{
	}
};

struct AnalysisCommand
{
	enum CommandType {
		DONE = 0,
		BLOCK = 1
	};

	CommandType commandType;
};

#include "IpcMailbox.h"
#include "MemorySlot.h"

namespace {
	typedef IpcMailboxMemory<COMMON_SHMEM_ADDRESS> MailboxMemory;

	typedef IpcMailbox<GeneratorParameters, MailboxMemory> CommandMailbox;
	CommandMailbox commandMailbox;

	typedef IpcMailbox<bool, CommandMailbox> AckMailbox;
	AckMailbox ackMailbox;

	typedef IpcMailbox<AnalysisCommand, AckMailbox> AnalysisCommandMailbox;
	AnalysisCommandMailbox analysisCommandMailbox;

	typedef IpcMailbox<bool, AnalysisCommandMailbox> AnalysisAckMailbox;
	AnalysisAckMailbox analysisAckMailbox;

	typedef MemorySlot<const int32_t*, AnalysisAckMailbox> OldestPtr;
	OldestPtr oldestPtr;

	typedef MemorySlot<const int32_t*, OldestPtr> LatestPtr;
	LatestPtr latestPtr;

	typedef MemorySlot<float, LatestPtr> DistortionLevel;
	DistortionLevel distortionLevel;

	typedef MemorySlot<float, DistortionLevel> DistortionFrequency;
	DistortionFrequency distortionFrequency;
}

#endif /* SHAREDTYPES_H_ */
