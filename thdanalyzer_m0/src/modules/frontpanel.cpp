#include <stdint.h>
#include <stdio.h>

#include "frontpanel.h"

#include "lcd1602.h"
#include "frontpanelcontrols.h"

#include "IpcMailbox.h"
#include "sharedtypes.h"

LCD1602 lcd;
FrontPanelControls frontpanelcontrols;

class FrontPanelState
{
public:
	FrontPanelState()
	{
		_menu = false;

		_enable = true;
		_needrefresh = true;

		_frequency = 1000;
		_level = 4;
	}

	void SetEnable(bool enable) { _enable = enable; Refresh(); }
	bool Enable() const { return _enable; }

	void SetMenu(bool menu) { _menu = menu; Refresh(); }
	bool Menu() const { return _menu; }

	void SetFrequency(float frequency) { _frequency = frequency; Refresh(); }
	float Frequency() const { return _frequency; }

	void SetLevel(float level) { _level = level; Refresh(); }
	float Level() const { return _level; }

	bool NeedRefresh() { bool need = _needrefresh; _needrefresh = false; return need; }

private:
	void Refresh() { _needrefresh = true; }

	bool _menu;
	bool _needrefresh;

	float _frequency;
	float _level;

	bool _enable;
};

void FrontPanel::Init()
{
	_state = new FrontPanelState;

    lcd.Init();
    frontpanelcontrols.Init();
}

void FrontPanel::Update()
{
	bool readencoder = false;

	while (true) {
		bool pressed;
		FrontPanelControls::Button b = frontpanelcontrols.ReadNextButton(pressed);
		if (b == FrontPanelControls::ButtonNone) {
			// out of events
			break;
		}

		switch (b) {
		case FrontPanelControls::ButtonEncoder:
			readencoder = true;
			break;
		case FrontPanelControls::ButtonEnable:
			_state->SetEnable(pressed);
			break;
		case FrontPanelControls::ButtonAuto:
			if (pressed) {
				if (_state->Menu()) {
					Cancel();
				}
				else {
					Auto();
				}
			}
			break;
		case FrontPanelControls::ButtonMenu:
			if (pressed) {
				if (_state->Menu()) {
					Ok();
				}
				else {
					Menu();
				}
			}
			break;
		case FrontPanelControls::ButtonBandwidthLimit:
			if (pressed) {
				BandwidthLimit();
			}
			break;
		case FrontPanelControls::ButtonBalancedIO:
			if (pressed) {
				BalancedIO();
			}
			break;
		case FrontPanelControls::ButtonUp:
			if (pressed) {
				if (_state->Menu()) {
					MoveMenuAdjust(-1);
				}
				else {
					FrequencyUp();
				}
			}
			break;
		case FrontPanelControls::ButtonDown:
			if (pressed) {
				if (_state->Menu()) {
					MoveMenuAdjust(1);
				}
				else {
					FrequencyDown();
				}
			}
			break;
		case FrontPanelControls::ButtonLevelReset:
			if (pressed) {
				LevelReset();
			}
			break;
		case FrontPanelControls::ButtonRefLevel:
			if (pressed) {
				RefLevel();
			}
			break;
		case FrontPanelControls::ButtonCustomLevel:
			if (pressed) {
				CustomLevel();
			}
			break;
		default:
			break;
		}
	}

	if (readencoder) {
		int32_t gaindelta = frontpanelcontrols.ReadEncoderDelta(FrontPanelControls::EncoderGain);
		int32_t freqdelta = frontpanelcontrols.ReadEncoderDelta(FrontPanelControls::EncoderFrequency);

		if (_state->Menu()) {
			if (gaindelta) {
				MoveMenuSelect(gaindelta);
			}
			if (freqdelta) {
				MoveMenuAdjust(freqdelta);
			}
		}
		else {
			if (gaindelta) {
				MoveGain(gaindelta);
			}

			if (freqdelta) {
				MoveFrequency(freqdelta);
			}
		}
	}

	if (_state->NeedRefresh()) {
		RefreshLcd();
	}
}

void FrontPanel::SetFrequency(float frequency)
{
	_state->SetFrequency(frequency);
}

void FrontPanel::SetLevel(float level)
{
	_state->SetLevel(level);
}

void FrontPanel::Auto()
{

}

void FrontPanel::Menu()
{

}

void FrontPanel::BandwidthLimit()
{

}

void FrontPanel::BalancedIO()
{

}

void FrontPanel::LevelReset()
{

}

void FrontPanel::RefLevel()
{

}

void FrontPanel::CustomLevel()
{

}

void FrontPanel::MoveMenuSelect(int32_t delta)
{

}

void FrontPanel::MoveMenuAdjust(int32_t delta)
{

}

void FrontPanel::Ok()
{

}

void FrontPanel::Cancel()
{

}

void FrontPanel::MoveGain(int32_t delta)
{
	if (delta <= -11) {
		delta = (delta + 8) * 4;
	}
	else if (delta >= 11) {
		delta = (delta - 8) * 4;
	}

	float level = _state->Level();
	if (level <= -100.0) {
		_state->SetLevel(level + float(delta));
	}
	else {
		_state->SetLevel(level + float(delta) * 0.1);
	}
}

void FrontPanel::MoveFrequency(int32_t delta)
{
	float frequency = _state->Frequency();
	if (frequency < 100.0) {
		frequency = frequency + float(delta) * 0.01;
	}
	else if (frequency < 1000.0) {
		frequency = frequency + float(delta) * 0.1;
	}
	else {
		frequency = frequency + float(delta);
	}

	_state->SetFrequency(frequency);
}

void FrontPanel::FrequencyUp()
{
	float frequency = _state->Frequency();
	if (frequency >= 10000.0) {
		return;
	}
	_state->SetFrequency(frequency * 2.0);
}

void FrontPanel::FrequencyDown()
{
	float frequency = _state->Frequency();
	if (frequency < 20.0) {
		return;
	}
	_state->SetFrequency(frequency * 0.5);
}

void FrontPanel::ValidateParams()
{
	float frequency = _state->Frequency();
	if (frequency < 10.0) {
		_state->SetFrequency(10.0);
	}
	if (frequency > 23000.0) {
		_state->SetFrequency(23000.0);
	}

	float level = _state->Level();
	if (level < -120.0) {
		_state->SetLevel(-120.0);
	}
	if (level > 20.0) {
		_state->SetLevel(20.0);
	}
}

void FrontPanel::RefreshLcd()
{
	ValidateParams();

	frontpanelcontrols.SetLed(FrontPanelControls::LedEnable, _state->Enable());

	float frequency = _state->Frequency();
	float level = _state->Level();

	GeneratorParameters currentparams;
	currentparams.frequency = frequency;

	currentparams.level = _state->Enable() ? level : -160.0;

	commandMailbox.Write(currentparams);
	while (!ackMailbox.Read());

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
		snprintf(text, 9, "% 5.0fdBu", level);
	}
	else {
		snprintf(text, 9, "% 5.1fdBu", level);
	}
	text[16] = '\0';
	lcd.Locate(8, 0);
	lcd.Print(text);
}
