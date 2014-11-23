#include <stdio.h>
#include <string.h>

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

void RenderLevelRefDecibel(char* text, int len, float level, const char* reflevelstring)
{
	if (level <= -100.0) {
		snprintf(text, len, "% 5.0f%s", level, reflevelstring);
	}
	else {
		snprintf(text, len, "% 5.1f%s", level, reflevelstring);
	}
}

void AnalyzerFormat::Format(float frequency, float leveldBu, char* buffer, FrequencyDisplayMode freqmode, LevelDisplayMode levelmode, FrontPanelState *state)
{
	memset(buffer, 0, 16);

	switch (freqmode) {
	case FrequencyDisplayModeHz:
		RenderFrequencyHz(&buffer[0], 9, frequency);
		break;
	}

	switch (levelmode) {
	case LevelDisplayModeRefRelativeDecibel:
		RenderLevelRefDecibel(&buffer[8], 9, state->RelativeLevel(leveldBu), state->RelativeLevelString());
	}

	for (int i = 0; i < 16; i++) {
		if (buffer[i] == '\0') {
			buffer[i] = ' ';
		}
	}
	buffer[16] = '\0';
}
