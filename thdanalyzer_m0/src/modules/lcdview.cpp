#include <stdio.h>

#include "lcdview.h"
#include "frontpanelstate.h"

#include "lcd1602.h"
#include "lcdbuffer.h"
#include "analyzerformat.h"

LCD1602 lcd;
LcdView lcdview;

LcdView::LcdView()
{

}

void LcdView::Init(FrontPanelState* state)
{
	_state = state;
	lcd.Init();
}

void FillSpaces(char* buffer, int size)
{
	for (int i = 0; i < size; i++) {
		if (buffer[i] == '\0') {
			while(i < size) {
				buffer[i] = ' ';
				i++;
			}
		}
	}
	buffer[size] = '\0';
}

void LcdView::RenderLevelFrequency()
{
	char text[17];
	AnalyzerFormat::Format(_state->Frequency(), _state->Level(), text, _state->GeneratorFrequencyDisplayMode(), _state->GeneratorLevelDisplayMode(), _state);
	lcd.Locate(0, 0);
	lcd.Print(text);

	AnalyzerFormat::Format(_state->DistortionFrequency(), _state->DistortionLevel(), text, _state->DistortionFrequencyDisplayMode(), _state->DistortionLevelDisplayMode(), _state);
	lcd.Locate(0, 1);
	lcd.Print(text);
}

void LcdView::RenderCV()
{
	char text[17];
	AnalyzerFormat::FormatVoltage("CV1", _state->Cv0(), text);
	lcd.Locate(0, 0);
	lcd.Print(text);

	AnalyzerFormat::FormatVoltage("CV2", _state->Cv1(), text);
	lcd.Locate(0, 1);
	lcd.Print(text);
}

void LcdView::Refresh()
{
	if (_state->MenuActive()) {
		LcdBuffer buffer;
		menu.MenuRender(_state->MenuEntry(), buffer);

		FillSpaces(buffer.row1, buffer.Width());
		FillSpaces(buffer.row2, buffer.Width());

		if (menu.IsSubmenu(_state->MenuEntry())) {
			buffer.row1[15] = '\1';
			buffer.row1[16] = '\0';
		}

		lcd.Locate(0, 0);
		lcd.Print(buffer.row1);
		lcd.Locate(0, 1);
		lcd.Print(buffer.row2);

		return;
	}

	switch (_state->OperationMode())
	{
	case FrontPanelState::OperationModeTHD:
	case FrontPanelState::OperationModeFrequencyAnalysis:
		RenderLevelFrequency();
		break;
	case FrontPanelState::OperationModeDCVoltageControl:
		RenderCV();
		break;
	default:
		break;
	}
}
