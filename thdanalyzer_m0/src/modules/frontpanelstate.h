#ifndef FRONTPANELSTATE_H_
#define FRONTPANELSTATE_H_

#include "frontpanelmenu.h"

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

	void SetMenuActive(bool menu) { _menu = menu; Refresh(); }
	bool MenuActive() const { return _menu; }

	void SetMenuEntry(FrontPanelMenu::MenuEntryId entry) { _menuentry = entry; Refresh(); }
	FrontPanelMenu::MenuEntryId MenuEntry() const { return _menuentry; }

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

	const char* RelativeLevelString() const
	{
		switch (RefLevelMode()) {
		case FrontPanelState::RefLevel4dBu:
			return "dBu";
		case FrontPanelState::RefLevel10dBV:
			return "dBV";
		case FrontPanelState::RefLevelCustom:
			break;
		}
		return "dB";
	}

	float RelativeLevel() const
	{
		return RelativeLevel(Level());
	}

	float RelativeLevelGain() const
	{
		switch (RefLevelMode()) {
		default:
		case FrontPanelState::RefLevel4dBu:
			// will return 0
			break;
		case FrontPanelState::RefLevel10dBV:
			return -2.218487499;
		case FrontPanelState::RefLevelCustom:
			return -CustomRefLevel();
		}

		return 0;
	}

	float RelativeLevel(float level) const
	{
		return level + RelativeLevelGain();
	}

	void SetRelativeLevel(float level)
	{
		SetLevel(level - RelativeLevelGain());
	}

	bool NeedConfigure() { bool need = _needconfigure; _needconfigure = false; return need; }
	bool NeedRefresh() { bool need = _needrefresh; _needrefresh = false; return need; }

private:
	void Configure() { _needconfigure = true; }
	void Refresh() { _needrefresh = true; }

	float ValidateFrequency(float frequency);
	float ValidateLevel(float level);

	bool _menu;
	FrontPanelMenu::MenuEntryId _menuentry;
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

#endif /* FRONTPANELSTATE_H_ */
