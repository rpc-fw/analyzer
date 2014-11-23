#include "freertos.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "analyzercontrol.h"

AnalyzerControl analyzercontrol;

namespace {
	SemaphoreHandle_t _semaphore = NULL;
}

void vAnalyzerControlTask(void* pvParameters)
{
	while(1) {
		analyzercontrol.Update();

		vTaskDelay(1);
	}
}

void AnalyzerControl::StartTask()
{
	_semaphore = xSemaphoreCreateCounting(1, 1);
	_needconfiguration = false;
	_needanalysis = 0;
	_analysiscomplete = false;

	xTaskCreate(vAnalyzerControlTask, "analyzercontrol", 512, NULL, 1 /* priority */, NULL);
}

void AnalyzerControl::Update()
{
	State prevstate = _state;
	do {
		prevstate = _state;

		switch (_state) {
		case StateIdle:
			if (_needconfiguration) {
				RequestConfigure();
				_state = ConfigurationUpload;
				break;
			}
			if (_needanalysis > 0) {
				RequestAnalyze();
				_state = StateAnalysisRunning;
				break;
			}
			break;
		case StateAnalysisRunning:
			if (AnalyzerCommandReady()) {
				WakeAnalysisRequest();
				_state = StateAnalysisProcessing;
			}
			break;
		case StateAnalysisProcessing:
			if (_analysiscomplete) {
				ReleaseAnalyzer();
				_state = StateAnalysisReleasing;
			}
			break;
		case StateAnalysisReleasing:
			if (AnalyzerCommandReady()) {
				_state = StateIdle;
			}
			break;
		case ConfigurationUpload:
			if (ConfigurationReady()) {
				_state = StateIdle;
			}
			break;
		}
	} while (_state != prevstate);
}

void AnalyzerControl::RequestConfigure()
{
	taskENTER_CRITICAL();

	_needconfiguration = false;
	commandMailbox.Write(_configuration);

	taskEXIT_CRITICAL();
}

void AnalyzerControl::RequestAnalyze()
{
	taskENTER_CRITICAL();

	AnalysisCommand cmd;
	cmd.commandType = AnalysisCommand::BLOCK;
	analysisCommandMailbox.Write(cmd);

	taskEXIT_CRITICAL();
}

bool AnalyzerControl::AnalyzerCommandReady()
{
	if (!analysisAckMailbox.CanRead()) {
		return false;
	}

	analysisAckMailbox.Read();
	return true;
}

void AnalyzerControl::WakeAnalysisRequest()
{
	xSemaphoreGive(_semaphore);
}

void AnalyzerControl::ReleaseAnalyzer()
{
	taskENTER_CRITICAL();

	AnalysisCommand cmd;
	cmd.commandType = AnalysisCommand::DONE;
	analysisCommandMailbox.Write(cmd);

	_analysiscomplete = false;

	taskEXIT_CRITICAL();
}

bool AnalyzerControl::ConfigurationReady()
{
	if (commandMailbox.CanWrite()) {
		ackMailbox.Read();
		return true;
	}

	return false;
}

void AnalyzerControl::AnalysisStart()
{
	taskENTER_CRITICAL();

	_needanalysis++;

	taskEXIT_CRITICAL();
}

bool AnalyzerControl::AnalysisAvailable()
{
	taskENTER_CRITICAL();

	bool result = _needanalysis && _state == StateAnalysisProcessing && !_analysiscomplete;

	taskEXIT_CRITICAL();

	return result;
}

void AnalyzerControl::AnalysisRead()
{
	xSemaphoreTake(_semaphore, portMAX_DELAY);
	WakeAnalysisRequest();
}


void AnalyzerControl::AnalysisFinish()
{
	taskENTER_CRITICAL();

	_needanalysis--;
	if (_needanalysis == 0) {
		xSemaphoreTake(_semaphore, portMAX_DELAY);
		_analysiscomplete = true;
	}

	taskEXIT_CRITICAL();
}

void AnalyzerControl::SetConfiguration(const GeneratorParameters& params)
{
	taskENTER_CRITICAL();

	_configuration = params;
	_needconfiguration = true;

	// Interrupt M4 core if it's analyzing
	if (StateAnalysisRunning) {
		__DSB();
		__SEV();
	}

	taskEXIT_CRITICAL();
}
