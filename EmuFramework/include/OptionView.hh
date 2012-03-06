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

#include <gui/View.hh>
#include <util/gui/MenuItem.hh>

void removeModalView();
static void setupDrawing(bool force = 0);
static void placeGameView();

class ButtonConfigCategoryView : public BaseMenuView
{
	struct ProfileMenuItem : public TextMenuItem, public MultiPickView::Callback, public YesNoAlertView::Callback
		{
			MultiPickView picker;
			const char *name[10];
			uint names;
			uint val;
			uint devType;

			void init(uint devType, KeyProfileManager *profileMgr)
			{
				names = profileMgr->defaultProfiles + 1;
				assert(names < sizeofArray(name));
				name[0] = "Unbind All";
				iterateTimes(profileMgr->defaultProfiles, i)
				{
					name[i+1] = profileMgr->defaultProfile[i].name;
				}
				TextMenuItem::init("Load Defaults");
				var_selfSet(devType);
			}

			void select(View *view, const InputEvent &e)
			{
				picker.init(name, names, this, !e.isPointer());
				picker.place(Gfx::viewportRect());
				modalView = &picker;
			}

			bool doSet(int val, const InputEvent &e)
			{
				removeModalView();
				this->val = val;
				ynAlertView.init("Really apply new key bindings?", this, !e.isPointer());
				ynAlertView.place(Gfx::viewportRect());
				modalView = &ynAlertView;
				return 0;
			}

			void onSelect()
			{
				keyConfig.loadProfile(devType, val-1);
				keyMapping.build(EmuControls::category);
			}

		} profile;

	struct CategoryMenuItem : public TextMenuItem
	{
		KeyCategory *cat;
		uint devType;
		void init(KeyCategory *cat, uint devType)
		{
			TextMenuItem::init(cat->name);
			var_selfs(cat);
			var_selfs(devType);
		}

		void select(View *view, const InputEvent &e)
		{
			bcMenu.init(cat, devType, !e.isPointer());
			viewStack.pushAndShow(&bcMenu);
		}
	} cat[sizeofArrayConst(EmuControls::category)];

	MenuItem *item[sizeofArrayConst(EmuControls::category) + 1];
public:
	void init(uint devType, bool highlightFirst)
	{
		using namespace EmuControls;
		name_ = InputEvent::devTypeName(devType);
		uint i = 0;
		profile.init(devType, &EmuControls::profileManager(devType)); item[i++] = &profile;
		forEachInArray(category, c)
		{
			cat[c_i].init(c, devType); item[i++] = &cat[c_i];
		}
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};

static ButtonConfigCategoryView bcatMenu;

class OptionView : public BaseMenuView
{
protected:
#ifdef CONFIG_USE_IN_TABLE_NAV
	BackMenuItem back;
#endif

	struct SndMenuItem : public BoolMenuItem
	{
		constexpr SndMenuItem() { }
		void init(bool on) { BoolMenuItem::init("Sound", on); }

		void select(View *view, const InputEvent &e)
		{
			toggle();
			optionSound = on;
		}
	} snd;

	#ifdef CONFIG_ENV_WEBOS
	struct TouchCtrlMenuItem : public BoolMenuItem
	{
		void init() { BoolMenuItem::init("On-screen Controls", optionTouchCtrl); }

		void select(View *view, const InputEvent &e)
		{
			toggle();
			optionTouchCtrl = on;
		}
	} touchCtrl;
	#else
	struct TouchCtrlMenuItem : public MultiChoiceMenuItem
	{
		constexpr TouchCtrlMenuItem() { }
		void init()
		{
			static const char *str[] =
			{
				"Off", "On", "Auto"
			};
			MultiChoiceMenuItem::init("On-screen Controls", str, int(optionTouchCtrl), sizeofArray(str));
		}

		void select(View *view, const InputEvent &e)
		{
			multiChoiceView.init(this, !e.isPointer());
			multiChoiceView.place(Gfx::viewportRect());
			modalView = &multiChoiceView;
		}

		void doSet(int val)
		{
			optionTouchCtrl = val;
		}
	} touchCtrl;
	#endif

	struct TouchCtrlConfigMenuItem : public TextMenuItem
	{
		constexpr TouchCtrlConfigMenuItem() { }
		void init() { TextMenuItem::init("On-screen Config"); }

		void select(View *view, const InputEvent &e)
		{
			#ifndef CONFIG_BASE_PS3
				logMsg("init touch config menu");
				tcMenu.init(!e.isPointer());
				viewStack.pushAndShow(&tcMenu);
			#endif
		}
	} touchCtrlConfig;

	struct AutoSaveStateMenuItem : public MultiChoiceMenuItem
	{
		constexpr AutoSaveStateMenuItem() { }
		void init()
		{
			static const char *str[] =
			{
				"Off", "On Exit",
				"15mins", "30mins"
			};
			int val = 0;
			switch(optionAutoSaveState.val)
			{
				bcase 1: val = 1;
				bcase 15: val = 2;
				bcase 30: val = 3;
			}
			MultiChoiceMenuItem::init("Auto-save State", str, val, sizeofArray(str));
		}

		void select(View *view, const InputEvent &e)
		{
			multiChoiceView.init(this, !e.isPointer());
			multiChoiceView.place(Gfx::viewportRect());
			modalView = &multiChoiceView;
		}

		void doSet(int val)
		{
			switch(val)
			{
				bcase 0: optionAutoSaveState.val = 0;
				bcase 1: optionAutoSaveState.val = 1;
				bcase 2: optionAutoSaveState.val = 15;
				bcase 3: optionAutoSaveState.val = 30;
			}
			EmuSystem::setupAutoSaveStateTime(optionAutoSaveState.val);
			logMsg("set auto-savestate %d", optionAutoSaveState.val);
		}
	} autoSaveState;

	struct FrameSkipMenuItem : public MultiChoiceMenuItem
	{
		constexpr FrameSkipMenuItem() { }
		void init()
		{
			static const char *str[] =
			{
				"Auto", "0",
				#if defined(CONFIG_BASE_IOS) || defined(CONFIG_BASE_X11)
				"1", "2", "3", "4"
				#endif
			};
			int baseVal = -1;
			int val = int(optionFrameSkip);
			if(optionFrameSkip.val == EmuSystem::optionFrameSkipAuto)
				val = -1;
			MultiChoiceMenuItem::init("Frame Skip", str, val, sizeofArray(str), baseVal);
		}

		void select(View *view, const InputEvent &e)
		{
			multiChoiceView.init(this, !e.isPointer());
			multiChoiceView.place(Gfx::viewportRect());
			modalView = &multiChoiceView;
		}

		void doSet(int val)
		{
			if(val == -1)
			{
				optionFrameSkip.val = EmuSystem::optionFrameSkipAuto;
				logMsg("set auto frame skip");
			}
			else
			{
				optionFrameSkip.val = val;
				logMsg("set frame skip: %d", int(optionFrameSkip));
			}
			EmuSystem::configAudioRate();
		}
	} frameSkip;

	struct AudioRateMenuItem : public MultiChoiceMenuItem
	{
		constexpr AudioRateMenuItem() { }
		void init()
		{
			static const char *str[] =
			{
				"22KHz", "32KHz", "44KHz", "48KHz"
			};
			int rates = 3;
			if(Audio::supportsRateNative(48000))
			{
				logMsg("supports 48KHz");
				rates++;
			}

			int val = 2; // default to 44KHz
			switch(optionSoundRate)
			{
				bcase 22050: val = 0;
				bcase 32000: val = 1;
				bcase 48000: val = 3;
			}

			MultiChoiceMenuItem::init("Sound Rate", str, val, rates);
		}

		void select(View *view, const InputEvent &e)
		{
			multiChoiceView.init(this, !e.isPointer());
			multiChoiceView.place(Gfx::viewportRect());
			modalView = &multiChoiceView;
		}

		void doSet(int val)
		{
			uint rate = 44100;
			switch(val)
			{
				bcase 0: rate = 22050;
				bcase 1: rate = 32000;
				bcase 3: rate = 48000;
			}
			if(rate != optionSoundRate)
			{
				optionSoundRate = rate;
				EmuSystem::configAudioRate();
			}
		}
	} audioRate;

	#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
	struct DirectTextureMenuItem : public BoolMenuItem
	{
		void init(bool on)
		{
			BoolMenuItem::init("Direct Texture", on, gfx_androidDirectTextureSupported());
		}

		void select(View *view, const InputEvent &e)
		{
			if(!active)
			{
				popup.postError(gfx_androidDirectTextureErrorString(gfx_androidDirectTextureError()));
				return;
			}
			toggle();
			gfx_setAndroidDirectTexture(on);
			optionDirectTexture.val = on;
			if(vidImg.tid)
				setupDrawing(1);
		}
	} directTexture;

	struct GLSyncHackMenuItem : public BoolMenuItem
	{
		void init()
		{
			BoolMenuItem::init("GPU Sync Hack", optionGLSyncHack);
		}

		void select(View *view, const InputEvent &e)
		{
			toggle();
			glSyncHackEnabled = on;
		}
	} glSyncHack;
	#endif

	struct PauseUnfocusedMenuItem : public BoolMenuItem
	{
		constexpr PauseUnfocusedMenuItem() { }
		void init(bool on)
		{
			BoolMenuItem::init(
			#ifndef CONFIG_BASE_PS3
				"Pause if unfocused"
			#else
				"Pause in XMB"
			#endif
			, on);
		}

		void select(View *view, const InputEvent &e)
		{
			toggle();
			optionPauseUnfocused = on;
		}
	} pauseUnfocused;

	struct LargeFontsMenuItem : public BoolMenuItem
	{
		constexpr LargeFontsMenuItem() { }
		void init(bool on) { BoolMenuItem::init("Large Fonts", on); }

		void select(View *view, const InputEvent &e)
		{
			toggle();
			optionLargeFonts = on;
			onViewChange();
		}
	} largeFonts;

	struct NotificationIconMenuItem : public BoolMenuItem
	{
		constexpr NotificationIconMenuItem() { }
		void init() { BoolMenuItem::init("Suspended App Icon", optionNotificationIcon); }

		void select(View *view, const InputEvent &e)
		{
			toggle();
			optionNotificationIcon = on;
		}
	} notificationIcon;

	struct AltGamepadConfirmMenuItem : public BoolMenuItem
	{
		constexpr AltGamepadConfirmMenuItem() { }
		void init() { BoolMenuItem::init("Alt Gamepad Confirm", input_swappedGamepadConfirm); }

		void select(View *view, const InputEvent &e)
		{
			toggle();
			input_swappedGamepadConfirm = on;
		}
	} altGamepadConfirm;

	struct NavViewMenuItem : public BoolMenuItem
	{
		constexpr NavViewMenuItem() { }
		void init() { BoolMenuItem::init("Title Bar", optionTitleBar); }

		void select(View *view, const InputEvent &e)
		{
			toggle();
			optionTitleBar = on;
			viewStack.setNavView(on ? &viewNav : 0);
			onViewChange();
		}
	} navView;

	struct BackNavMenuItem : public BoolMenuItem
	{
		constexpr BackNavMenuItem() { }
		void init() { BoolMenuItem::init("Title Back Navigation", View::needsBackControl); }

		void select(View *view, const InputEvent &e)
		{
			toggle();
			View::setNeedsBackControl(on);
			viewNav.setBackImage(View::needsBackControl ? getArrowAsset() : 0);
			onViewChange();
		}
	} backNav;

	struct RememberLastMenuItem : public BoolMenuItem
	{
		constexpr RememberLastMenuItem() { }
		void init() { BoolMenuItem::init("Remember last menu", optionRememberLastMenu); }

		void select(View *view, const InputEvent &e)
		{
			toggle();
			optionRememberLastMenu = on;
		}
	}	rememberLastMenu;

	struct ButtonConfigMenuItem : public TextMenuItem
	{
		constexpr ButtonConfigMenuItem() { }
		void init() { TextMenuItem::init("Key Config"); }

		void select(View *view, const InputEvent &e)
		{
			#if defined(CONFIG_BASE_PS3)
				//bcatMenu.init(systemKeyControl, systemKeyControlName, guiKeyControl, InputEvent::DEV_PS3PAD, !e.isPointer());
				bcatMenu.init(InputEvent::DEV_PS3PAD, !e.isPointer());
			#elif defined(INPUT_SUPPORTS_KEYBOARD)
				bcatMenu.init(InputEvent::DEV_KEYBOARD, !e.isPointer());
			#else
				bug_exit("invalid dev type in initButtonConfigMenu");
			#endif
			viewStack.pushAndShow(&bcatMenu);
		}
	} buttonConfig;

	#ifdef CONFIG_BASE_IOS_ICADE
	struct ICadeMenuItem : public BoolMenuItem
	{
		constexpr ICadeMenuItem() { }
		void init() { BoolMenuItem::init("Use iCade", Base::ios_iCadeActive()); }

		void select(View *view, const InputEvent &e)
		{
			toggle();
			Base::ios_setICadeActive(on);
		}
	} iCade;

	struct ICadeButtonConfigMenuItem : public TextMenuItem
	{
		constexpr ICadeButtonConfigMenuItem() { }
		void init() { TextMenuItem::init("iCade Key Config"); }

		void select(View *view, const InputEvent &e)
		{
			bcatMenu.init(InputEvent::DEV_ICADE, !e.isPointer());
			viewStack.pushAndShow(&bcatMenu);
		}
	} iCadeButtonConfig;
	#endif

	#ifdef CONFIG_BLUETOOTH
	struct WiiButtonConfigMenuItem : public TextMenuItem
	{
		constexpr WiiButtonConfigMenuItem() { }
		void init() { TextMenuItem::init("Wiimote Key Config"); }

		void select(View *view, const InputEvent &e)
		{
			bcatMenu.init(InputEvent::DEV_WIIMOTE, !e.isPointer());
			viewStack.pushAndShow(&bcatMenu);
		}
	} wiiButtonConfig;

	struct ICPButtonConfigMenuItem : public TextMenuItem
	{
		constexpr ICPButtonConfigMenuItem() { }
		void init() { TextMenuItem::init("iControlPad Key Config"); }

		void select(View *view, const InputEvent &e)
		{
			bcatMenu.init(InputEvent::DEV_ICONTROLPAD, !e.isPointer());
			viewStack.pushAndShow(&bcatMenu);
		}
	} iCPButtonConfig;

	struct ZeemoteButtonConfigMenuItem : public TextMenuItem
	{
		constexpr ZeemoteButtonConfigMenuItem() { }
		void init() { TextMenuItem::init("Zeemote JS1 Key Config"); }

		void select(View *view, const InputEvent &e)
		{
			bcatMenu.init(InputEvent::DEV_ZEEMOTE, !e.isPointer());
			viewStack.pushAndShow(&bcatMenu);
		}
	} zeemoteButtonConfig;

	struct BTScanSecsMenuItem : public MultiChoiceMenuItem
	{
		constexpr BTScanSecsMenuItem() { }
		void init()
		{
			static const char *str[] =
			{
				"2secs", "4secs", "6secs", "8secs", "10secs"
			};

			int val = 1;
			switch(Bluetooth::scanSecs)
			{
				bcase 2: val = 0;
				bcase 4: val = 1;
				bcase 6: val = 2;
				bcase 8: val = 3;
				bcase 10: val = 4;
			}
			MultiChoiceMenuItem::init("Bluetooth Scan", str, val, sizeofArray(str));
		}

		void select(View *view, const InputEvent &e)
		{
			multiChoiceView.init(this, !e.isPointer());
			multiChoiceView.place(Gfx::viewportRect());
			modalView = &multiChoiceView;
		}

		void doSet(int val)
		{
			switch(val)
			{
				bcase 0: Bluetooth::scanSecs = 2;
				bcase 1: Bluetooth::scanSecs = 4;
				bcase 2: Bluetooth::scanSecs = 6;
				bcase 3: Bluetooth::scanSecs = 8;
				bcase 4: Bluetooth::scanSecs = 10;
			}
			logMsg("set bluetooth scan time %d", Bluetooth::scanSecs);
		}
	} btScanSecs;
	#endif

	struct OrientationMenuItem : public MultiChoiceMenuItem
	{
		constexpr OrientationMenuItem() { }
		enum { O_AUTO = -1, O_90, O_270, O_0 };

		int convertMenuValueToOption(int val)
		{
			if(val == O_AUTO)
				return Gfx::VIEW_ROTATE_AUTO;
			else if(val == O_90)
				return Gfx::VIEW_ROTATE_90;
			else if(val == O_270)
				return Gfx::VIEW_ROTATE_270;
			else if(val == O_0)
				return Gfx::VIEW_ROTATE_0;
			assert(0);
			return 0;
		}

		void init(const char *name, uint option)
		{
			static const char *str[] =
			{
				#if defined(CONFIG_BASE_IOS) || defined(CONFIG_BASE_ANDROID)
				"Auto",
				#endif

				#if defined(CONFIG_BASE_IOS) || defined(CONFIG_BASE_ANDROID) || defined(CONFIG_ENV_WEBOS)
				"Landscape", "Landscape 2", "Portrait"
				#else
				"90 Left", "90 Right", "None"
				#endif
			};
			int baseVal = 0;
			#if defined(CONFIG_BASE_IOS) || defined(CONFIG_BASE_ANDROID)
				baseVal = -1;
			#endif
			uint initVal = O_AUTO;
			if(option == Gfx::VIEW_ROTATE_90)
				initVal = O_90;
			else if(option == Gfx::VIEW_ROTATE_270)
				initVal = O_270;
			else if(option == Gfx::VIEW_ROTATE_0)
				initVal = O_0;
			MultiChoiceMenuItem::init(name, str, initVal, sizeofArray(str), baseVal);
		}

		void select(View *view, const InputEvent &e)
		{
			multiChoiceView.init(this, !e.isPointer());
			multiChoiceView.place(Gfx::viewportRect());
			modalView = &multiChoiceView;
		}
	};

	struct GameOrientationMenuItem : public OrientationMenuItem
	{
		constexpr GameOrientationMenuItem() { }
		void init()
		{
			OrientationMenuItem::init("Orientation", optionGameOrientation);
		}

		void doSet(int val)
		{
			optionGameOrientation.val = convertMenuValueToOption(val);
			logMsg("set game orientation: %s", Gfx::orientationName(int(optionGameOrientation)));
		}
	} gameOrientation;

	struct MenuOrientationMenuItem : public OrientationMenuItem
	{
		constexpr MenuOrientationMenuItem() { }
		void init()
		{
			OrientationMenuItem::init("Orientation", optionMenuOrientation);
		}

		void doSet(int val)
		{
			optionMenuOrientation.val = convertMenuValueToOption(val);
			if(!Gfx::setValidOrientations(optionMenuOrientation, 1))
				onViewChange();
			logMsg("set menu orientation: %s", Gfx::orientationName(int(optionMenuOrientation)));
		}
	} menuOrientation;

	struct AspectRatioMenuItem : public MultiChoiceMenuItem
	{
		constexpr AspectRatioMenuItem() { }
		void init()
		{
			static const char *str[] = { systemAspectRatioString, "1:1", "Full Screen" };
			MultiChoiceMenuItem::init("Aspect Ratio", str, optionAspectRatio, sizeofArray(str));
		}

		void select(View *view, const InputEvent &e)
		{
			multiChoiceView.init(this, !e.isPointer());
			multiChoiceView.place(Gfx::viewportRect());
			modalView = &multiChoiceView;
		}

		void doSet(int val)
		{
			optionAspectRatio.val = val;
			logMsg("set aspect ratio: %d", int(optionAspectRatio));
			placeGameView();
		}
	} aspectRatio;

	struct ZoomMenuItem : public MultiChoiceMenuItem
	{
		constexpr ZoomMenuItem() { }
		void init()
		{
			static const char *str[] = { "100%", "90%", "80%", "70%", "Integer-only" };
			int val = 0;
			switch(optionImageZoom.val)
			{
				bcase 100: val = 0;
				bcase 90: val = 1;
				bcase 80: val = 2;
				bcase 70: val = 3;
				bcase optionImageZoomIntegerOnly: val = 4;
			}
			MultiChoiceMenuItem::init("Zoom", str, val, sizeofArray(str));
		}

		void select(View *view, const InputEvent &e)
		{
			multiChoiceView.init(this, !e.isPointer());
			multiChoiceView.place(Gfx::viewportRect());
			modalView = &multiChoiceView;
		}

		void doSet(int val)
		{
			switch(val)
			{
				bcase 0: optionImageZoom.val = 100;
				bcase 1: optionImageZoom.val = 90;
				bcase 2: optionImageZoom.val = 80;
				bcase 3: optionImageZoom.val = 70;
				bcase 4: optionImageZoom.val = optionImageZoomIntegerOnly;
			}
			logMsg("set image zoom: %d", int(optionImageZoom));
			placeGameView();
		}
	} zoom;

	struct DPIMenuItem : public MultiChoiceMenuItem
	{
		constexpr DPIMenuItem() { }
		void init()
		{
			static const char *str[] = { "Auto", "96", "120", "160", "220", "240", "265", "320" };
			uint init = 0;
			switch(optionDPI)
			{
				bcase 96: init = 1;
				bcase 120: init = 2;
				bcase 160: init = 3;
				bcase 220: init = 4;
				bcase 240: init = 5;
				bcase 265: init = 6;
				bcase 320: init = 7;
			}
			assert(init < sizeofArray(str));
			MultiChoiceMenuItem::init("DPI Override", str, init, sizeofArray(str));
		}

		void select(View *view, const InputEvent &e)
		{
			multiChoiceView.init(this, !e.isPointer());
			multiChoiceView.place(Gfx::viewportRect());
			modalView = &multiChoiceView;
		}

		void doSet(int val)
		{
			switch(val)
			{
				bdefault: optionDPI.val = 0;
				bcase 1: optionDPI.val = 96;
				bcase 2: optionDPI.val = 120;
				bcase 3: optionDPI.val = 160;
				bcase 4: optionDPI.val = 220;
				bcase 5: optionDPI.val = 240;
				bcase 6: optionDPI.val = 265;
				bcase 7: optionDPI.val = 320;
			}
			Base::setDPI(optionDPI);
			logMsg("set DPI: %d", (int)optionDPI);
			onViewChange();
		}
	} dpi;

	struct ImgFilterMenuItem : public MultiChoiceMenuItem
	{
		constexpr ImgFilterMenuItem() { }
		void init()
		{
			static const char *str[] = { "None", "Linear" };
			MultiChoiceMenuItem::init("Image Filter", str, optionImgFilter, sizeofArray(str));
		}

		void select(View *view, const InputEvent &e)
		{
			multiChoiceView.init(this, !e.isPointer());
			multiChoiceView.place(Gfx::viewportRect());
			modalView = &multiChoiceView;
		}

		void doSet(int val)
		{
			optionImgFilter.val = val;
			vidImg.setFilter(val);
		}
	} imgFilter;

	struct RelativePointerDecelMenuItem : public MultiChoiceMenuItem
	{
		constexpr RelativePointerDecelMenuItem() { }
		void init()
		{
			static const char *str[] = { "Low", "Med", "High" };
			int init = 0;
			if(optionRelPointerDecel == optionRelPointerDecelLow)
				init = 0;
			if(optionRelPointerDecel == optionRelPointerDecelMed)
				init = 1;
			if(optionRelPointerDecel == optionRelPointerDecelHigh)
				init = 2;
			MultiChoiceMenuItem::init("Trackball Sensitivity", str, init, sizeofArray(str));
		}

		void select(View *view, const InputEvent &e)
		{
			multiChoiceView.init(this, !e.isPointer());
			multiChoiceView.place(Gfx::viewportRect());
			modalView = &multiChoiceView;
		}

		void doSet(int val)
		{
			#if defined(CONFIG_BASE_ANDROID)
			if(val == 0)
				optionRelPointerDecel.val = optionRelPointerDecelLow;
			else if(val == 1)
				optionRelPointerDecel.val = optionRelPointerDecelMed;
			else if(val == 2)
				optionRelPointerDecel.val = optionRelPointerDecelHigh;
			#endif
		}
	} relativePointerDecel;


	void loadVideoItems(MenuItem *item[], uint &items)
	{
		name_ = "Video Options";
		if(!optionFrameSkip.isConst) { frameSkip.init(); item[items++] = &frameSkip; }
		#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
		directTexture.init(optionDirectTexture); item[items++] = &directTexture;
		glSyncHack.init(); item[items++] = &glSyncHack;
		#endif
		aspectRatio.init(); item[items++] = &aspectRatio;
		imgFilter.init(); item[items++] = &imgFilter;
		zoom.init(); item[items++] = &zoom;
		if(!optionGameOrientation.isConst) { gameOrientation.init(); item[items++] = &gameOrientation; }
	}

	void loadAudioItems(MenuItem *item[], uint &items)
	{
		name_ = "Audio Options";
		snd.init(optionSound); item[items++] = &snd;
		if(!optionSoundRate.isConst) { audioRate.init(); item[items++] = &audioRate; }
	}

	void loadInputItems(MenuItem *item[], uint &items)
	{
		name_ = "Input Options";
		if(!optionTouchCtrl.isConst)
		{
			touchCtrl.init(); item[items++] = &touchCtrl;
			touchCtrlConfig.init(); item[items++] = &touchCtrlConfig;
		}
		#if !defined(CONFIG_BASE_IOS) && !defined(CONFIG_BASE_PS3)
		buttonConfig.init(); item[items++] = &buttonConfig;
		#endif
		#ifdef CONFIG_BASE_IOS_ICADE
		iCade.init(); item[items++] = &iCade;
		iCadeButtonConfig.init(); item[items++] = &iCadeButtonConfig;
		#endif
		#ifdef CONFIG_BLUETOOTH
		wiiButtonConfig.init(); item[items++] = &wiiButtonConfig;
		iCPButtonConfig.init(); item[items++] = &iCPButtonConfig;
		zeemoteButtonConfig.init(); item[items++] = &zeemoteButtonConfig;
		btScanSecs.init(); item[items++] = &btScanSecs;
		#endif
		if(!optionRelPointerDecel.isConst) { relativePointerDecel.init(); item[items++] = &relativePointerDecel; }
	}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		name_ = "System Options";
		autoSaveState.init(); item[items++] = &autoSaveState;
	}

	void loadGUIItems(MenuItem *item[], uint &items)
	{
		name_ = "GUI Options";
		if(!optionPauseUnfocused.isConst) { pauseUnfocused.init(optionPauseUnfocused); item[items++] = &pauseUnfocused; }
		if(!optionNotificationIcon.isConst) { notificationIcon.init(); item[items++] = &notificationIcon; }
		if(!optionTitleBar.isConst) { navView.init(); item[items++] = &navView; }
		if(!View::needsBackControlIsConst) { backNav.init(); item[items++] = &backNav; }
		rememberLastMenu.init(); item[items++] = &rememberLastMenu;
		if(!optionLargeFonts.isConst) { largeFonts.init(optionLargeFonts); item[items++] = &largeFonts; }
		if(!optionDPI.isConst) { dpi.init(); item[items++] = &dpi; }
		if(!optionMenuOrientation.isConst) { menuOrientation.init(); item[items++] = &menuOrientation; }
		#ifndef CONFIG_ENV_WEBOS
		altGamepadConfirm.init(); item[items++] = &altGamepadConfirm;
		#endif
	}

public:
	constexpr OptionView(): BaseMenuView("Options") { }
};
