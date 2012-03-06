#pragma once

#include "MenuView.hh"

class MdMenuView : public MenuView
{
private:
	MenuItem *item[14];

public:

	void loadMenu(bool highlightFirst)
	{
		uint items = 0;
		#ifdef CONFIG_USE_IN_TABLE_NAV
		resumeGame.init(); item[items++] = &resumeGame;
		#endif
		loadGame.init(); item[items++] = &loadGame;
		recentGames.init(); item[items++] = &recentGames;
		reset.init(); item[items++] = &reset;
		loadState.init(); item[items++] = &loadState;
		saveState.init(); item[items++] = &saveState;
		stateSlot.init(); item[items++] = &stateSlot;
		options.init(); item[items++] = &options;
		#ifdef CONFIG_BLUETOOTH
		scanWiimotes.init(); item[items++] = &scanWiimotes;
		#endif
		inputPlayerMap.init(); item[items++] = &inputPlayerMap;
		benchmark.init(); item[items++] = &benchmark;
		about.init(); item[items++] = &about;
		exitApp.init(); item[items++] = &exitApp;
		assert(items <= sizeofArray(item));
		BaseMenuView::init(item, items, highlightFirst);
	}

	void init(bool highlightFirst)
	{
		loadMenu(highlightFirst);
	}
};
