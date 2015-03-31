#ifndef LCDVIEW_H_
#define LCDVIEW_H_

class FrontPanelState;

class LcdView
{
public:
	LcdView();
	void Init(FrontPanelState* state);

	void Refresh();
private:
	void RenderLevelFrequency();
	void RenderCV();

	FrontPanelState* _state;
};

extern LcdView lcdview;

#endif /* LCDVIEW_H_ */
