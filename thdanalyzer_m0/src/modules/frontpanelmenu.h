#ifndef FRONTPANELMENU_H_
#define FRONTPANELMENU_H_

class LcdBuffer;
class FrontPanelState;

class FrontPanelMenu
{
public:
	typedef int MenuEntryId;

	FrontPanelMenu();
	void Init(FrontPanelState* state);

	MenuEntryId Root(MenuEntryId current) const;

	MenuEntryId Next(MenuEntryId current) const;
	MenuEntryId Prev(MenuEntryId current) const;
	MenuEntryId EnterSubmenu(MenuEntryId entry) const;
	MenuEntryId Back(MenuEntryId current) const;

	bool IsChild(MenuEntryId entry) const;
	bool IsSubmenu(MenuEntryId entry) const;

	void MenuRender(MenuEntryId entry, LcdBuffer& screen);
	void MenuValueChange(MenuEntryId entry, int delta);

private:
	FrontPanelState* _state;
};

extern FrontPanelMenu menu;

#endif /* FRONTPANELMENU_H_ */
