#ifndef MEMORYSLOT_H_
#define MEMORYSLOT_H_

template <typename T, typename Memory>
class MemorySlot
{
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
	static uint32_t EndPtr()
	{
		return RoundPtr(Memory::EndPtr()) + RoundPtr(sizeof(T));
	}

	volatile T& operator*()
	{
		return * reinterpret_cast<volatile T*> (Memory::EndPtr());
	}
};


#endif /* MEMORYSLOT_H_ */
