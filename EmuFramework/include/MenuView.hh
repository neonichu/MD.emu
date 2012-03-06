/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#pragma once

#include <util/gui/BaseMenuView.hh>
#include "EmuSystem.hh"
#include <meta.h>

static void onInputEvent(void *, const InputEvent &e);
void startGameFromMenu();
void removeModalView();

class OptionCategoryView : public BaseMenuView
{
	struct SubConfigMenuItem : public TextMenuItem
	{
		constexpr SubConfigMenuItem(): idx(0) { }
		uint idx;
		void init(const char *name, uint idx)
		{
			TextMenuItem::init(name);
			var_selfs(idx);
		}

		void select(View *view, const InputEvent &e)
		{
			oCategoryMenu.init(idx, !e.isPointer());
			viewStack.pushAndShow(&oCategoryMenu);
		}
	} subConfig[5];

	MenuItem *item[5];
public:
	constexpr OptionCategoryView(): BaseMenuView("Options")
	#ifdef CONFIG_CXX11
	, item({0})
	#endif
	{ }

	void init(bool highlightFirst)
	{
		//logMsg("running option category init");
		uint i = 0;
		static const char *name[] = { "Video", "Audio", "Input", "System", "GUI" };
		forEachInArray(subConfig, e)
		{
			e->init(name[e_i], e_i); item[i++] = e;
		}
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};

static OptionCategoryView oMenu;

class StateSlotView : public BaseMenuView
{
private:
	static const uint stateSlots = 11;
	struct StateSlotMenuItem : public TextMenuItem
	{
		constexpr StateSlotMenuItem(): slot(0) { }
		int slot;
		void init(const char *slotStr, int slot)
		{
			TextMenuItem::init(slotStr, EmuSystem::gameIsRunning() && EmuSystem::stateExists(slot));
			this->slot = slot;
		}

		void select(View *view, const InputEvent &e)
		{
			EmuSystem::saveStateSlot = slot;
			//view->dismiss(initMainMenu);
			viewStack.popAndShow();
		}
	} stateSlot[stateSlots];

	MenuItem *item[stateSlots];
public:
	constexpr StateSlotView(): BaseMenuView("State Slot")
	#ifdef CONFIG_CXX11
	, item({0})
	#endif
	{ }

	void init(bool highlightFirst)
	{
		uint i = 0;
		stateSlot[0].init("Auto", -1); item[i] = &stateSlot[i]; i++;
		stateSlot[1].init("0", 0); item[i] = &stateSlot[i]; i++;
		stateSlot[2].init("1", 1); item[i] = &stateSlot[i]; i++;
		stateSlot[3].init("2", 2); item[i] = &stateSlot[i]; i++;
		stateSlot[4].init("3", 3); item[i] = &stateSlot[i]; i++;
		stateSlot[5].init("4", 4); item[i] = &stateSlot[i]; i++;
		stateSlot[6].init("5", 5); item[i] = &stateSlot[i]; i++;
		stateSlot[7].init("6", 6); item[i] = &stateSlot[i]; i++;
		stateSlot[8].init("7", 7); item[i] = &stateSlot[i]; i++;
		stateSlot[9].init("8", 8); item[i] = &stateSlot[i]; i++;
		stateSlot[10].init("9", 9); item[i] = &stateSlot[i]; i++;
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};

static StateSlotView ssMenu;

class RecentGameView : public BaseMenuView
{
private:
#ifdef CONFIG_USE_IN_TABLE_NAV
	BackMenuItem back;
#endif

	struct RecentGameMenuItem : public TextMenuItem
	{
		constexpr RecentGameMenuItem(): info(0) { }
		RecentGameInfo *info;
		void init(RecentGameInfo *info)
		{
			TextMenuItem::init(info->name, Fs::fileExists(info->path));
			this->info = info;
		}

		void select(View *view, const InputEvent &e)
		{
			FsSys::cPath dir, file;
			dirName(info->path, dir);
			baseName(info->path, file);
			FsSys::chdir(dir);
			if(EmuSystem::loadGame(file))
			{
				//view->dismiss(startGameFromMenu);
				startGameFromMenu();
			}
		}
	} recentGame[10];

	struct ClearMenuItem : public TextMenuItem
	{
		constexpr ClearMenuItem() { }
		void init()
		{
			TextMenuItem::init("Clear List", recentGameList.size);
		}

		void select(View *view, const InputEvent &e)
		{
			recentGameList.removeAll();
			viewStack.popAndShow();
			//view->dismiss(initMainMenu);
		}
	} clear;

	MenuItem *item[1 + 10 + 1];
public:
	constexpr RecentGameView(): BaseMenuView("Recent Games")
	#ifdef CONFIG_CXX11
	, item({0})
	#endif
	{ }

	void init(bool highlightFirst)
	{
		uint i = 0;
		#ifdef CONFIG_USE_IN_TABLE_NAV
		back.init(); item[i++] = &back;
		#endif
		int rIdx = 0;
		forEachInDLList(&recentGameList, e)
		{
			recentGame[rIdx].init(&e); item[i++] = &recentGame[rIdx];
			rIdx++;
		}
		clear.init(); item[i++] = &clear;
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};

static RecentGameView rMenu;

class InputPlayerMapView : public BaseMenuView
{
private:
	static const uint numDevs = 1 // key
	+ 1 // pointer
	+ 1 // iCade
	#ifdef CONFIG_BASE_PS3
	+ 5
	#endif
	#ifdef CONFIG_BLUETOOTH
	+ (Bluetooth::maxGamepadsPerTypeStorage*3)
	#endif
	;

	struct InputPlayerMapMenuItem : public MultiChoiceMenuItem
	{
		constexpr InputPlayerMapMenuItem(): player(0) { }
		uint *player;
		void init(const char *name, uint *player)
		{
			this->player = player;
			static const char *str[] = { "1", "2", "3", "4", "5" };
			MultiChoiceMenuItem::init(name, str, *player, maxPlayers);
		}

		void select(View *view, const InputEvent &e)
		{
			multiChoiceView.init(this, !e.isPointer());
			multiChoiceView.place(Gfx::viewportRect());
			modalView = &multiChoiceView;
		}

		void doSet(int val)
		{
			*player = val;
			//updateActivePlayerKeyInputs();
		}
	} inputMap[numDevs];

#ifdef CONFIG_USE_IN_TABLE_NAV
	BackMenuItem back;
#endif

	MenuItem *item[numDevs + 1];
public:
	constexpr InputPlayerMapView(): BaseMenuView("Input/Player Mapping")
	#ifdef CONFIG_CXX11
	, item({0})
	#endif
	{ }

	void init(bool highlightFirst)
	{
		uint i = 0, iMaps = 0;
		#ifdef CONFIG_USE_IN_TABLE_NAV
		back.init(); item[i] = &back; i++;
		#endif
		#ifdef INPUT_SUPPORTS_POINTER
		inputMap[iMaps].init("Touch Screen", &pointerInputPlayer); item[i] = &inputMap[iMaps++]; i++;
		#endif
		#if !defined(CONFIG_BASE_IOS) && !defined(CONFIG_BASE_PS3) && defined(INPUT_SUPPORTS_KEYBOARD)
		inputMap[iMaps].init("Keyboard", &keyboardInputPlayer); item[i] = &inputMap[iMaps++]; i++;
		#endif
		#ifdef CONFIG_BASE_IOS_ICADE
		inputMap[iMaps].init("iCade", &iCadeInputPlayer); item[i] = &inputMap[iMaps++]; i++;
		#endif
		#if defined(CONFIG_BASE_PS3)
		assert(maxPlayers <= 5);
		iterateTimes((int)maxPlayers, p)
		{
			static const char *str[] = { "Controller 1", "Controller 2", "Controller 3", "Controller 4", "Controller 5" };
			inputMap[iMaps].init(str[p], &gamepadInputPlayer[p]); item[i] = &inputMap[iMaps++]; i++;
		}
		#endif
		#ifdef CONFIG_BLUETOOTH
		assert(maxPlayers <= 5);
		iterateTimes((int)maxPlayers, p)
		{
			static const char *str[] = { "Wiimote 1", "Wiimote 2", "Wiimote 3", "Wiimote 4", "Wiimote 5" };
			inputMap[iMaps].init(str[p], &wiimoteInputPlayer[p]); item[i] = &inputMap[iMaps++]; i++;
		}
		iterateTimes((int)maxPlayers, p)
		{
			static const char *str[] = { "iControlPad 1", "iControlPad 2", "iControlPad 3", "iControlPad 4", "iControlPad 5" };
			inputMap[iMaps].init(str[p], &iControlPadInputPlayer[p]); item[i] = &inputMap[iMaps++]; i++;
		}
		iterateTimes((int)maxPlayers, p)
		{
			static const char *str[] = { "Zeemote JS1 1", "Zeemote JS1 2", "Zeemote JS1 3", "Zeemote JS1 4", "Zeemote JS1 5" };
			inputMap[iMaps].init(str[p], &zeemoteInputPlayer[p]); item[i] = &inputMap[iMaps++]; i++;
		}
		#endif
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};

static InputPlayerMapView ipmMenu;

//class BenchmarkFilePicker;

class MenuView : public BaseMenuView
{
protected:
	struct LoadGameMenuItem : public TextMenuItem
	{
		constexpr LoadGameMenuItem() { }
		void init() { TextMenuItem::init("Load Game"); }

		void select(View *view, const InputEvent &e)
		{
			//view->dismiss(initFilePicker);
			fPicker.init(!e.isPointer());
			viewStack.useNavView = 0;
			viewStack.pushAndShow(&fPicker);
		}
	} loadGame;

#ifdef CONFIG_USE_IN_TABLE_NAV
	struct ResumeGameMenuItem : public TextMenuItem
	{
		void init() { TextMenuItem::init("Resume Game"); }

		void refreshActive()
		{
			active = EmuSystem::gameIsRunning();
		}

		void select(View *view)
		{
			if(EmuSystem::gameIsRunning())
			{
				//view->dismiss(startGameFromMenu);
				startGameFromMenu();
			}
		}
	} resumeGame;
#endif

	struct ResetMenuItem : public TextMenuItem, public YesNoAlertView::Callback
	{
		constexpr ResetMenuItem() { }
		void init() { TextMenuItem::init("Reset"); }

		void refreshActive()
		{
			active = EmuSystem::gameIsRunning();
		}

		void onSelect()
		{
			EmuSystem::resetGame();
			startGameFromMenu();
			//activeView->dismiss(startGameFromMenu);
		}

		void select(View *view, const InputEvent &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				ynAlertView.init("Really Reset Game?", this, !e.isPointer());
				ynAlertView.place(Gfx::viewportRect());
				modalView = &ynAlertView;
			}
		}
	} reset;

	struct LoadStateMenuItem : public TextMenuItem, public YesNoAlertView::Callback
	{
		constexpr LoadStateMenuItem() { }
		void init()
		{
			TextMenuItem::init("Load State");
		}

		void refreshActive()
		{
			active = EmuSystem::gameIsRunning() && EmuSystem::stateExists(EmuSystem::saveStateSlot);
		}

		void onSelect()
		{
			int ret = EmuSystem::loadState();
			if(ret != STATE_RESULT_OK)
			{
				if(ret != STATE_RESULT_OTHER_ERROR) // check if we're responsible for posting the error
					popup.postError(stateResultToStr(ret));
			}
			else
				startGameFromMenu();
				//activeView->dismiss(startGameFromMenu);
		}

		void select(View *view, const InputEvent &e)
		{
			if(active && EmuSystem::gameIsRunning())
			{
				ynAlertView.init("Really Load State?", this, !e.isPointer());
				ynAlertView.place(Gfx::viewportRect());
				modalView = &ynAlertView;
			}
		}
	} loadState;

	struct RecentGamesItem : public TextMenuItem
	{
		constexpr RecentGamesItem() { }
		void init() { TextMenuItem::init("Recent Games"); }

		void select(View *view, const InputEvent &e)
		{
			if(recentGameList.size)
			{
				//view->dismiss(initRecentGamesMenu);
				rMenu.init(!e.isPointer());
				viewStack.pushAndShow(&rMenu);
			}
		}

		void refreshActive()
		{
			active = recentGameList.size;
		}
	} recentGames;

	struct SaveStateMenuItem : public TextMenuItem, public YesNoAlertView::Callback
	{
		constexpr SaveStateMenuItem() { }
		void init() { TextMenuItem::init("Save State"); }

		void refreshActive()
		{
			active = EmuSystem::gameIsRunning();
		}

		void onSelect()
		{
			int ret = EmuSystem::saveState();
			if(ret != STATE_RESULT_OK)
				popup.postError(stateResultToStr(ret));
			else
				startGameFromMenu();
				//activeView->dismiss(startGameFromMenu);
		}

		void select(View *view, const InputEvent &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				if(!EmuSystem::stateExists(EmuSystem::saveStateSlot))
				{
					onSelect();
				}
				else
				{
					ynAlertView.init("Really Overwrite State?", this, !e.isPointer());
					ynAlertView.place(Gfx::viewportRect());
					modalView = &ynAlertView;
				}
			}
		}
	} saveState;

	struct StateSlotMenuItem : public TextMenuItem
	{
		constexpr StateSlotMenuItem()
		#ifdef CONFIG_CXX11
		: txt({0})
		#endif
		{ }

		static char saveSlotChar(int slot)
		{
			switch(slot)
			{
				case -1: return 'a';
				case 0 ... 9: return 48 + slot;
				default: bug_branch("%d", slot); return 0;
			}
		}

		char txt[sizeof("State Slot (0)")];

		void init()
		{
			logMsg("size %d", (int)sizeof("State Slot (0)"));
			strcpy(txt, "State Slot (0)");
			txt[12] = saveSlotChar(EmuSystem::saveStateSlot);
			TextMenuItem::init(txt);
		}

		void select(View *view, const InputEvent &e)
		{
			//view->dismiss(initStateSlotMenu);
			ssMenu.init(!e.isPointer());
			viewStack.pushAndShow(&ssMenu);
		}

		void refreshActive()
		{
			txt[12] = saveSlotChar(EmuSystem::saveStateSlot);
			compile();
		}
	} stateSlot;

	struct OptionsMenuItem : public TextMenuItem
	{
		constexpr OptionsMenuItem() { }
		void init() { TextMenuItem::init("Options"); }

		void select(View *view, const InputEvent &e)
		{
			//view->dismiss(initOptionMenu);
			oMenu.init(!e.isPointer());
			viewStack.pushAndShow(&oMenu);
		}
	} options;

	struct InputPlayerMapItem : public TextMenuItem
	{
		constexpr InputPlayerMapItem() { }
		void init() { TextMenuItem::init("Input/Player Mapping"); }

		void select(View *view, const InputEvent &e)
		{
			//view->dismiss(initInputPlayerMapMenu);
			ipmMenu.init(!e.isPointer());
			viewStack.pushAndShow(&ipmMenu);
		}
	} inputPlayerMap;

	struct BenchmarkMenuItem : public TextMenuItem
	{
		constexpr BenchmarkMenuItem() { }
		BenchmarkFilePicker picker;
		void init() { TextMenuItem::init("Benchmark Game"); }

		void select(View *view, const InputEvent &e)
		{
			picker.init(!e.isPointer());
			picker.place(Gfx::viewportRect());
			modalView = &picker;
			Base::displayNeedsUpdate();
		}
	} benchmark;

	#ifdef CONFIG_BLUETOOTH
	struct ScanWiimotesMenuItem : public TextMenuItem
	{
		constexpr ScanWiimotesMenuItem() { }
		void init() { TextMenuItem::init("Scan for Wiimotes/iCP/JS1"); }

		static void btDevConnected(int msg)
		{
			using namespace Bluetooth;
			switch(msg)
			{
				bcase MSG_NO_DEVS:
				{
					popup.post("No devices found");
					Base::displayNeedsUpdate();
				}
				bcase MSG_ERR_CHANNEL:
				{
					popup.postError("Failed opening BT channel");
					Base::displayNeedsUpdate();
				}
				bcase MSG_ERR_NAME:
				{
					popup.postError("Failed reading device name");
					Base::displayNeedsUpdate();
				}
				#ifdef CONFIG_BASE_IOS
				bcase MSG_NO_PERMISSION:
				{
					popup.postError("BTstack power on failed, make sure the iOS Bluetooth stack is not active");
					Base::displayNeedsUpdate();
				}
				#endif
			}
		}

		#ifdef CONFIG_BTSTACK

		struct InstallBtStack : public YesNoAlertView::Callback
		{
			void onSelect()
			{
				logMsg("launching Cydia");
				Base::openURL("cydia://package/ch.ringwald.btstack");
			}
		} installBtStack;

		#endif

		void select(View *view, const InputEvent &e)
		{
			#ifdef CONFIG_BASE_IOS_SETUID
				if(Fs::fileExists(CATS::warWasBeginning))
					return;
			#endif

			if(Bluetooth::initBT() == OK)
			{
				if(Bluetooth::startBT(btDevConnected))
				{
					popup.post("Starting Scan...\nSee website for device-specific help", 4);
				}
				else
				{
					popup.post("Still scanning", 1);
				}
			}
			else
			{
				#ifdef CONFIG_BTSTACK
					popup.postError("Failed connecting to BT daemon");
					if(!Fs::fileExists("/var/lib/dpkg/info/ch.ringwald.btstack.list"))
					{
						ynAlertView.init("BTstack not found, open Cydia and install?", &installBtStack, !e.isPointer());
						ynAlertView.place(Gfx::viewportRect());
						modalView = &ynAlertView;
					}
				#elif defined(CONFIG_BASE_ANDROID)
					popup.postError("Bluetooth not accessible, verify it's on and your Android version is compatible");
				#else
					popup.postError("Bluetooth not accessible");
				#endif
			}
			Base::displayNeedsUpdate();
		}
	} scanWiimotes;
	#endif

	struct AboutMenuItem : public TextMenuItem
	{
		constexpr AboutMenuItem() { }
		void init() { TextMenuItem::init("About"); }

		void select(View *view, const InputEvent &e)
		{
			credits.init();
			viewStack.pushAndShow(&credits);
			//credits.dismissHandler = initMainMenu;
		}
	} about;

	struct ExitAppMenuItem : public TextMenuItem
	{
		constexpr ExitAppMenuItem() { }
		void init() { TextMenuItem::init("Exit"); }

		void select(View *view, const InputEvent &e)
		{
			Base::exit();
		}
	} exitApp;

public:
	constexpr MenuView(): BaseMenuView(CONFIG_APP_NAME " " IMAGINE_VERSION) { }

	void onShow()
	{
		logMsg("refreshing main menu state");
#ifdef CONFIG_USE_IN_TABLE_NAV
		resumeGame.refreshActive();
#endif
		recentGames.refreshActive();
		reset.refreshActive();
		saveState.refreshActive();
		loadState.refreshActive();
		stateSlot.refreshActive();
	}
};
