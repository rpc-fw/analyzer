#pragma once

class Audio
{
public:
	void Init();

	int SampleRate() const
	{
		switch (_clockmode) {
		case AUDIO_CLOCK_192000:
			return 192000;
		case AUDIO_CLOCK_96000:
			return 96000;
		case AUDIO_CLOCK_48000:
		default:
			return 48000;
			break;
		}
	}

	float SampleRateFloat() const
	{
		// Assume inlining
		return SampleRate();
	}

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
		AUDIO_CLOCK_48000 = 0,
		AUDIO_CLOCK_96000,
		AUDIO_CLOCK_192000
	};
	void Clock(ClockMode clockmode);

	ClockMode _clockmode;
};

extern Audio audio;
