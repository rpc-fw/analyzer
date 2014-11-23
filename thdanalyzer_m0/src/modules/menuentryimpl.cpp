#include <string.h>
#include <algorithm>

#include "menuentryimpl.h"

#include "lcdbuffer.h"
#include "frontpanelstate.h"
#include "ethernet/EthernetHost.h"
#include "analyzerformat.h"

template<typename T, size_t N> T enumselect(const T (&entries)[N], const T current, int delta)
{
	const T* pos = std::find(&entries[0], &entries[N], current);
	size_t index = pos - &entries[0];

	if (delta > 0) {
		while (delta--) {
			index++;
			if (index >= N) {
				index -= N;
			}
		}
	}
	else {
		while (delta++) {
			if (index == 0) {
				index += N;
			}
			index--;
		}
	}

	return entries[index];
}

void SentinelMenuEntry::Render(FrontPanelState* state, LcdBuffer& buffer) const {}
void SentinelMenuEntry::ChangeValue(FrontPanelState* state, int delta) const {}

void DistortionStyleMenuEntry::Render(FrontPanelState* state, LcdBuffer& buffer) const
{
	AnalyzerFormat::Format(state->DistortionFrequency(), state->DistortionLevel(), buffer.row2, state->DistortionFrequencyDisplayMode(), state->DistortionLevelDisplayMode(), state);
}

struct FormatPreset {
	AnalyzerFormat::FrequencyDisplayMode freqmode;
	AnalyzerFormat::LevelDisplayMode levelmode;

	bool operator==(const FormatPreset& other) const {
		return freqmode == other.freqmode && levelmode == other.levelmode;
	}
};

void DistortionStyleMenuEntry::ChangeValue(FrontPanelState* state, int delta) const
{
	const static FormatPreset presets[] = {
			{ AnalyzerFormat::FrequencyDisplayModeHz, AnalyzerFormat::LevelDisplayModeRefRelativeDecibel },
			{ AnalyzerFormat::FrequencyDisplayModeHz, AnalyzerFormat::LevelDisplayModeGeneratorRelativeDecibel },
			{ AnalyzerFormat::FrequencyDisplayModeHz, AnalyzerFormat::LevelDisplayModeGeneratorRelativePercent },
			{ AnalyzerFormat::FrequencyDisplayModeRatio, AnalyzerFormat::LevelDisplayModeRefRelativeDecibel },
			{ AnalyzerFormat::FrequencyDisplayModeRatio, AnalyzerFormat::LevelDisplayModeGeneratorRelativeDecibel },
			{ AnalyzerFormat::FrequencyDisplayModeRatio, AnalyzerFormat::LevelDisplayModeGeneratorRelativePercent }
	};

	FormatPreset cur = { state->DistortionFrequencyDisplayMode(), state->DistortionLevelDisplayMode() };
	FormatPreset p = enumselect(presets, cur, delta);

	state->SetDistortionFrequencyDisplayMode(p.freqmode);
	state->SetDistortionLevelDisplayMode(p.levelmode);
}

void GeneratorStyleMenuEntry::Render(FrontPanelState* state, LcdBuffer& buffer) const
{
	AnalyzerFormat::Format(state->Frequency(), state->Level(), buffer.row2, state->GeneratorFrequencyDisplayMode(), state->GeneratorLevelDisplayMode(), state);
}

void GeneratorStyleMenuEntry::ChangeValue(FrontPanelState* state, int delta) const
{
	const static FormatPreset presets[] = {
			{ AnalyzerFormat::FrequencyDisplayModeHz, AnalyzerFormat::LevelDisplayModeRefRelativeDecibel },
			{ AnalyzerFormat::FrequencyDisplayModeHz, AnalyzerFormat::LevelDisplayModeVoltage},
	};

	FormatPreset cur = { state->GeneratorFrequencyDisplayMode(), state->GeneratorLevelDisplayMode() };
	FormatPreset p = enumselect(presets, cur, delta);

	state->SetGeneratorFrequencyDisplayMode(p.freqmode);
	state->SetGeneratorLevelDisplayMode(p.levelmode);
}

void OperationModeMenuEntry::Render(FrontPanelState* state, LcdBuffer& buffer) const
{
	switch (state->OperationMode()) {
	case FrontPanelState::OperationModeTHD:
		strncpy(buffer.row2, "THD Analysis", buffer.Width());
		break;
	case FrontPanelState::OperationModeFrequencyAnalysis:
		strncpy(buffer.row2, "Freq.Analysis", buffer.Width());
		break;
	}
}

void OperationModeMenuEntry::ChangeValue(FrontPanelState* state, int delta) const
{
	const static enum FrontPanelState::OperationMode presets[] = {
			FrontPanelState::OperationModeTHD,
			FrontPanelState::OperationModeFrequencyAnalysis
	};

	state->SetOperationMode(enumselect(presets, state->OperationMode(), delta));
}

void StringMenuEntry::Render(FrontPanelState* state, LcdBuffer& buffer) const {}
void StringMenuEntry::ChangeValue(FrontPanelState* state, int delta) const {}

void TitleMenuEntry::Render(FrontPanelState* state, LcdBuffer& buffer) const {}
void TitleMenuEntry::ChangeValue(FrontPanelState* state, int delta) const {}

void IPAddressEntry::Render(FrontPanelState* state, LcdBuffer& buffer) const
{
	strncpy(buffer.row2, ethhost.IpAddress(), 16);
}

void IPAddressEntry::ChangeValue(FrontPanelState* state, int delta) const {}

void DHCPNameEntry::Render(FrontPanelState* state, LcdBuffer& buffer) const
{
	strncpy(buffer.row2, ethhost.HostName(), 16);
}
void DHCPNameEntry::ChangeValue(FrontPanelState* state, int delta) const {}

