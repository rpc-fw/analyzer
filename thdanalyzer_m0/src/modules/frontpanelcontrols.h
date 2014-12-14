#ifndef FRONTPANELCONTROLS_H_
#define FRONTPANELCONTROLS_H_

#include "fronti2c.h"

class FrontPanelControls
{
public:
	enum Button {
		ButtonEncoder = -2,
		ButtonNone = -1,
		ButtonAuto = 0,
		ButtonMenu,
		ButtonBandwidthLimit,
		ButtonBalancedIO,
		ButtonUp,
		ButtonDown,
		ButtonLevelReset,
		ButtonRefLevel,
		ButtonCustomLevel,
		ButtonEnable
	};

	enum Led {
		LedNone = -1,
		LedAuto = 0,
		LedBandwidthLimit,
		LedBalancedIO,
		LedRefLevel4dBu,
		LedRefLevel10dBV,
		LedRefLevelCustom,
		LedEnable
	};

	enum Encoder {
		EncoderNone = -1,
		EncoderGain = 0,
		EncoderFrequency
	};

	FrontPanelControls();

	void Init();

	// button events one at a time
	Button ReadNextButton(bool& pressed);

	// set a led
	void SetLed(Led ledid, bool state);

	// get encoder rotation
	int ReadEncoderDelta(Encoder encoderid);

private:
	int LedToId(Led led) const;
	Button IdToButton(int buttonid) const;
	int EncoderToId(Encoder encoder) const;
	int GetEncoderDelta(Encoder encoderid) const;

	void RequestAll();

	FrontI2C _i2c;

	int32_t _encoderdelta[2];
	bool _ledstate[7];
};

#endif /* FRONTPANELCONTROLS_H_ */
