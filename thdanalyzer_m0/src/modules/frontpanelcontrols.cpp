
#include "frontpanelcontrols.h"

FrontPanelControls::FrontPanelControls()
{

}

void FrontPanelControls::Init()
{
	_i2c.Init();

	RequestAll();
}

FrontPanelControls::Button FrontPanelControls::IdToButton(int buttonid) const
{
	switch (buttonid) {
	case 0:
		return ButtonAuto;
	case 1:
		return ButtonMenu;
	case 2:
		return ButtonBandwidthLimit;
	case 3:
		return ButtonBalancedIO;
	case 4:
		return ButtonUp;
	case 5:
		return ButtonDown;
	case 6:
		return ButtonLevelReset;
	case 7:
		return ButtonRefLevel;
	case 8:
		return ButtonCustomLevel;
	case 9:
		return ButtonEnable;
	default:
		break;
	}

	return ButtonNone;
}

// button events one at a time
FrontPanelControls::Button FrontPanelControls::ReadNextButton(bool& pressed)
{
	uint8_t data;
	_i2c.Read(0x10, &data, 1);

    pressed = ((data >> 7) & 1) != 0;
    int id = data & 0x7f;

    if (data != 0xff) {
    	// button
    	if (!(data & 0x40)) {
    		FrontPanelControls::Button button = IdToButton(id);

    		// Enable switch is inverted
    		if (button == ButtonEnable) {
    			pressed = !pressed;
    		}

    		return button;
		}
		else {
			// encoder
			int encoderid = data & 0x80 ? 1 : 0;
			_encoderdelta[encoderid] += (data & 0x3f) - 16;
			return ButtonEncoder;
		}
	}

    return ButtonNone;
}

int FrontPanelControls::LedToId(FrontPanelControls::Led led) const
{
	switch (led) {
	case LedAuto:
		return 0;
	case LedBandwidthLimit:
		return 1;
	case LedBalancedIO:
		return 2;
	case LedRefLevelCustom:
		return 3;
	case LedRefLevel10dBV:
		return 4;
	case LedRefLevel4dBu:
		return 5;
	case LedEnable:
		return 6;
	default:
		break;
	}

	return 0;
}

// set a led
void FrontPanelControls::SetLed(FrontPanelControls::Led led, bool state)
{
	int ledid = LedToId(led);

	if (state == _ledstate[ledid]) {
		return;
	}

	uint8_t buffer[2];
	buffer[0] = 0x40;
	buffer[1] = uint8_t(ledid) | (state ? 0x80 : 0);

	_i2c.Write(0x10, buffer, sizeof(buffer));

	_ledstate[ledid] = state;
}

int FrontPanelControls::EncoderToId(FrontPanelControls::Encoder encoder) const
{
	switch (encoder) {
	case EncoderGain:
		return 0;
	case EncoderFrequency:
		return 1;
	default:
		break;
	}

	return 0;
}

// get encoder rotation
int FrontPanelControls::ReadEncoderDelta(FrontPanelControls::Encoder encoderid)
{
	int delta = _encoderdelta[EncoderToId(encoderid)];
	_encoderdelta[EncoderToId(encoderid)] = 0;
	return delta;
}

void FrontPanelControls::RequestAll()
{
	uint8_t buffer[2];
	buffer[0] = 0xff;
	buffer[1] = 0xff;

	_i2c.Write(0x10, buffer, sizeof(buffer));
}
