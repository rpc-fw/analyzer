#include <stdio.h>
#include <string.h>
#include <math.h>

#include "analyzerformat.h"
#include "frontpanelstate.h"

void RenderFrequencyHz(char* text, int len, float frequency)
{
	if (frequency < 100) {
		snprintf(text, len, "%1.2fHz  ", frequency);
	}
	else if (frequency < 1000) {
		snprintf(text, len, "%1.1fHz  ", frequency);
	}
	else if (frequency < 10000) {
		snprintf(text, len, "%1.0f Hz  ", frequency);
	}
	else {
		snprintf(text, len, "%1.0fHz  ", frequency);
	}
	text[len] = '\0';
}

void RenderFrequencyRatio(char* text, int len, float frequency, float reference)
{
	float ratio = frequency / reference;
	float nearest = roundf(ratio);
	if (fabsf(ratio - nearest) < 0.05) {
		snprintf(text, len, " %3.0f:1", nearest);
		return;
	}
	else if (ratio >= 1) {
		snprintf(text, len, "%2.2f:1", ratio);
		return;
	}

	ratio = reference / frequency;
	nearest = roundf(ratio);
	if (fabsf(ratio - nearest) < 0.05) {
		snprintf(text, len, "   1:%2.0f", nearest);
		return;
	}

	// else
	RenderFrequencyHz(text, len, frequency);
}

void RenderLevelRefDecibel(char* text, int len, float level, const char* reflevelstring)
{
	if (level <= -100.0) {
		snprintf(text, len, "% 5.0f%s", level, reflevelstring);
	}
	else {
		snprintf(text, len, "% 5.1f%s", level, reflevelstring);
	}
}

void RenderRelativeDecibel(char* text, int len, float level)
{
	snprintf(text, len, "% 6.1fdB", level);
}

void RenderRelativePercent(char* text, int len, float level)
{
	const float ln10 = 2.30258509299;
	float p = 100.0 * expf(level * (1.0 / 20.0) * ln10);

	if (p >= 1000.0) {
		snprintf(text, len, "%7.1f%%", p);
	}
	else if (p >= 100.0) {
		snprintf(text, len, "%7.1f%%", p);
	}
	else if (p >= 10.0) {
		snprintf(text, len, "%7.1f%%", p);
	}
	else if (p >= 1.0) {
		snprintf(text, len, "%7.2f%%", p);
	}
	else if (p >= 0.1) {
		snprintf(text, len, "%7.3f%%", p);
	}
	else if (p >= 0.01) {
		snprintf(text, len, "%7.4f%%", p);
	}
	else {
		snprintf(text, len, "%7.5f%%", p);
	}
}

void RenderLevelVoltagePeakToPeak(char* text, int len, float level)
{
	const float ln10 = 2.30258509299;
	float v = 2.19089023 * expf(level * (1.0 / 20.0) * ln10);

	if (v >= 4.995) {
		snprintf(text, len, "%5.2fVpp", v);
		return;
	}
	else if (v >= 0.0995) {
		snprintf(text, len, "%5.3fVpp", v);
		return;
	}
	else if (v >= 0.004995) {
		snprintf(text, len, "%4.1fmVpp", v * 1000.0);
		return;
	}
	else if (v >= 0.0009995) {
		snprintf(text, len, "%4.2fmVpp", v * 1000.0);
		return;
	}
	else if (v >= 0.00009995) {
		snprintf(text, len, "%4.0fuVpp", v * 1000000.0);
		return;
	}
	else if (v >= 0.000004995) {
		snprintf(text, len, "%4.1fuVpp", v * 1000000.0);
		return;
	}
	// else
	snprintf(text, len, "%4.2fuVpp", v * 1000000.0);
}

void AnalyzerFormat::Format(float frequency, float leveldBu, char* buffer, FrequencyDisplayMode freqmode, LevelDisplayMode levelmode, FrontPanelState *state)
{
	memset(buffer, 0, 16);

	switch (freqmode) {
	case FrequencyDisplayModeHz:
		RenderFrequencyHz(&buffer[0], 9, frequency);
		break;
	case FrequencyDisplayModeRatio:
		RenderFrequencyRatio(&buffer[0], 9, frequency, state->Frequency());
		break;
	}

	switch (levelmode) {
	case LevelDisplayModeRefRelativeDecibel:
		RenderLevelRefDecibel(&buffer[8], 9, state->RelativeLevel(leveldBu), state->RelativeLevelString());
		break;
	case LevelDisplayModeGeneratorRelativeDecibel:
		RenderRelativeDecibel(&buffer[8], 9, leveldBu - state->Level());
		break;
	case LevelDisplayModeGeneratorRelativePercent:
		RenderRelativePercent(&buffer[8], 9, leveldBu - state->Level());
		break;
	case LevelDisplayModeVoltage:
		RenderLevelVoltagePeakToPeak(&buffer[8], 9, leveldBu);
		break;
	}

	for (int i = 0; i < 16; i++) {
		if (buffer[i] == '\0') {
			buffer[i] = ' ';
		}
	}
	buffer[16] = '\0';
}

void AnalyzerFormat::FormatVoltage(const char* name, float voltage, char* buffer)
{
	memset(buffer, 0, 16);

	// avoid "-0.000"
	if (fabs(voltage) < 0.0005) {
		voltage = 0.0;
	}

	snprintf(&buffer[0], 16, "%s % 3.3f V", name, voltage);
	for (int i = 0; i < 16; i++) {
		if (buffer[i] == '\0') {
			buffer[i] = ' ';
		}
	}

	buffer[16] = '\0';
}
