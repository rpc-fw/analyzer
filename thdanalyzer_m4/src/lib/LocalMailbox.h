#ifndef LOCALMAILBOX_H_
#define LOCALMAILBOX_H_

template <typename T>
class LocalMailbox
{
private:
	enum MailboxStatus
	{
		MailboxStatusFree,
		MailboxStatusPending
	};

public:
	LocalMailbox()
	{
		_status = MailboxStatusFree;
	}

	void Write(const T& data)
	{
		// block if mailbox is reserved
		while (_status != MailboxStatusFree);

		// ok, now it's free
		const_cast<T&>(_data) = data;
		_status = MailboxStatusPending;
	}

	bool Read(T& target)
	{
		if (_status != MailboxStatusPending) {
			return false;
		}

		target = const_cast<T&> (_data);

		_status = MailboxStatusFree;

		return true;
	}

	volatile MailboxStatus _status;
	volatile T _data;
};

#endif /* LOCALMAILBOX_H_ */
