#ifndef IPCMAILBOX_H_
#define IPCMAILBOX_H_

#include "LPC43xx.h"

template <uint32_t memaddr>
class IpcMailboxMemory
{
public:
	IpcMailboxMemory() {}

	static uint32_t EndPtr()
	{
		return memaddr;
	}
};

template <typename T, typename Memory>
class IpcMailbox
{
private:
	enum MailboxStatus
	{
		MailboxStatusFree,
		MailboxStatusPending
	};

	static uint32_t RoundPtr(uint32_t ptr)
	{
		if (sizeof(T) < 4) {
			return (ptr + 3) & ~3;
		}
		else {
			return (ptr + sizeof(T)-1) & ~(sizeof(T)-1);
		}
	}

public:
	IpcMailbox()
	{
		volatile MailboxStatus* statusptr = reinterpret_cast<volatile MailboxStatus*> (RoundPtr(Memory::EndPtr()));

		*statusptr = MailboxStatusFree;
	}

	static uint32_t EndPtr()
	{
		return RoundPtr(Memory::EndPtr() + sizeof(uint32_t)) + RoundPtr(sizeof(T));
	}

	void Write(const T& data)
	{
		volatile MailboxStatus* statusptr = reinterpret_cast<volatile MailboxStatus*> (RoundPtr(Memory::EndPtr()));
		T* tptr = reinterpret_cast<T*> (RoundPtr(Memory::EndPtr() + sizeof(uint32_t)));

		// block if mailbox is reserved
		while (*statusptr != MailboxStatusFree);

		// ok, now it's free
		*tptr = data;
		*statusptr = MailboxStatusPending;

		// memory barrier
		__DSB();

		// trigger remote processor
		//__SEV();
	}

	bool Read(T& target)
	{
		volatile MailboxStatus* statusptr = reinterpret_cast<volatile MailboxStatus*> (RoundPtr(Memory::EndPtr()));
		if (*statusptr != MailboxStatusPending) {
			return false;
		}

		T* tptr = reinterpret_cast<T*> (RoundPtr(Memory::EndPtr() + sizeof(uint32_t)));
		target = *tptr;

		*statusptr = MailboxStatusFree;

		return true;
	}

	bool Read()
	{
		volatile MailboxStatus* statusptr = reinterpret_cast<volatile MailboxStatus*> (RoundPtr(Memory::EndPtr()));
		if (*statusptr != MailboxStatusPending) {
			return false;
		}

		*statusptr = MailboxStatusFree;

		return true;
	}
};

#endif /* IPCMAILBOX_H_ */
