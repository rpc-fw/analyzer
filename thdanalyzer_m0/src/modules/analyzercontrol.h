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

	enum State {
		StateIdle,
		StateAnalysisRunning,
		StateAnalysisProcessing,
		StateAnalysisReleasing,
		ConfigurationUpload
	};

	State _state;

	bool _needconfiguration;
	GeneratorParameters _configuration;

	int _needanalysis;
	bool _analysiscomplete;
};

extern AnalyzerControl analyzercontrol;

#endif /* ANALYZERCONTROL_H_ */
