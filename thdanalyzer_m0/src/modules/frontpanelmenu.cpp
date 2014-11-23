#include <string.h>

#include "frontpanelmenu.h"

FrontPanelMenu menu;

#include "menuentryimpl.h"

namespace {
	enum MenuEntryTag {
		TagNone = -1,
		TagIPNetwork = 1
	};

	enum MenuEntryType {
		MenuEntryTypeSentinel,
		MenuEntryTypeSubmenuTitle,
		MenuEntryTypeParameterList
	};

	struct MenuEntry {
		MenuEntryTag tag;
		MenuEntryTag childtag;
		FrontPanelMenu::MenuEntryId parent;
		FrontPanelMenu::MenuEntryId child;
		const char* name;
		MenuEntryType type;
		const IMenuEntry& handler;
	};

	#define SENTINEL() { TagNone, TagNone, -1, -1, 0, MenuEntryTypeSentinel, SentinelMenuEntry() }
	#define IS_SENTINEL(id) (menudata[id].name == 0)

	#define ENTRY(tag, child, name, type, handler) { tag, child, -1, -1, name, type, handler }

	MenuEntry menudata[] = {
			ENTRY(TagNone, TagNone, "Meter style", MenuEntryTypeParameterList, DistortionStyleMenuEntry()),
			ENTRY(TagNone, TagNone, "Generator style", MenuEntryTypeParameterList, GeneratorStyleMenuEntry()),
			ENTRY(TagNone, TagNone, "Operation mode", MenuEntryTypeParameterList, StringMenuEntry()),
			ENTRY(TagNone, TagIPNetwork, "IP network", MenuEntryTypeSubmenuTitle, TitleMenuEntry()),
			SENTINEL(),
			ENTRY(TagIPNetwork, TagNone, "IP address", MenuEntryTypeParameterList, IPAddressEntry()),
			SENTINEL(),
	};

	const int menucount = (sizeof(menudata) / sizeof(menudata[0]));

	FrontPanelMenu::MenuEntryId FindTag(MenuEntryTag tag)
	{
		for (int i = 0; i < menucount; i++) {
			if (menudata[i].tag == tag) {
				return i;
			}
		}

		return -1;
	}


	void InitMenu(FrontPanelMenu::MenuEntryId start, FrontPanelMenu::MenuEntryId parent)
	{
		if (start == -1) {
			return;
		}

		for (FrontPanelMenu::MenuEntryId i = start; !IS_SENTINEL(i); i++) {
			menudata[i].parent = parent;
			if (menudata[i].childtag != TagNone) {
				menudata[i].child = FindTag(menudata[i].childtag);
				InitMenu(menudata[i].child, i);
			}
		}
	}
}


FrontPanelMenu::FrontPanelMenu()
{
}

void FrontPanelMenu::Init()
{
	InitMenu(0, -1);
}

FrontPanelMenu::MenuEntryId FrontPanelMenu::Root(FrontPanelMenu::MenuEntryId current) const
{
	return 0;
}

FrontPanelMenu::MenuEntryId FrontPanelMenu::Next(FrontPanelMenu::MenuEntryId current) const
{
	MenuEntryId next = current + 1;

	// wrap around
	if (IS_SENTINEL(next)) {
		while (next > 0) {
			next--;
			if (IS_SENTINEL(next)) {
				// reached beginning of previous menu, step back
				next++;
				break;
			}
		}
	}

	return next;
}

FrontPanelMenu::MenuEntryId FrontPanelMenu::Prev(FrontPanelMenu::MenuEntryId current) const
{
	MenuEntryId prev = current - 1;

	// wrap around
	if (prev < 0 || IS_SENTINEL(prev)) {
		do {
			prev++;
		} while (!IS_SENTINEL(prev));

		// reached next sentinel, step back
		prev--;
	}

	return prev;
}

FrontPanelMenu::MenuEntryId FrontPanelMenu::EnterSubmenu(FrontPanelMenu::MenuEntryId entry) const
{
	return menudata[entry].child;
}

bool FrontPanelMenu::IsChild(FrontPanelMenu::MenuEntryId entry) const
{
	return (menudata[entry].parent != -1);
}

bool FrontPanelMenu::IsSubmenu(FrontPanelMenu::MenuEntryId entry) const
{
	return (menudata[entry].child != -1);
}

FrontPanelMenu::MenuEntryId FrontPanelMenu::Back(FrontPanelMenu::MenuEntryId current) const
{
	return menudata[current].parent;
}

void FrontPanelMenu::MenuRender(MenuEntryId entry, LcdBuffer& screen)
{
	strncpy(screen.row1, menudata[entry].name, screen.Width());
	screen.row2[0] = '\0';
	menudata[entry].handler.Render(screen);
}

void FrontPanelMenu::MenuValueChange(MenuEntryId entry, int delta)
{
	menudata[entry].handler.ChangeValue(delta);
}
