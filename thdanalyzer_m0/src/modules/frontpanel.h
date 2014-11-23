#ifndef FRONTPANEL_H_
#define FRONTPANEL_H_

#include "frontpanelcontrols.h"

class FrontPanelState;

class FrontPanel
{
public:
	FrontPanel() : _state(0) {}

	void Init();
	void StartTask();

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
	void RefreshLeds();

	void Configure();

	float RelativeLevelGain() const;
	const char* RelativeLevelString() const;
	float RelativeLevel() const;
	float RelativeLevel(float level) const;
	void SetRelativeLevel(float level);

	FrontPanelState* _state;
	bool _firstupdate;
	bool _needconfigure;
};

extern FrontPanel frontpanel;

#endif /* FRONTPANEL_H_ */
