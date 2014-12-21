#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "freertos.h"
#include "task.h"

#include "frontpanel.h"

#include "frontpanelcontrols.h"
#include "analyzercontrol.h"
#include "lcdview.h"

#include "IpcMailbox.h"
#include "sharedtypes.h"

FrontPanelControls frontpanelcontrols;

#include "frontpanelstate.h"

const int gain_accel_scale = 100;
const int gain_accel_threshold = 4;
const int freq_accel_scale = 100;
const int freq_accel_threshold = 3;

static int AccelerateGain(int delta)
{
#define NUM_MEASUREMENTS 4
	static int counts[NUM_MEASUREMENTS] = {0};
	static uint32_t times[NUM_MEASUREMENTS] = {0};
	static uint32_t lasttime = 0;

	uint32_t curtime = xTaskGetTickCount();
	int timedelta = curtime - lasttime;
	lasttime = curtime;

	for (int i = 0; i < NUM_MEASUREMENTS - 1; i++) {
		counts[i] = counts[i+1];
		times[i] = times[i+1];
	}

	counts[NUM_MEASUREMENTS - 1] = delta;
	times[NUM_MEASUREMENTS - 1] = timedelta;

	int totaltime = 0;
	int totaldelta = 0;
	for (int i = NUM_MEASUREMENTS - 1; i >= 0; i--) {
		totaldelta += counts[i];
		totaltime += times[i];
		if (totaltime > 3*gain_accel_scale) {
			break;
		}
	}

	int weight = abs(totaldelta);

	if (weight > gain_accel_threshold) {
		return delta * (weight / gain_accel_threshold);
	}

	return delta;
}

static int AccelerateFrequency(int delta)
{
	static int counts[NUM_MEASUREMENTS] = {0};
	static uint32_t times[NUM_MEASUREMENTS] = {0};
	static uint32_t lasttime = 0;

	uint32_t curtime = xTaskGetTickCount();
	int timedelta = curtime - lasttime;
	lasttime = curtime;

	for (int i = 0; i < NUM_MEASUREMENTS - 1; i++) {
		counts[i] = counts[i+1];
		times[i] = times[i+1];
	}

	counts[NUM_MEASUREMENTS - 1] = delta;
	times[NUM_MEASUREMENTS - 1] = timedelta;

	int totaltime = 0;
	int totaldelta = 0;
	for (int i = NUM_MEASUREMENTS - 1; i >= 0; i--) {
		totaldelta += counts[i];
		totaltime += times[i];
		if (totaltime > 3*gain_accel_scale) {
			break;
		}
	}

	int weight = abs(totaldelta);

	if (weight > freq_accel_threshold) {
		return delta * (weight / freq_accel_threshold);
	}

	return delta;
}

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

	menu.Init(_state);
    lcdview.Init(_state);
}

void vFrontPanelTask(void* pvParameters)
{
    frontpanelcontrols.Init();

	while(1) {
		frontpanel.Update();

		vTaskDelay(5);
	}
}

void FrontPanel::StartTask()
{
	xTaskCreate(vFrontPanelTask, "frontpanel", 512, NULL, 2 /* priority */, NULL);
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
				if (_state->MenuActive()) {
					Cancel();
				}
				else {
					Auto();
				}
			}
			break;
		case FrontPanelControls::ButtonMenu:
			if (pressed) {
				if (_state->MenuActive()) {
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
				if (_state->MenuActive()) {
					MoveMenuAdjust(-1);
				}
				else {
					FrequencyUp();
				}
			}
			break;
		case FrontPanelControls::ButtonDown:
			if (pressed) {
				if (_state->MenuActive()) {
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

		if (_state->MenuActive()) {
			if (gaindelta) {
				MoveMenuSelect(gaindelta);
			}
			if (freqdelta) {
				MoveMenuAdjust(freqdelta);
			}
		}
		else {
			if (gaindelta) {
				gaindelta = AccelerateGain(gaindelta);
				MoveGain(gaindelta);
			}

			if (freqdelta) {
				freqdelta = AccelerateFrequency(freqdelta);
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
		RefreshLeds();
		lcdview.Refresh();
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
	_state->SetMenuEntry(0);
	_state->SetMenuActive(true);
}

void FrontPanel::BandwidthLimit()
{

}

void FrontPanel::BalancedIO()
{
	_state->SetBalancedIO(!_state->BalancedIO());
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

	if (_state->RelativeLevel() > zero-0.5 && _state->RelativeLevel() < zero+0.5) {
		_state->SetRelativeLevel(-60.0);
	}
	else if (_state->RelativeLevel() > -60.5 && _state->RelativeLevel() < -59.5) {
		_state->SetRelativeLevel(zero);
	}
	else if (_state->RelativeLevel() <= -30) {
		_state->SetRelativeLevel(-60.0);
	}
	else {
		_state->SetRelativeLevel(zero);
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
	if (delta > 0) {
		while (delta--) {
			_state->SetMenuEntry(menu.Next(_state->MenuEntry()));
		}
	}
	else if (delta < 0) {
		while (delta++) {
			_state->SetMenuEntry(menu.Prev(_state->MenuEntry()));
		}
	}
}

void FrontPanel::MoveMenuAdjust(int32_t delta)
{
	menu.MenuValueChange(_state->MenuEntry(), delta);
}

void FrontPanel::Ok()
{
	if (menu.IsSubmenu(_state->MenuEntry())) {
		_state->SetMenuEntry(menu.EnterSubmenu(_state->MenuEntry()));
	}
	else {
		_state->SetMenuActive(false);
	}
}

void FrontPanel::Cancel()
{
	if (menu.IsChild(_state->MenuEntry())) {
		_state->SetMenuEntry(menu.Back(_state->MenuEntry()));
	}
	_state->SetMenuActive(false);
}

void FrontPanel::MoveGain(int32_t delta)
{
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

void FrontPanel::RefreshLeds()
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
}

void FrontPanel::Configure()
{
	GeneratorParameters currentparams;
	currentparams._balancedio = _state->BalancedIO();
	currentparams._frequency = _state->Frequency();
	if (currentparams._balancedio) {
		currentparams._level = _state->Enable() ? _state->Level() : -160.0;
	}
	else {
		// unbalanced I/O, add 6dB of gain
		currentparams._level = _state->Enable() ? (_state->Level() + 6.0) : -160.0;
	}
	currentparams._analysismode = _state->OperationMode() == FrontPanelState::OperationModeFrequencyAnalysis;
	analyzercontrol.SetConfiguration(currentparams);
}
