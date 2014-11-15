#ifndef FRONTI2C_H_
#define FRONTI2C_H_

#include <stdint.h>

class FrontI2C
{
public:
	void Init();

	bool Read(uint8_t address, uint8_t* buffer, int count);
	bool Write(uint8_t address, uint8_t* buffer, int count);
};

#endif /* FRONTI2C_H_ */
