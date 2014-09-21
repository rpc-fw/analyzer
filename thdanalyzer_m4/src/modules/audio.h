#pragma once

class Audio
{
public:
	void Init();

private:
	void PinSetup();
	void InMono(bool state);
	void InDif(bool state);
	void InCKS0(bool state);
	void InCKS1(bool state);
	void InCKS2(bool state);
	void OutMs(bool state);
	void OutWrite(uint8_t reg, uint8_t value);
	uint8_t OutRead(uint8_t reg);

	void AdcReset();
	void DacReset();

	void I2SSetup();
	void AdcEnable();
	void DacEnable();

	enum ClockMode {
		AUDIO_CLOCK_48000 = 0
	};
	void Clock(ClockMode clockmode);

	ClockMode _clockmode;
};
