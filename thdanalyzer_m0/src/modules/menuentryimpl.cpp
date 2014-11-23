#include <string.h>

#include "menuentryimpl.h"

#include "lcdbuffer.h"
#include "frontpanelstate.h"
#include "ethernet/EthernetHost.h"
#include "analyzerformat.h"

void SentinelMenuEntry::Render(FrontPanelState* state, LcdBuffer& buffer) const {}
void SentinelMenuEntry::ChangeValue(FrontPanelState* state, int delta) const {}

void DistortionStyleMenuEntry::Render(FrontPanelState* state, LcdBuffer& buffer) const
{
	AnalyzerFormat::Format(state->DistortionFrequency(), state->DistortionLevel(), buffer.row2, AnalyzerFormat::FrequencyDisplayModeHz, AnalyzerFormat::LevelDisplayModeRefRelativeDecibel, state);
}
void DistortionStyleMenuEntry::ChangeValue(FrontPanelState* state, int delta) const {}

void GeneratorStyleMenuEntry::Render(FrontPanelState* state, LcdBuffer& buffer) const
{
	AnalyzerFormat::Format(state->Frequency(), state->Level(), buffer.row2, AnalyzerFormat::FrequencyDisplayModeHz, AnalyzerFormat::LevelDisplayModeRefRelativeDecibel, state);
}
void GeneratorStyleMenuEntry::ChangeValue(FrontPanelState* state, int delta) const {}

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

