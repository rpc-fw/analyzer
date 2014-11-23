#ifndef LCD1602_H_
#define LCD1602_H_

#include <stdint.h>

class LCD1602
{
public:
	LCD1602();

	void Init();

	// Move cursor to column, row
	void Locate(int col, int row);

	// Write string on screen
	void Print(const char* string);

private:

	void PinInit();
	void InitFont();

	void Contrast(bool state);
	void Brightness(bool state);
	void RS(bool state);
	void RW(bool state);
	void E(bool state);

	void DB4(uint8_t state);
	void DB(uint8_t data);
};

#endif /* LCD1602_H_ */
