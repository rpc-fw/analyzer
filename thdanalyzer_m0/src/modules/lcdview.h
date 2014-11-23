#ifndef LCDVIEW_H_
#define LCDVIEW_H_

class FrontPanelState;

class LcdView
{
public:
	LcdView();
	void Init();

	void Refresh(FrontPanelState* state);

	void SetState(FrontPanelState* state) { _state = state; }

private:
	FrontPanelState* _state;
};

#endif /* LCDVIEW_H_ */
