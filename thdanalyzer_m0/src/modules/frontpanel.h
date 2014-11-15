#ifndef FRONTPANEL_H_
#define FRONTPANEL_H_

#include "frontpanelcontrols.h"

class FrontPanelState;

class FrontPanel
{
public:
	FrontPanel() : _state(0) {}

	void Init();

	void Update();

	void SetFrequency(float frequency);
	void SetLevel(float level);

private:
	void Auto();
	void Menu();

	void BandwidthLimit();
	void BalancedIO();

	void LevelReset();
	void RefLevel();
	void CustomLevel();

	void MoveMenuSelect(int32_t delta);
	void MoveMenuAdjust(int32_t delta);
	void Ok();
	void Cancel();

	void MoveGain(int32_t delta);
	void MoveFrequency(int32_t delta);
	void FrequencyUp();
	void FrequencyDown();

	void ValidateParams();
	void RefreshLcd();

	FrontPanelState* _state;
};


#endif /* FRONTPANEL_H_ */
