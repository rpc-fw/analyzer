#ifndef ANALYZERFORMAT_H_
#define ANALYZERFORMAT_H_

class FrontPanelState;

class AnalyzerFormat
{
public:
	enum FrequencyDisplayMode {
		FrequencyDisplayModeHz,
		FrequencyDisplayModeRatio
	};

	enum LevelDisplayMode {
		LevelDisplayModeVoltage,
		LevelDisplayModeRefRelativeDecibel,
		LevelDisplayModeGeneratorRelativeDecibel,
		LevelDisplayModeGeneratorRelativePercent
	};

	static void Format(float frequency, float leveldBu, char* buffer, FrequencyDisplayMode freqmode, LevelDisplayMode levelmode, FrontPanelState *state);

	static void FormatVoltage(const char* name, float voltage, char* buffer);
};

#endif /* ANALYZERFORMAT_H_ */
