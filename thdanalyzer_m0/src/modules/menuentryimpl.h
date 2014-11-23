#ifndef MENUENTRYIMPL_H_
#define MENUENTRYIMPL_H_

class FrontPanelState;
class LcdBuffer;

class IMenuEntry
{
public:
	virtual ~IMenuEntry() {}
	virtual void Render(FrontPanelState*, LcdBuffer&) const = 0;
	virtual void ChangeValue(FrontPanelState*, int delta) const = 0;
};

class SentinelMenuEntry : public IMenuEntry
{
public:
	SentinelMenuEntry() {}

	virtual void Render(FrontPanelState* state, LcdBuffer& buffer) const;
	virtual void ChangeValue(FrontPanelState* state, int delta) const;
};

class DistortionStyleMenuEntry : public IMenuEntry
{
public:
	DistortionStyleMenuEntry() {}

	virtual void Render(FrontPanelState* state, LcdBuffer& buffer) const;
	virtual void ChangeValue(FrontPanelState* state, int delta) const;
};

class GeneratorStyleMenuEntry : public IMenuEntry
{
public:
	GeneratorStyleMenuEntry() {}

	virtual void Render(FrontPanelState* state, LcdBuffer& buffer) const;
	virtual void ChangeValue(FrontPanelState* state, int delta) const;
};

class StringMenuEntry : public IMenuEntry
{
public:
	StringMenuEntry() {}

	virtual void Render(FrontPanelState* state, LcdBuffer& buffer) const;
	virtual void ChangeValue(FrontPanelState* state, int delta) const;
};

class TitleMenuEntry : public IMenuEntry
{
public:
	TitleMenuEntry() {}

	virtual void Render(FrontPanelState* state, LcdBuffer& buffer) const;
	virtual void ChangeValue(FrontPanelState* state, int delta) const;
};

class IPAddressEntry : public IMenuEntry
{
public:
	IPAddressEntry() {}

	virtual void Render(FrontPanelState* state, LcdBuffer& buffer) const;
	virtual void ChangeValue(FrontPanelState* state, int delta) const;
};

class DHCPNameEntry : public IMenuEntry
{
public:
	DHCPNameEntry() {}

	virtual void Render(FrontPanelState* state, LcdBuffer& buffer) const;
	virtual void ChangeValue(FrontPanelState* state, int delta) const;
};

#endif /* MENUENTRYIMPL_H_ */
