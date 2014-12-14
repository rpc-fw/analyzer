#ifndef ANALYZERCONTROL_H_
#define ANALYZERCONTROL_H_

#include "sharedtypes.h"

class AnalyzerControl
{
public:
	void StartTask();

	void AnalysisStart();
	void AnalysisFinish();

	bool AnalysisAvailable();
	void AnalysisRead();

	void SetConfiguration(const GeneratorParameters& params);

	void Update();
private:
	void RequestConfigure();
	void RequestAnalyze();
	bool AnalyzerCommandReady();
	void WakeAnalysisRequest();
	void ReleaseAnalyzer();
	bool ConfigurationReady();

	enum ConfigurationState {
		ConfigurationIdle,
		ConfigurationUpload
	};
	enum AnalysisState {
		StateIdle,
		StateAnalysisRunning,
		StateAnalysisProcessing,
		StateAnalysisReleasing,
	};

	AnalysisState _analysisstate;
	ConfigurationState _configurationstate;

	bool _needconfiguration;
	GeneratorParameters _configuration;

	int _needanalysis;
	bool _analysiscomplete;
};

extern AnalyzerControl analyzercontrol;

#endif /* ANALYZERCONTROL_H_ */
