#include <stdio.h>

#include "lcdview.h"
#include "frontpanelstate.h"

#include "lcd1602.h"

LCD1602 lcd;

LcdView::LcdView()
{

}

void LcdView::Init()
{
	lcd.Init();
}

void LcdView::Refresh(FrontPanelState* state)
{
	SetState(state);

	float frequency = _state->Frequency();
	float level = _state->RelativeLevel();

	char text[17];
	if (frequency < 100) {
		snprintf(text, 16, "%1.2fHz  ", frequency);
	}
	else if (frequency < 1000) {
		snprintf(text, 16, "%1.1fHz  ", frequency);
	}
	else if (frequency < 10000) {
		snprintf(text, 16, "%1.0f Hz  ", frequency);
	}
	else {
		snprintf(text, 16, "%1.0fHz  ", frequency);
	}
	text[16] = '\0';
	lcd.Locate(0, 0);
	lcd.Print(text);

	if (level <= -100.0) {
		snprintf(text, 9, "% 5.0f%s", level, _state->RelativeLevelString());
	}
	else {
		snprintf(text, 9, "% 5.1f%s", level, _state->RelativeLevelString());
	}
	text[16] = '\0';
	lcd.Locate(8, 0);
	lcd.Print(text);

	frequency = _state->DistortionFrequency();
	level = _state->RelativeLevel(_state->DistortionLevel());
	if (frequency < 100) {
		snprintf(text, 16, "%1.2fHz  ", frequency);
	}
	else if (frequency < 1000) {
		snprintf(text, 16, "%1.1fHz  ", frequency);
	}
	else if (frequency < 10000) {
		snprintf(text, 16, "%1.0f Hz  ", frequency);
	}
	else {
		snprintf(text, 16, "%1.0fHz  ", frequency);
	}
	text[16] = '\0';
	lcd.Locate(0, 1);
	lcd.Print(text);

	if (level <= -100.0) {
		snprintf(text, 9, "% 5.0f%s", level, _state->RelativeLevelString());
	}
	else {
		snprintf(text, 9, "% 5.1f%s", level, _state->RelativeLevelString());
	}
	text[16] = '\0';
	lcd.Locate(8, 1);
	lcd.Print(text);

	SetState(NULL);
}
