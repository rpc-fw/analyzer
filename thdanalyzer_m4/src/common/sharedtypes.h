#ifndef SHAREDTYPES_H_
#define SHAREDTYPES_H_

#define COMMON_SHMEM_ADDRESS (0x2000C010)
//#define COMMON_SHMEM_SIZE (256)

struct GeneratorParameters
{
	float frequency;
	float level;
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
}

#endif /* SHAREDTYPES_H_ */
