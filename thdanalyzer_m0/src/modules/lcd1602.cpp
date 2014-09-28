#define CMSIS_BITPOSITIONS
#include "LPC43xx.h"

extern "C" {
#include "lpc43xx_cgu.h"
#include "lpc43xx_scu.h"
}

#include "lcd1602.h"

static void delay_us(volatile uint32_t us)
{
	us *= (SystemCoreClock / 1000000) / 3;
	while(us--);
}

static void delay_ms(uint32_t ms)
{
	delay_us(ms * 1000);
}

#define LCD_DELAY_WRITE delay_us(100)
#define LCD_DELAY_SHORT delay_ms(1)
#define LCD_DELAY_LONG delay_ms(6)

LCD1602::LCD1602()
{

}

void LCD1602::Contrast(bool state)
{
	if (state) {
		LPC_GPIO_PORT->SET[0] |= (1 << 11);
	}
	else {
		LPC_GPIO_PORT->CLR[0] |= (1 << 11);
	}
}

void LCD1602::Brightness(bool state)
{
	if (state) {
		LPC_GPIO_PORT->SET[1] |= (1 << 8);
	}
	else {
		LPC_GPIO_PORT->CLR[1] |= (1 << 8);
	}
}

void LCD1602::RS(bool state)
{
	if (state) {
		LPC_GPIO_PORT->SET[3] |= (1 << 1);
	}
	else {
		LPC_GPIO_PORT->CLR[3] |= (1 << 1);
	}
}

void LCD1602::RW(bool state)
{
	if (state) {
		// Read
		LPC_GPIO_PORT->DIR[5] &= ~((1 << 15) | (1 << 5) | (1 << 4) | (1 << 3));
		LPC_GPIO_PORT->SET[3] |= (1 << 2);
	}
	else {
		// Write
		LPC_GPIO_PORT->CLR[3] |= (1 << 2);
		LPC_GPIO_PORT->DIR[5] |= (1 << 15) | (1 << 5) | (1 << 4) | (1 << 3);
	}
}

void LCD1602::E(bool state)
{
	if (state) {
		LPC_GPIO_PORT->SET[0] |= (1 << 5);
	}
	else {
		LPC_GPIO_PORT->CLR[0] |= (1 << 5);
	}
}

/*
uint8_t LCD1602::Read4()
{
	uint32_t reg = LPC_GPIO_PORT->PIN[5];
	uint8_t bits = (((reg >> 15) << 3) & 0x8) | ((reg >> 3) & 0x7);

	return bits;
}

uint8_t LCD1602::ReadFlags()
{
	RW(true);
	RS(false);
	E(false);
	LCD_DELAY_WRITE;
	E(true);
	LCD_DELAY_WRITE;
	uint8_t hi = Read4();
	E(false);
	LCD_DELAY_WRITE;
	E(true);
	LCD_DELAY_WRITE;
	uint8_t lo = Read4();
	E(false);
	LCD_DELAY_WRITE;

	return lo | (hi << 4);
}
*/

void LCD1602::DB4(uint8_t state)
{
	//                            DB7        DB6        DB5        DB4
	LPC_GPIO_PORT->CLR[5] |= (1 << 15) | (1 << 5) | (1 << 4) | (1 << 3);
	LPC_GPIO_PORT->SET[5] |= (((state & 0x8) >> 3) << 15) | ((state & 0x7) << 3);
}

void LCD1602::DB(uint8_t data)
{
	RW(false);
	E(false);
	DB4(data >> 4);
	E(true);
	LCD_DELAY_WRITE;
	E(false);
	LCD_DELAY_WRITE;
	DB4(data & 0x0F);
	E(true);
	LCD_DELAY_WRITE;
	E(false);
	LCD_DELAY_WRITE;
}

void LCD1602::Init()
{
	PinInit();
	Brightness(false);
	RW(false);
	RS(false);
	DB4(0);
	E(false);

	// Wait 50 ms from power on
	delay_ms(50);

	DB4(3);

	// Run three clock cycles to init
	E(true);
	LCD_DELAY_SHORT;
	E(false);
	LCD_DELAY_LONG;

	E(true);
	LCD_DELAY_SHORT;
	E(false);
	LCD_DELAY_SHORT;

	E(true);
	LCD_DELAY_SHORT;
	E(false);
	LCD_DELAY_SHORT;

	// Configure display

	DB4(2);

	E(true);
	LCD_DELAY_SHORT;
	E(false);
	LCD_DELAY_LONG;

	DB(0x28);
	LCD_DELAY_SHORT;
	DB(0x8);
	LCD_DELAY_SHORT;
	DB(0x1);
	LCD_DELAY_SHORT;
	DB(0x6);
	LCD_DELAY_SHORT;

	RS(false);
	DB(0x0c);
	LCD_DELAY_LONG;

	// Test code, TODO

	Locate(0, 0);
	Print("Hello");
	//Brightness(true);
	Contrast(false);

	// Enable SCT peripheral clock
	CGU_ConfigPWR(CGU_PERIPHERAL_SCT, ENABLE);
	LPC_CCU1->CLK_M4_SCT_CFG |= CCU1_CLK_M4_SCT_CFG_RUN_Msk;
	while(!(LPC_CCU1->CLK_M4_SCT_STAT & CCU1_CLK_M4_SCT_STAT_RUN_Msk));

	scu_pinmux(0x1,  5,  GPIO_PUP, FUNC1); // CTOUT_10

	LPC_SCT->CONFIG = (1 << 17);
	LPC_SCT->CTRL_L |= (12-1) << 5;
	LPC_SCT->MATCHREL[0].L = 100-1;
	LPC_SCT->MATCHREL[1].L = 1;
	LPC_SCT->EVENT[0].STATE = 0xFFFFFFFF;
	LPC_SCT->EVENT[0].CTRL = (1 << 12);
	LPC_SCT->EVENT[1].STATE = 0xFFFFFFFF;
	LPC_SCT->EVENT[1].CTRL = (1 << 12) | (1 << 0);

	LPC_SCT->OUT[10].SET = (1 << 0);
	LPC_SCT->OUT[10].CLR = (1 << 1);

	LPC_SCT->CTRL_L &= ~(1 << 2);

	// Fade in
	for (int i = 0; i < 100; i++)
	{
		LPC_SCT->MATCHREL[1].L = i;
		delay_ms(10);
	}
}

void LCD1602::PinInit()
{
	// Enable GPIO peripheral clock
	LPC_CCU1->CLK_M4_GPIO_CFG |= CCU1_CLK_M4_GPIO_CFG_RUN_Msk;
	while(!(LPC_CCU1->CLK_M4_GPIO_STAT & CCU1_CLK_M4_GPIO_STAT_RUN_Msk));

	// Reset pins
	LPC_GPIO_PORT->CLR[0] |= (1 << 11) | (1 << 5);
	LPC_GPIO_PORT->CLR[1] |= (1 << 8) | (1 << 5);
	LPC_GPIO_PORT->CLR[3] |= (1 << 2) | (1 << 1);
	LPC_GPIO_PORT->CLR[5] |= (1 << 15) | (1 << 5) | (1 << 4) | (1 << 3);

	// P1_4 contrast
	scu_pinmux(0x1,  4,  GPIO_NOPULL, FUNC0); // GPIO0[11]
	LPC_GPIO_PORT->DIR[0] |= (1 << 11);
	// P1_5 brightness
	scu_pinmux(0x1,  5,  GPIO_NOPULL, FUNC0); // GPIO1[8]
	LPC_GPIO_PORT->DIR[1] |= (1 << 8);
	// P2_3 db4
	// P2_4 db5
	// P2_5 db6
	scu_pinmux(0x2,  3,  GPIO_NOPULL, FUNC4); // GPIO5[3]
	scu_pinmux(0x2,  4,  GPIO_NOPULL, FUNC4); // GPIO5[4]
	scu_pinmux(0x2,  5,  GPIO_NOPULL, FUNC4); // GPIO5[5]
	LPC_GPIO_PORT->DIR[5] |= (1 << 5) | (1 << 4) | (1 << 3);
	// P6_2 rs
	scu_pinmux(0x6,  2,  GPIO_NOPULL, FUNC0); // GPIO3[1]
	LPC_GPIO_PORT->DIR[3] |= (1 << 1);
	// P6_3 r/#w
	scu_pinmux(0x6,  3,  GPIO_NOPULL, FUNC0); // GPIO3[2]
	LPC_GPIO_PORT->DIR[3] |= (1 << 2);
	// P6_6 enable
	scu_pinmux(0x6,  6,  GPIO_NOPULL, FUNC0); // GPIO0[5]
	LPC_GPIO_PORT->DIR[0] |= (1 << 5);
	// P6_7 db7
	scu_pinmux(0x6,  7,  GPIO_NOPULL, FUNC4); // GPIO5[15]
	LPC_GPIO_PORT->DIR[5] |= (1 << 15);
}

void LCD1602::Locate(int col, int row)
{
	RS(false);
	DB(0x80 | (row << 6) | col);
	LCD_DELAY_WRITE;
}

void LCD1602::Print(const char* string)
{
	RS(true);
	while (*string) {
		DB(*string);
		string++;
	}
}
