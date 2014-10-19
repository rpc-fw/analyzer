#define CMSIS_BITPOSITIONS
#include "LPC43xx.h"

extern "C" {
#include "lpc43xx_cgu.h"
#include "lpc43xx_scu.h"
#include "lpc43xx_i2s.h"
}

#include "audio.h"

static void audio_waitus(volatile uint32_t us)
{
	us *= (SystemCoreClock / 1000000) / 3;
	while(us--);
}

void Audio::PinSetup()
{
	// Enable GPIO peripheral clock
	LPC_CCU1->CLK_M4_GPIO_CFG |= CCU1_CLK_M4_GPIO_CFG_RUN_Msk;
	while(!(LPC_CCU1->CLK_M4_GPIO_STAT & CCU1_CLK_M4_GPIO_STAT_RUN_Msk));

	// P3_0 CODEC_SCK
	scu_pinmux(0x3,  0,  SSP_IO, FUNC0); // I2S0_RX_SCK
	// P3_1 CODEC_LRCK
	scu_pinmux(0x3,  1,  SSP_IO, FUNC1); // I2S0_RX_WS
	// P3_2 IN_SDA1
	scu_pinmux(0x3,  2,  SSP_IO, FUNC1); // I2S0_RX_SDA
	// P3_4 IN_SDA2
	scu_pinmux(0x3,  4,  SSP_IO, FUNC6); // I2S1_RX_SDA
	//// P3_5 CODEC_LRCLK
	//scu_pinmux(0x3,  5,  SSP_IO, FUNC6); // I2S1_RX_WS
	scu_pinmux(0x3, 5, GPIO_PDN, FUNC0); // GPIO1[15], input
	LPC_GPIO_PORT->DIR[3] &= ~(1 << 5);
	// P3_6 OUT_MDI
	scu_pinmux(0x3,  6,  GPIO_PDN, FUNC0); // GPIO0[6], output
	LPC_GPIO_PORT->DIR[0] |= (1 << 6);
	// P3_7 OUT_MC
	scu_pinmux(0x3,  7,  GPIO_PDN, FUNC4); // GPIO5[10], output
	LPC_GPIO_PORT->DIR[5] |= (1 << 10);
	// P3_8 #IN_RESET
	scu_pinmux(0x3,  8,  GPIO_PDN, FUNC4); // GPIO5[11], output
	LPC_GPIO_PORT->CLR[5] |= (1 << 11);
	LPC_GPIO_PORT->DIR[5] |= (1 << 11);
	// P4_0 IN_MONO
	scu_pinmux(0x4,  0,  GPIO_PDN, FUNC0); // GPIO2[0], output
	LPC_GPIO_PORT->DIR[2] |= (1 << 0);
	// P4_3 IN_DIF
	scu_pinmux(0x4,  3,  GPIO_PDN, FUNC0); // GPIO2[3], output
	LPC_GPIO_PORT->DIR[2] |= (1 << 3);
	// P7_0 #OUT_RESET
	scu_pinmux(0x7,  0,  GPIO_PDN, FUNC0); // GPIO3[8], output
	LPC_GPIO_PORT->CLR[3] |= (1 << 8);
	LPC_GPIO_PORT->DIR[3] |= (1 << 8);
	// P7_1 #OUT_MS
	scu_pinmux(0x7,  1,  GPIO_PDN, FUNC0); // GPIO3[9], out
	LPC_GPIO_PORT->SET[3] |= (1 << 9);
	LPC_GPIO_PORT->DIR[3] |= (1 << 9);
	// P7_2 OUT_SDA
	scu_pinmux(0x7,  2,  SSP_IO, FUNC2); // I2S0_TX_SDA
	// P7_3 OUT_MDO
	scu_pinmux(0x7,  3,  GPIO_PDN, FUNC0); // GPIO3[11], in
	LPC_GPIO_PORT->DIR[3] &= ~(1 << 11);
	// P7_4 IN_CKS0
	scu_pinmux(0x7,  4,  GPIO_PDN, FUNC0); // GPIO3[12], out
	LPC_GPIO_PORT->DIR[3] |= (1 << 12);
	// P7_5 IN_CKS1
	scu_pinmux(0x7,  5,  GPIO_PDN, FUNC0); // GPIO3[13], out
	LPC_GPIO_PORT->DIR[3] |= (1 << 13);
	// P7_6 IN_CKS2
	scu_pinmux(0x7,  6,  GPIO_PDN, FUNC0); // GPIO3[14], out
	LPC_GPIO_PORT->DIR[3] |= (1 << 14);
}

void Audio::InMono(bool state)
{
	if (state) {
		LPC_GPIO_PORT->SET[2] |= (1 << 0);
	}
	else {
		LPC_GPIO_PORT->CLR[2] |= (1 << 0);
	}
}

void Audio::InDif(bool state)
{
	if (state) {
		LPC_GPIO_PORT->SET[2] |= (1 << 3);
	}
	else {
		LPC_GPIO_PORT->CLR[2] |= (1 << 3);
	}
}

void Audio::AdcReset()
{
	LPC_GPIO_PORT->CLR[5] |= (1 << 11);
	InMono(false);
	InDif(true);
}

void Audio::OutMs(bool state)
{
	if (state) {
		LPC_GPIO_PORT->CLR[3] |= (1 << 9);
	}
	else {
		LPC_GPIO_PORT->SET[3] |= (1 << 9);
	}
}

void Audio::DacReset()
{
	LPC_GPIO_PORT->CLR[3] |= (1 << 8);
	OutMs(false);
}

void Audio::InCKS0(bool state)
{
	if (state) {
		LPC_GPIO_PORT->SET[3] |= (1 << 12);
	}
	else {
		LPC_GPIO_PORT->CLR[3] |= (1 << 12);
	}
}

void Audio::InCKS1(bool state)
{
	if (state) {
		LPC_GPIO_PORT->SET[3] |= (1 << 13);
	}
	else {
		LPC_GPIO_PORT->CLR[3] |= (1 << 13);
	}
}
void Audio::InCKS2(bool state)
{
	if (state) {
		LPC_GPIO_PORT->SET[3] |= (1 << 14);
	}
	else {
		LPC_GPIO_PORT->CLR[3] |= (1 << 14);
	}
}

void Audio::Clock(ClockMode clockmode)
{
	_clockmode = clockmode;

	// TODO

	switch (clockmode)
	{
	case AUDIO_CLOCK_48000:
		InCKS0(false);
		InCKS1(true);
		InCKS2(true);
		break;
	default:
		while(1);
	}
}

extern "C" uint32_t expectedPLL0AudioFreq;

void Audio::I2SSetup()
{
	CGU_ConfigPWR(CGU_PERIPHERAL_I2S, ENABLE);

	// CGU_SetPLL0_Audio hangs/takes a very very long time to finish; skip it
	//CGU_SetPLL0_Audio(12000000, 24576000, 0.75, 1.25);
	// From user manual UM10503 page 191,
	// Table 151. PLL0AUDIO divider setting for 12 MHz with fractional divider bypassed
	// 12 MHz -> 24.576 MHz, Error: 0
	LPC_CGU->PLL0AUDIO_CTRL |= 1; 	// Power down PLL
	LPC_CGU->PLL0AUDIO_NP_DIV = 0x0003f00e;
	LPC_CGU->PLL0AUDIO_MDIV = 0x000034d3;
	LPC_CGU->PLL0AUDIO_CTRL =(CGU_CLKSRC_XTAL_OSC<<24) | (1<<13) | (1<<4);
	expectedPLL0AudioFreq = 24576000;

	CGU_EnableEntity(CGU_CLKSRC_PLL0_AUDIO, ENABLE);
	CGU_EntityConnect(CGU_CLKSRC_PLL0_AUDIO, CGU_BASE_APB1);
	CGU_UpdateClock();

	I2S_CFG_Type config_i2s = {
			.wordwidth = I2S_WORDWIDTH_32,
			.mono = I2S_STEREO,
			.stop = I2S_STOP_ENABLE,
			.reset = I2S_RESET_ENABLE,
			.ws_sel = I2S_SLAVE_MODE,
			.mute = I2S_MUTE_ENABLE,
			.Reserved0 = {0, 0}
	};
	I2S_MODEConf_Type modeconfig_i2s_tx = {
			.clksel = I2S_CLKSEL_FRDCLK,
			.fpin = I2S_4PIN_ENABLE,
			.mcena = I2S_MCLK_DISABLE,
			.Reserved = 0
	};
	I2S_MODEConf_Type modeconfig_i2s_rx = {
			.clksel = I2S_CLKSEL_FRDCLK,
			.fpin = I2S_4PIN_DISABLE,
			.mcena = I2S_MCLK_DISABLE,
			.Reserved = 0
	};

	I2S_Init(LPC_I2S0);
	I2S_Init(LPC_I2S1);

	I2S_Config(LPC_I2S0, I2S_TX_MODE, &config_i2s);
	I2S_Config(LPC_I2S0, I2S_RX_MODE, &config_i2s);
	I2S_Config(LPC_I2S1, I2S_RX_MODE, &config_i2s);

	I2S_ModeConfig(LPC_I2S0, &modeconfig_i2s_tx, I2S_TX_MODE);
	I2S_ModeConfig(LPC_I2S0, &modeconfig_i2s_rx, I2S_RX_MODE);
	I2S_ModeConfig(LPC_I2S1, &modeconfig_i2s_rx, I2S_RX_MODE);

	// Don't use FreqConfig: it will connect clock source to PLL1 (wrong!)
	// We already connected it to PLL0AUDIO.
	//I2S_FreqConfig(LPC_I2S0, 48000, I2S_TX_MODE);
	//I2S_FreqConfig(LPC_I2S0, 48000, I2S_RX_MODE);
	//I2S_FreqConfig(LPC_I2S1, 48000, I2S_RX_MODE);
	// 2x clock multiplier, I2S peripheral needs 2x clockrate:
	// From user manual UM10503 page 1190, I2S Transmit Clock Rate register
	// I2S_TX_MCLK = PCLK * (X/Y) /2
	// X=2, Y=1
	LPC_I2S0->TXRATE = 1 | (2 << 8);
	LPC_I2S0->RXRATE = 1 | (2 << 8);
	LPC_I2S1->RXRATE = 1 | (2 << 8);

	// From user manual UM10503 page 1190, 43.7.2.2.4 Typical Receiver slave mode
	// "The receive bit rate divider must be set to 1 (RXBITRATE[5:0]=000000) for
	//  this mode to operate correctly."
	I2S_SetBitRate(LPC_I2S0, 63, I2S_TX_MODE);
	I2S_SetBitRate(LPC_I2S0, 0, I2S_RX_MODE);
	I2S_SetBitRate(LPC_I2S1, 0, I2S_RX_MODE);

	// I2S0 interrupt will drive I2S1 also
	I2S_IRQConfig(LPC_I2S0, I2S_RX_MODE, 2);
	I2S_IRQCmd(LPC_I2S0, I2S_RX_MODE, ENABLE);
	I2S_IRQCmd(LPC_I2S0, I2S_TX_MODE, DISABLE);
	I2S_IRQCmd(LPC_I2S1, I2S_RX_MODE, DISABLE);
	I2S_IRQCmd(LPC_I2S1, I2S_TX_MODE, DISABLE);

	NVIC_SetPriority(I2S0_IRQn, 0);
	//NVIC_SetPriority(I2S1_IRQn, 0);

	NVIC_EnableIRQ(I2S0_IRQn);
	//NVIC_EnableIRQ(I2S1_IRQn);

	// Setup done

	I2S_Start(LPC_I2S0);
	I2S_Start(LPC_I2S1);

	NVIC_ClearPendingIRQ(I2S0_IRQn);
}

void Audio::AdcEnable()
{
	LPC_GPIO_PORT->SET[5] |= (1 << 11);
}

void Audio::OutWrite(uint8_t reg, uint8_t value)
{
	// R/#W
	// IDX6..0
	// D7..0
	// TODO
}

uint8_t Audio::OutRead(uint8_t reg)
{
	// R/#W
	// IDX6..0
	// D7..0
	// TODO
	return 0;
}

void Audio::DacEnable()
{
	LPC_GPIO_PORT->SET[3] |= (1 << 8);
}

void Audio::Init()
{
	PinSetup();

	AdcReset();
	DacReset();
	Clock(AUDIO_CLOCK_48000);
	I2SSetup();

	audio_waitus(100);

	// feed the dac
	while (((LPC_I2S0->STATE >> 16) & 0xF) < 8) {
		LPC_I2S0->TXFIFO = 0.0;
		LPC_I2S0->TXFIFO = 0.0;
	}

	AdcEnable();
	DacEnable();
}
