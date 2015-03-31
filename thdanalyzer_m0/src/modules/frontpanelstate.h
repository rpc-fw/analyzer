#ifndef FRONTPANELSTATE_H_
#define FRONTPANELSTATE_H_

#include "sharedtypes.h"
#include "frontpanelmenu.h"
#include "analyzerformat.h"

class FrontPanelState
{
public:
	enum RefLevelMode {
		RefLevel4dBu = 0,
		RefLevel10dBV,
		RefLevelCustom
	};

	enum OperationMode {
		OperationModeTHD = 0,
		OperationModeFrequencyAnalysis = 1,
		OperationModeDCVoltageControl = 2
	};

	FrontPanelState()
	{
		_menu = false;

		_operationmode = OperationModeTHD;

		_enable = true;
		_needrefresh = true;
		_balancedio = true;

		_frequency = 1000;
		_level = 4;
		_reflevelmode = RefLevel4dBu;

		_generatorfrequencydisplaymode = AnalyzerFormat::FrequencyDisplayModeHz;
		_generatorleveldisplaymode = AnalyzerFormat::LevelDisplayModeRefRelativeDecibel;
		_analyzerfrequencydisplaymode = AnalyzerFormat::FrequencyDisplayModeHz;
		_analyzerleveldisplaymode = AnalyzerFormat::LevelDisplayModeRefRelativeDecibel;
		_distortionfrequencydisplaymode = AnalyzerFormat::FrequencyDisplayModeHz;
		_distortionleveldisplaymode = AnalyzerFormat::LevelDisplayModeRefRelativeDecibel;

		_cv0 = 0.0;
		_cv1 = 0.0;
	}

	void SetOperationMode(OperationMode mode)
	{
		_operationmode = mode;
		_cv0 = 0.0;
		_cv1 = 0.0;
		Configure();
		Refresh();
	}

	OperationMode OperationMode() const { return _operationmode; }

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

	void SetGeneratorFrequencyDisplayMode(AnalyzerFormat::FrequencyDisplayMode mode) { _generatorfrequencydisplaymode = mode; Refresh(); }
	AnalyzerFormat::FrequencyDisplayMode GeneratorFrequencyDisplayMode() const { return _generatorfrequencydisplaymode; }

	void SetGeneratorLevelDisplayMode(AnalyzerFormat::LevelDisplayMode mode) { _generatorleveldisplaymode = mode; Refresh(); }
	AnalyzerFormat::LevelDisplayMode GeneratorLevelDisplayMode() const { return _generatorleveldisplaymode; }

	void SetAnalyzerFrequencyDisplayMode(AnalyzerFormat::FrequencyDisplayMode mode) { _analyzerfrequencydisplaymode = mode; Refresh(); }
	AnalyzerFormat::FrequencyDisplayMode AnalyzerFrequencyDisplayMode() const { return _analyzerfrequencydisplaymode; }

	void SetAnalyzerLevelDisplayMode(AnalyzerFormat::LevelDisplayMode mode) { _analyzerleveldisplaymode = mode; Refresh(); }
	AnalyzerFormat::LevelDisplayMode AnalyzerLevelDisplayMode() const { return _analyzerleveldisplaymode; }

	void SetDistortionFrequencyDisplayMode(AnalyzerFormat::FrequencyDisplayMode mode) { _distortionfrequencydisplaymode = mode; Refresh(); }
	AnalyzerFormat::FrequencyDisplayMode DistortionFrequencyDisplayMode() const { return _distortionfrequencydisplaymode; }

	void SetDistortionLevelDisplayMode(AnalyzerFormat::LevelDisplayMode mode) { _distortionleveldisplaymode = mode; Refresh(); }
	AnalyzerFormat::LevelDisplayMode DistortionLevelDisplayMode() const { return _distortionleveldisplaymode; }

	void SetCv0(float cv0) { _cv0 = ValidateCV(cv0); Configure(); Refresh(); }
	float Cv0() const { return _cv0; }

	void SetCv1(float cv1) { _cv1 = ValidateCV(cv1); Configure(); Refresh(); }
	float Cv1() const { return _cv1; }

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
	float ValidateCV(float cv);

	bool _menu;
	FrontPanelMenu::MenuEntryId _menuentry;
	bool _needconfigure;
	bool _needrefresh;

	float _frequency;
	float _level;

	float _distortionfrequency;
	float _distortionlevel;

	float _cv0;
	float _cv1;

	enum OperationMode _operationmode;
	bool _enable;
	bool _balancedio;

	enum RefLevelMode _reflevelmode;
	float _refleveldbu;

	AnalyzerFormat::FrequencyDisplayMode _generatorfrequencydisplaymode;
	AnalyzerFormat::LevelDisplayMode _generatorleveldisplaymode;
	AnalyzerFormat::FrequencyDisplayMode _analyzerfrequencydisplaymode;
	AnalyzerFormat::LevelDisplayMode _analyzerleveldisplaymode;
	AnalyzerFormat::FrequencyDisplayMode _distortionfrequencydisplaymode;
	AnalyzerFormat::LevelDisplayMode _distortionleveldisplaymode;
};

#endif /* FRONTPANELSTATE_H_ */
