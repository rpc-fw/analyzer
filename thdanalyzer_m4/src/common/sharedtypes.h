#ifndef SHAREDTYPES_H_
#define SHAREDTYPES_H_

#define COMMON_SHMEM_ADDRESS (0x2000C010)
//#define COMMON_SHMEM_SIZE (256)

struct GeneratorParameters
{
	float frequency;
	float level;
};

typedef IpcMailboxMemory<COMMON_SHMEM_ADDRESS> MailboxMemory;
typedef IpcMailbox<GeneratorParameters, MailboxMemory> CommandMailbox;
CommandMailbox commandMailbox;
typedef IpcMailbox<bool, CommandMailbox> AckMailbox;
AckMailbox ackMailbox;

#endif /* SHAREDTYPES_H_ */
