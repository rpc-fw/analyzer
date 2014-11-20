#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "freertos.h"
#include "task.h"

#include "frontpanel.h"

#include "lcd1602.h"
#include "frontpanelcontrols.h"
#include "analyzercontrol.h"

#include "IpcMailbox.h"
#include "sharedtypes.h"

LCD1602 lcd;
FrontPanelControls frontpanelcontrols;

class FrontPanelState
{
public:
	enum RefLevelMode {
		RefLevel4dBu = 0,
		RefLevel10dBV,
		RefLevelCustom
	};

	FrontPanelState()
	{
		_menu = false;

		_enable = true;
		_needrefresh = true;
		_balancedio = true;

		_frequency = 1000;
		_level = 4;
		_reflevelmode = RefLevel4dBu;
	}

	void SetEnable(bool enable) { _enable = enable; Configure(); Refresh(); }
	bool Enable() const { return _enable; }

	void SetBalancedIO(bool balancedio) { _balancedio = balancedio; _level = ValidateLevel(_level); Configure(); Refresh(); }
	bool BalancedIO() const { return _balancedio; }

	void SetMenu(bool menu) { _menu = menu; Refresh(); }
	bool Menu() const { return _menu; }

	void SetRefLevelMode(RefLevelMode mode) { _reflevelmode = mode; Refresh(); }
	RefLevelMode RefLevelMode() const { return _reflevelmode; }

	void SetCustomRefLevel(float leveldbu) { _refleveldbu = leveldbu; Refresh(); }
	float CustomRefLevel() const { return _refleveldbu; }

	void SetFrequency(float frequency) { _frequency = ValidateFrequency(frequency); Configure(); Refresh(); }
	float Frequency() const { return _frequency; }

	void SetLevel(float level) { _level = ValidateLevel(level); Configure(); Refresh(); }
	float Level() const { return _level; }

	void SetDistortionFrequency(float frequency) { _distortionfrequency = frequency; Refresh(); }
	float DistortionFrequency() const { return _distortionfrequency; }

	void SetDistortionLevel(float level) { _distortionlevel = level; Refresh(); }
	float DistortionLevel() const { return _distortionlevel; }

	bool NeedConfigure() { bool need = _needconfigure; _needconfigure = false; return need; }
	bool NeedRefresh() { bool need = _needrefresh; _needrefresh = false; return need; }

private:
	void Configure() { _needconfigure = true; }
	void Refresh() { _needrefresh = true; }

	float ValidateFrequency(float frequency);
	float ValidateLevel(float level);

	bool _menu;
	bool _needconfigure;
	bool _needrefresh;

	float _frequency;
	float _level;

	float _distortionfrequency;
	float _distortionlevel;

	bool _enable;
	bool _balancedio;

	enum RefLevelMode _reflevelmode;
	float _refleveldbu;
};

float FrontPanelState::ValidateFrequency(float frequency)
{
	if (frequency < 10.0) {
		return 10.0;
	}
	else if (frequency > 23000.0) {
		return 23000.0;
	}
	else if (frequency < 100.0) {
		frequency = 0.01 * round(frequency * 100.0);
	}
	else if (frequency < 1000.0) {
		frequency = 0.1 * round(frequency * 10.0);
	}
	else {
		frequency = round(frequency);
	}

	return frequency;
}

float FrontPanelState::ValidateLevel(float level)
{
	if (_balancedio) {
		if (level < -120.0) {
			return -120.0;
		}
		else if (level > 22.0) {
			return 22.0;
		}
	}
	else {
		if (level < -120.0) {
			return -120.0;
		}
		else if (level > 16.0) {
			return 16.0;
		}
	}

	// else
	level = 0.1 * round(level * 10.0);
	return level;
}

void FrontPanel::Init()
{
	_firstupdate = true;

	_state = new FrontPanelState;

    lcd.Init();
    frontpanelcontrols.Init();
}

void vFrontPanelTask(void* pvParameters)
{
	while(1) {
		frontpanel.Update();

		vTaskDelay(5);
	}
}

void FrontPanel::StartTask()
{
	xTaskCreate(vFrontPanelTask, "frontpanel", 512, NULL, 1 /* priority */, NULL);
}

void FrontPanel::Update()
{
	bool readencoder = false;

	// avoid processing too many events at once to avoid hanging lcd
	int maxupdates = _firstupdate ? 100 : 4;
	for (int i = 0; i < maxupdates; i++) {
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

	if (_firstupdate) {
		analyzercontrol.AnalysisStart();
	}
	else {
		if (analyzercontrol.AnalysisAvailable()) {
			analyzercontrol.AnalysisRead();
			_state->SetDistortionFrequency(*distortionFrequency);
			_state->SetDistortionLevel(*distortionLevel);
			analyzercontrol.AnalysisFinish();
			analyzercontrol.AnalysisStart();
		}
	}

	if (_state->NeedConfigure()) {
		Configure();
	}

	if (_state->NeedRefresh()) {
		RefreshLcd();
	}

	_firstupdate = false;
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
	_state->SetBalancedIO(!_state->BalancedIO());
}

const char* FrontPanel::RelativeLevelString() const
{
	switch (_state->RefLevelMode()) {
	case FrontPanelState::RefLevel4dBu:
		return "dBu";
	case FrontPanelState::RefLevel10dBV:
		return "dBV";
	case FrontPanelState::RefLevelCustom:
		break;
	}
	return "dB";
}

float FrontPanel::RelativeLevel() const
{
	return RelativeLevel(_state->Level());
}

float FrontPanel::RelativeLevelGain() const
{
	switch (_state->RefLevelMode()) {
	default:
	case FrontPanelState::RefLevel4dBu:
		// will return 0
		break;
	case FrontPanelState::RefLevel10dBV:
		return -2.218487499;
	case FrontPanelState::RefLevelCustom:
		return -_state->CustomRefLevel();
	}

	return 0;
}

float FrontPanel::RelativeLevel(float level) const
{
	return level + RelativeLevelGain();
}

void FrontPanel::SetRelativeLevel(float level)
{
	_state->SetLevel(level - RelativeLevelGain());
}

void FrontPanel::LevelReset()
{
	float zero = 0;

	switch (_state->RefLevelMode()) {
	default:
	case FrontPanelState::RefLevel4dBu:
		zero = 4;
		break;
	case FrontPanelState::RefLevel10dBV:
		zero = -10;
		break;
	case FrontPanelState::RefLevelCustom:
		zero = 0;
		break;
	}

	if (RelativeLevel() > zero-0.5 && RelativeLevel() < zero+0.5) {
		SetRelativeLevel(-60.0);
	}
	else if (RelativeLevel() > -60.5 && RelativeLevel() < -59.5) {
		SetRelativeLevel(zero);
	}
	else if (RelativeLevel() <= -30) {
		SetRelativeLevel(-60.0);
	}
	else {
		SetRelativeLevel(zero);
	}
}

void FrontPanel::RefLevel()
{
	switch (_state->RefLevelMode()) {
	case FrontPanelState::RefLevel4dBu:
		_state->SetRefLevelMode(FrontPanelState::RefLevel10dBV);
		break;
	case FrontPanelState::RefLevel10dBV:
		_state->SetRefLevelMode(FrontPanelState::RefLevel4dBu);
		break;
	case FrontPanelState::RefLevelCustom:
		_state->SetRefLevelMode(FrontPanelState::RefLevel4dBu);
		break;
	}
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

void FrontPanel::RefreshLcd()
{
	frontpanelcontrols.SetLed(FrontPanelControls::LedEnable, _state->Enable());
	frontpanelcontrols.SetLed(FrontPanelControls::LedBalancedIO, _state->BalancedIO());

	switch (_state->RefLevelMode()) {
	case FrontPanelState::RefLevel4dBu:
		frontpanelcontrols.SetLed(FrontPanelControls::LedRefLevel4dBu, true);
		frontpanelcontrols.SetLed(FrontPanelControls::LedRefLevel10dBV, false);
		frontpanelcontrols.SetLed(FrontPanelControls::LedRefLevelCustom, false);
		break;
	case FrontPanelState::RefLevel10dBV:
		frontpanelcontrols.SetLed(FrontPanelControls::LedRefLevel4dBu, false);
		frontpanelcontrols.SetLed(FrontPanelControls::LedRefLevel10dBV, true);
		frontpanelcontrols.SetLed(FrontPanelControls::LedRefLevelCustom, false);
		break;
	case FrontPanelState::RefLevelCustom:
		frontpanelcontrols.SetLed(FrontPanelControls::LedRefLevel4dBu, false);
		frontpanelcontrols.SetLed(FrontPanelControls::LedRefLevel10dBV, false);
		frontpanelcontrols.SetLed(FrontPanelControls::LedRefLevelCustom, true);
		break;
	}

	float frequency = _state->Frequency();
	float level = RelativeLevel();

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
		snprintf(text, 9, "% 5.0f%s", level, RelativeLevelString());
	}
	else {
		snprintf(text, 9, "% 5.1f%s", level, RelativeLevelString());
	}
	text[16] = '\0';
	lcd.Locate(8, 0);
	lcd.Print(text);


	frequency = _state->DistortionFrequency();
	level = RelativeLevel(_state->DistortionLevel());
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
		snprintf(text, 9, "% 5.0f%s", level, RelativeLevelString());
	}
	else {
		snprintf(text, 9, "% 5.1f%s", level, RelativeLevelString());
	}
	text[16] = '\0';
	lcd.Locate(8, 1);
	lcd.Print(text);

}

void FrontPanel::Configure()
{
	GeneratorParameters currentparams;
	currentparams.balancedio = _state->BalancedIO();
	currentparams.frequency = _state->Frequency();
	if (currentparams.balancedio) {
		currentparams.level = _state->Enable() ? _state->Level() : -160.0;
	}
	else {
		// unbalanced I/O, add 6dB of gain
		currentparams.level = _state->Enable() ? (_state->Level() + 6.0) : -160.0;
	}
	analyzercontrol.SetConfiguration(currentparams);
}
