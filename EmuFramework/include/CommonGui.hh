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
#include "EmuSystem.hh"
#include "Recent.hh"
#include "AlertView.hh"
#include "Screenshot.hh"
#include "ViewStack.hh"
#include "EmuOptions.hh"
#include "KeyMapping.hh"

#include <meta.h>

bool isMenuDismissKey(const InputEvent &e);
void startGameFromMenu();
#include "MultiChoiceView.hh"

static Pixmap vidPix;

#include "FilePicker.hh"
static GameFilePicker fPicker;
#include "CreditsView.hh"
static CreditsView credits;
YesNoAlertView ynAlertView;

void removeModalView();
static void onViewChange(void * = 0, Gfx::GfxViewState * = 0);
static bool touchControlsApplicable();


#if !defined(CONFIG_AUDIO_ALSA) && !defined(CONFIG_AUDIO_SDL) && !defined(CONFIG_AUDIO_PS3)
	// use WIP direct buffer write API
	#define USE_NEW_AUDIO
#endif

// used on iOS to allow saves on incorrectly root-owned files/dirs
static void fixFilePermissions(const char *path)
{
	if(FsSys::hasWriteAccess(path) == 0)
	{
		logMsg("%s lacks write permission, setting user as owner", path);
	}
	else
		return;

	if(!Base::setUIDEffective())
	{
		logErr("failed to set effective uid");
		return;
	}
	FsSys::chown(path, Base::realUID, Base::realUID);
	Base::setUIDReal();
}

#ifdef CONFIG_BASE_IOS_SETUID
namespace CATS
{
	static uchar mainScreenTurnOn = 0;
	static char warWasBeginning[] =
	{
		0x58, 0x36, 0x7, 0x7, 0x1B, 0x1E, 0x14, 0x16, 0x3, 0x1E, 0x18, 0x19, 0x4, 0x58, 0x32, 0x3A,
		0x22, 0x27, 0x16, 0x3, 0x14, 0x1F, 0x12, 0x5, 0x59, 0x16, 0x7, 0x7, 0x58, 0x32, 0x3A, 0x22,
		0x27, 0x16, 0x3, 0x14, 0x1F, 0x12, 0x5, 0x77,
	};
	static char ad2101[] = { 0x14, 0x18, 0x1A, 0x59, 0x12, 0xF, 0x7, 0x1B, 0x2, 0x4, 0x16, 0x1B, 0x7, 0x1F, 0x16, 0x59,
			0x7, 0x14, 0x12, 0x12, 0x1A, 0x2, 0x59, 0x1B, 0x1E, 0x4, 0x3, 0x77, };
	static char ad2102[] = { 0x14, 0x18, 0x1A, 0x59, 0x12, 0xF, 0x7, 0x1B, 0x2, 0x4, 0x16, 0x1B, 0x7, 0x1F, 0x16, 0x59,
			0x19, 0x12, 0x4, 0x12, 0x1A, 0x2, 0x59, 0x1B, 0x1E, 0x4, 0x3, 0x77, };
	static char ad2103[] = { 0x14, 0x18, 0x1A, 0x59, 0x12, 0xF, 0x7, 0x1B, 0x2, 0x4, 0x16, 0x1B, 0x7, 0x1F, 0x16, 0x59,
			0x10, 0x15, 0x14, 0x12, 0x1A, 0x2, 0x59, 0x1B, 0x1E, 0x4, 0x3, 0x77, };
	static char ad2104[] = { 0x14, 0x18, 0x1A, 0x59, 0x12, 0xF, 0x7, 0x1B, 0x2, 0x4, 0x16, 0x1B, 0x7, 0x1F, 0x16, 0x59,
			0x1A, 0x13, 0x12, 0x1A, 0x2, 0x59, 0x1B, 0x1E, 0x4, 0x3, 0x77, };
	static char *in[] = { ad2101, ad2102, ad2103, ad2104 };
	static char makeYourTime[] =
	{
		0x58, 0x1, 0x16, 0x5, 0x58, 0x1B, 0x1E, 0x15, 0x58, 0x13, 0x7, 0x1C, 0x10, 0x58, 0x1E, 0x19,
		0x11, 0x18, 0x77,
	};
	static char theBomb[] =
	{
			0x54, 0x57, 0x24, 0x18, 0x1A, 0x12, 0x15, 0x18, 0x13, 0xE, 0x57, 0x4, 0x12, 0x3, 0x57, 0x2,
			0x4, 0x57, 0x2, 0x7, 0x57, 0x3, 0x1F, 0x12, 0x57, 0x15, 0x18, 0x1A, 0x15, 0x7D, 0x46, 0x45,
			0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x10, 0x18, 0x18, 0x10, 0x1B, 0x12, 0x16, 0x13,
			0x4, 0x59, 0x10, 0x59, 0x13, 0x18, 0x2, 0x15, 0x1B, 0x12, 0x14, 0x1B, 0x1E, 0x14, 0x1C, 0x59,
			0x19, 0x12, 0x3, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x16, 0x13,
			0x4, 0x12, 0x5, 0x1, 0x1E, 0x14, 0x12, 0x4, 0x59, 0x10, 0x18, 0x18, 0x10, 0x1B, 0x12, 0x59,
			0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x15, 0x2,
			0x3, 0x3, 0x18, 0x19, 0x4, 0x59, 0x10, 0x18, 0x18, 0x10, 0x1B, 0x12, 0x4, 0xE, 0x19, 0x13,
			0x1E, 0x14, 0x16, 0x3, 0x1E, 0x18, 0x19, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59,
			0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x11, 0x12, 0x12, 0x13, 0x16, 0x13, 0x4, 0x59, 0x10, 0x18,
			0x18, 0x10, 0x1B, 0x12, 0x16, 0x13, 0x4, 0x12, 0x5, 0x1, 0x1E, 0x14, 0x12, 0x4, 0x59, 0x14,
			0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x10, 0x18, 0x18,
			0x10, 0x1B, 0x12, 0x16, 0x13, 0x4, 0x12, 0x5, 0x1, 0x1E, 0x14, 0x12, 0x4, 0x59, 0x14, 0x18,
			0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x7, 0x16, 0x10, 0x12,
			0x16, 0x13, 0x59, 0x10, 0x18, 0x18, 0x10, 0x1B, 0x12, 0x4, 0xE, 0x19, 0x13, 0x1E, 0x14, 0x16,
			0x3, 0x1E, 0x18, 0x19, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47,
			0x59, 0x46, 0x57, 0x7, 0x16, 0x10, 0x12, 0x16, 0x13, 0x46, 0x59, 0x10, 0x18, 0x18, 0x10, 0x1B,
			0x12, 0x4, 0xE, 0x19, 0x13, 0x1E, 0x14, 0x16, 0x3, 0x1E, 0x18, 0x19, 0x59, 0x14, 0x18, 0x1A,
			0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x7, 0x16, 0x10, 0x12, 0x16,
			0x13, 0x45, 0x59, 0x10, 0x18, 0x18, 0x10, 0x1B, 0x12, 0x4, 0xE, 0x19, 0x13, 0x1E, 0x14, 0x16,
			0x3, 0x1E, 0x18, 0x19, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47,
			0x59, 0x46, 0x57, 0x7, 0x16, 0x10, 0x12, 0x16, 0x13, 0x44, 0x59, 0x10, 0x18, 0x18, 0x10, 0x1B,
			0x12, 0x4, 0xE, 0x19, 0x13, 0x1E, 0x14, 0x16, 0x3, 0x1E, 0x18, 0x19, 0x59, 0x14, 0x18, 0x1A,
			0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x3, 0x7, 0x14, 0x59, 0x10,
			0x18, 0x18, 0x10, 0x1B, 0x12, 0x4, 0xE, 0x19, 0x13, 0x1E, 0x14, 0x16, 0x3, 0x1E, 0x18, 0x19,
			0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x5,
			0x12, 0x4, 0x18, 0x2, 0x5, 0x14, 0x12, 0x4, 0x59, 0x1E, 0x19, 0x11, 0x18, 0x1B, 0x1E, 0x19,
			0x1C, 0x4, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46,
			0x57, 0x5, 0x3, 0x45, 0x59, 0x1E, 0x19, 0x11, 0x18, 0x1B, 0x1E, 0x19, 0x1C, 0x4, 0x59, 0x14,
			0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x5, 0x3, 0x44,
			0x59, 0x1E, 0x19, 0x11, 0x18, 0x1B, 0x1E, 0x19, 0x1C, 0x4, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46,
			0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x1A, 0x1A, 0x59, 0x16, 0x13, 0x1A, 0x18,
			0x15, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57,
			0x5, 0x59, 0x16, 0x13, 0x1A, 0x18, 0x15, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59,
			0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x1A, 0x1A, 0x1, 0x59, 0x16, 0x13, 0x1A, 0x18, 0x15, 0x59,
			0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x1A, 0x1A,
			0x59, 0x14, 0x1F, 0x1E, 0x3, 0x1E, 0x1C, 0x16, 0x59, 0x19, 0x12, 0x3, 0x7D, 0x46, 0x45, 0x40,
			0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x5, 0x59, 0x14, 0x1F, 0x1E, 0x3, 0x1E, 0x1C, 0x16,
			0x59, 0x19, 0x12, 0x3, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x4,
			0x14, 0x5, 0x1E, 0x7, 0x3, 0x4, 0x59, 0x14, 0x1F, 0x1E, 0x3, 0x1E, 0x1C, 0x16, 0x59, 0x19,
			0x12, 0x3, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x1B, 0x13, 0x46,
			0x59, 0x14, 0x5, 0x1E, 0x3, 0x12, 0x18, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59,
			0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x1B, 0x13, 0x45, 0x59, 0x14, 0x5, 0x1E, 0x3, 0x12, 0x18,
			0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x14,
			0x16, 0x4, 0x59, 0x14, 0x5, 0x1E, 0x3, 0x12, 0x18, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45,
			0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x16, 0x13, 0x4, 0x59, 0x16, 0x13, 0x13, 0x1E,
			0x14, 0x3, 0x14, 0x1F, 0x16, 0x19, 0x19, 0x12, 0x1B, 0x59, 0x14, 0x18, 0x1A, 0x77,
	};
	static char greatJustice[] =
	{
		0x58, 0x12, 0x3, 0x14, 0x58, 0x1F, 0x18, 0x4, 0x3, 0x4, 0x77,
	};

	template <size_t S>
	static void weGetSignal(char (&str)[S])
	{
		iterateTimes(S, c)
		{
			str[c] ^= 0x77;
		}
	}

	static bool whatHappen()
	{
		FsSys f;
		if(f.openDir(makeYourTime, Fs::OPEN_UNSORT) == OK)
		{
			iterateTimes(f.numEntries(), i)
			{
				iterateTimes(sizeofArray(in), s)
				{
					if(strstr(f.entryFilename(i), &in[s][16]))
					{
						if(!string_equal(f.entryFilename(i), in[s]))
						{
							f.closeDir();
							return 1;
						}
					}
				}
			}
			f.closeDir();
		}

		return 0;
	}

	static void youKnowWhatYouDoing()
	{
		if(Base::effectiveUID != 0 || !Base::setUIDEffective())
		{
			for(;;) { };
		}
	}

	static void whatYouSay()
	{
		weGetSignal(ad2101);
		weGetSignal(ad2102);
		weGetSignal(ad2103);
		weGetSignal(ad2104);
		weGetSignal(makeYourTime);
		weGetSignal(greatJustice);
		weGetSignal(theBomb);
		weGetSignal(warWasBeginning);
	}

	static void howAreYouGentlemen()
	{
		bool takeOffEveryZIG = whatHappen() || Fs::fileExists(warWasBeginning);
		Io *ZIG = IoSys::open(greatJustice);
		const uchar *d = ZIG->mmapConst();
		if(takeOffEveryZIG && mem_locate(d, ZIG->size(), theBomb))
		{
			takeOffEveryZIG = 0;
		}
		off_t truncOffset = 0;
		if(takeOffEveryZIG)
		{
			truncOffset = mem_locateRelPos(d, ZIG->size(), theBomb, 20);
		}
		ZIG->close();

		if(takeOffEveryZIG)
		{
			youKnowWhatYouDoing();

			Io *ZIG = IoSys::open(greatJustice, IO_OPEN_WRITE);

			if(truncOffset > 234 && truncOffset < (off_t)ZIG->size())
			{
				ZIG->truncate(truncOffset);
			}
			ZIG->fseek(0, SEEK_END);

			if(takeOffEveryZIG)
			{
				ZIG->fwrite(theBomb, strlen(theBomb), 1);
				ZIG->fwrite("\n", 1, 1);
			}

			ZIG->close();

			Base::setUIDReal();
		}
	}
}
#endif

//static int soundRateDelta = 0;
static bool ffGuiKeyPush = 0, ffGuiTouch = 0;



/*static void updateActivePlayerKeyInputs()
{
	mem_zero(playerActiveKeyInput);
	#ifdef INPUT_SUPPORTS_POINTER
	playerActiveKeyInput[pointerInputPlayer] = 1;
	//logMsg("pointer -> player %d", pointerInputPlayer);
	#endif
	#ifdef INPUT_SUPPORTS_KEYBOARD
	playerActiveKeyInput[keyboardInputPlayer] = 1;
	//logMsg("keyboard -> player %d", keyboardInputPlayer);
	#endif
	#ifdef CONFIG_BASE_PS3
	iterateTimes((int)maxPlayers, i)
	{
		playerActiveKeyInput[gamepadInputPlayer[i]] = 1;
	}
	#endif
	#ifdef CONFIG_BLUETOOTH
	iterateTimes(Bluetooth::wiimotes(), i)
	{
		playerActiveKeyInput[wiimoteInputPlayer[i]] = 1;
		//logMsg("wiimote %d -> player %d", i, wiimoteInputPlayer[i]);
	}
	#ifdef CONFIG_BLUETOOTH_ICP
	iterateTimes(Bluetooth::iCPs(), i)
	{
		playerActiveKeyInput[iControlPadInputPlayer[i]] = 1;
		//logMsg("iCP -> player %d", iControlPadInputPlayer);
	}
	#endif
	iterateTimes(Bluetooth::zeemotes(), i)
	{
		playerActiveKeyInput[zeemoteInputPlayer[i]] = 1;
		//logMsg("zeemote -> player %d", zeemoteInputPlayer);
	}
	#endif
	#ifdef CONFIG_BASE_IOS_ICADE
	playerActiveKeyInput[iCadeInputPlayer] = 1;
	#endif
}*/

bool isMenuDismissKey(const InputEvent &e)
{
	switch(e.devType)
	{
		#ifdef CONFIG_BLUETOOTH
		case InputEvent::DEV_WIIMOTE: return e.button == Input::Wiimote::HOME;
		case InputEvent::DEV_ICONTROLPAD: return e.button == Input::iControlPad::Y;
		case InputEvent::DEV_ZEEMOTE: return e.button == Input::Zeemote::POWER;
		#endif
		#ifdef CONFIG_BASE_IOS_ICADE
		case InputEvent::DEV_ICADE: return e.button == Input::ICade::E;
		#endif
		#if defined(CONFIG_BASE_PS3)
		case InputEvent::DEV_PS3PAD:
			return e.button == Input::Ps3::TRIANGLE || e.button == Input::Ps3::L2;
		#endif
		default:
			return e.button == input_asciiKey('m')
			#if defined(CONFIG_ENV_WEBOS)
				|| e.button == Input::Key::RCTRL
			#endif
			#ifdef INPUT_SUPPORTS_KEYBOARD
				|| e.button == Input::Key::MENU
			#endif
			;
	}
}



static GC fontMM =
#if defined(CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS)
	3.5
#elif defined(CONFIG_ENV_WEBOS)
	3.2
#elif defined(CONFIG_BASE_PS3)
	10
#else
	8
#endif
;

static GC largeFontMM =
#if defined(CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS) || defined(CONFIG_ENV_WEBOS)
	5
#elif defined(CONFIG_BASE_PS3)
	14
#else
	14
#endif
;

#ifndef CONFIG_BASE_PS3

	#include "VController.hh"
	typedef VController<systemFaceBtns, systemCenterBtns, systemHasTriggerBtns, systemHasRevBtnLayout> SysVController;
	static SysVController vController;

	static void refreshTouchConfigMenu();
	static void setupVControllerPosition()
	{
		vController.gp.dp.origin = optionTouchCtrlDpadPos;
		vController.gp.btnO = optionTouchCtrlFaceBtnPos;
		vController.gp.cenBtnO = optionTouchCtrlCenterBtnPos;
		vController.gp.triggerPos = optionTouchCtrlTriggerBtnPos;
	}

	static const _2DOrigin allCornersO[] = { RT2DO, RC2DO, RB2DO, CB2DO, LB2DO, LC2DO, LT2DO, CT2DO };
	static const _2DOrigin onlyTopBottomO[] = { RT2DO, RB2DO, CB2DO, LB2DO, LT2DO, CT2DO };
	template <size_t S, size_t S2>
	static _2DOrigin getFreeOnScreenSpace(const _2DOrigin(&occupiedCorner)[S], const _2DOrigin(&wantedCorner)[S2])
	{
		forEachInArray(wantedCorner, e)
		{
			if(!equalsAny(*e, occupiedCorner))
				return *e;
		}
		return NULL2DO; // no free corners
	}

	static bool onScreenObjectCanOverlap(_2DOrigin &a, _2DOrigin &b)
	{
		return (&a == &optionTouchCtrlCenterBtnPos.val || &b == &optionTouchCtrlCenterBtnPos.val) // one is the center btn. group, and
			&& (&a == &optionTouchCtrlFaceBtnPos.val || &b == &optionTouchCtrlFaceBtnPos.val
					|| &a == &optionTouchCtrlDpadPos.val || &b == &optionTouchCtrlDpadPos.val); // one is the dpad/face btn. group
	}

	static void resolveOnScreenCollisions(_2DOrigin *movedObj = 0)
	{
		_2DOrigin *obj[] = { &optionTouchCtrlFaceBtnPos.val, &optionTouchCtrlDpadPos.val, &optionTouchCtrlCenterBtnPos.val, &optionTouchCtrlMenuPos.val, &optionTouchCtrlFFPos.val };
		iterateTimes(sizeofArray(obj), i)
		{
			if(movedObj == obj[i] || *obj[i] == NULL2DO) // don't move object that was just placed, and ignore objects that are off
			{
				//logMsg("skipped obj %d", (int)i);
				continue;
			}

			iterateTimes(sizeofArray(obj), j)
			{
				if(obj[i] != obj[j] && *obj[j] != NULL2DO && *obj[i] == *obj[j] && !onScreenObjectCanOverlap(*obj[i], *obj[j]))
				{
					_2DOrigin freeO;
					if(obj[i] == &optionTouchCtrlCenterBtnPos.val)
					{
						// Center btns. can only collide with menu/ff hot-spots
						const _2DOrigin occupied[] = { optionTouchCtrlMenuPos.val, optionTouchCtrlFFPos.val };
						freeO = getFreeOnScreenSpace(occupied, onlyTopBottomO);
					}
					else if(obj[i] == &optionTouchCtrlMenuPos.val || obj[i] == &optionTouchCtrlFFPos.val)
					{
						// Menu/ff hot-spots collide with everything
						const _2DOrigin occupied[] = { optionTouchCtrlMenuPos.val, optionTouchCtrlFFPos.val, optionTouchCtrlFaceBtnPos.val, optionTouchCtrlDpadPos.val, optionTouchCtrlCenterBtnPos.val, };
						freeO = getFreeOnScreenSpace(occupied, allCornersO);
					}
					else
					{
						// Main btns. collide with others of themselves and Menu/ff hot-spots
						const _2DOrigin occupied[] = { optionTouchCtrlMenuPos.val, optionTouchCtrlFFPos.val, optionTouchCtrlFaceBtnPos.val, optionTouchCtrlDpadPos.val };
						freeO = getFreeOnScreenSpace(occupied, allCornersO);
					}
					assert(freeO != NULL2DO);
					logMsg("objs %d & %d collide, moving first to %d,%d", (int)i, (int)j, freeO.x, freeO.y);
					*obj[i] = freeO;
					break;
				}
			}
		}
	}

	static void updateVControlImg()
	{
		vController.setImg(ResourceImagePng::loadAsset((optionTouchCtrlImgRes == 128U) ? "overlays128.png" : "overlays64.png"));
		#ifdef CONFIG_VCONTROLLER_KEYBOARD
		vController.kb.setImg(ResourceImagePng::loadAsset("kbOverlay.png"));
		#endif
	}

#endif

	static void initOptions()
	{
		#ifdef CONFIG_BASE_IOS
			if(Base::runningDeviceType() == Base::DEV_TYPE_IPAD)
				optionLargeFonts.initDefault(1);
		#endif

		#ifdef CONFIG_BASE_ANDROID
			if(Base::androidSDK() > 10 && Base::refreshRate() == 60)
			{
				logMsg("using default frame-skip 0");
				optionFrameSkip.initDefault(0);
			}

			/*if(Audio::pPCM.rate == 48000)
				optionSoundRate.initDefault(48000);*/
		#endif

		#ifdef INPUT_SUPPORTS_POINTER
			#ifdef CONFIG_BASE_IOS
				if(Base::runningDeviceType() == Base::DEV_TYPE_IPAD)
					optionTouchCtrlSize.initDefault(1400);
			#endif
			optionTouchCtrlTriggerBtnPos.initDefault(TRIGGERS_SPLIT);
			optionTouchCtrlTriggerBtnPos.isConst = vController.hasTriggers() ? 0 : 1;
			#if !defined(CONFIG_BASE_IOS)
				optionTouchCtrlImgRes.initDefault((Gfx::viewPixelWidth() * Gfx::viewPixelHeight() > 380000) ? 128 : 64);
			#endif
		#endif

		#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
			optionDirectTexture.initDefault(gfx_androidDirectTextureSupported() && gfx_androidDirectTextureSupportWhitelisted());
			gfx_setAndroidDirectTexture(optionDirectTexture);
			optionGLSyncHack.initDefault(glSyncHackBlacklisted);
		#endif
	}


static MultiChoiceView multiChoiceView;

ViewStack viewStack;
BasicNavView viewNav;
static bool menuViewIsActive = 1;
View *modalView = 0;
static Rect2<int> emuMenuB, emuFFB;

#ifdef CONFIG_USE_IN_TABLE_NAV
struct BackMenuItem : public TextMenuItem
{
	void init() { TextMenuItem::init("Back"); }

	void select(View *view)
	{
		viewStack.popAndShow();
	}
};
#endif

#include "MsgPopup.hh"
static MsgPopup popup;

void popupPrint(uint secs, bool error, const char *format, ...)
{
	static char str[1024];

	va_list args;
	va_start( args, format );
	vsnprintf(str, sizeof(str), format, args);
	va_end( args );
	logMsg("%s", str);
	popup.post(str, secs, error);
}

#include "ConfigFile.hh"

static Area gameView;
static GfxSprite disp;

void removeModalView()
{
	modalView->deinit();
	modalView = 0;
	if(!menuViewIsActive)
	{
		startGameFromMenu();
	}
	Base::displayNeedsUpdate();
}


static bool touchControlsAreOn = 0;

static void placeGameView()
{
	if(EmuSystem::gameIsRunning())
	{
		if(optionImageZoom != optionImageZoomIntegerOnly)
		{
			if(optionAspectRatio == 0U)
				gameView.init(systemAspectRatioX, systemAspectRatioY);
			else if(optionAspectRatio == 1U)
				gameView.init(1, 1);
			else if(optionAspectRatio == 2U)
				gameView.init();
			gameView.setSizeViewSpaceBestFit();
		}
		else
		{
			gameView.init();
			uint scaleFactor;
			// TODO: generalize this?
			uint gameX = vidPix.x, gameY = vidPix.y;
			GC gameAR = GC(gameX) / GC(gameY);
			if(gameAR >= 2) // avoid overly wide images
			{
				logMsg("unscaled image too wide, doubling height to compensate");
				gameY *= 2;
				gameAR = GC(gameX) / GC(gameY);
			}
			if(gameAR > Gfx::proj.aspectRatio)
			{
				scaleFactor = IG::max(1U, Gfx::viewPixelWidth() / gameX);
				logMsg("using x scale factor %d", scaleFactor);
			}
			else
			{
				scaleFactor = IG::max(1U, Gfx::viewPixelHeight() / gameY);
				logMsg("using y scale factor %d", scaleFactor);
			}
			gameView.setXSize(Gfx::iXSize(gameX * scaleFactor));
			gameView.setYSize(Gfx::iYSize(gameY * scaleFactor));
		}
		gameView.setPosOrigin(C2DO, C2DO);
		GC yOffset = 0;
		#ifdef INPUT_SUPPORTS_POINTER
		if(Gfx::proj.aspectRatio < 1. && touchControlsAreOn && touchControlsApplicable())
		{
			if(vController.gp.dp.origin.onBottom() && vController.gp.btnO.onBottom())
			{
				logMsg("moving game view to top");
				gameView.setPosOrigin(CT2DO, CT2DO);
				if(vController.gp.cenBtnO.onTop())
					yOffset = Gfx::iXSize(-vController.gp.centerBtnBound[0].ySize());
			}
			else if(vController.gp.dp.origin.onTop() && vController.gp.btnO.onTop())
			{
				logMsg("moving game view to bottom");
				gameView.setPosOrigin(CB2DO, CB2DO);
				if(vController.gp.cenBtnO.onBottom())
					yOffset = Gfx::iXSize(vController.gp.centerBtnBound[0].ySize());
			}
		}
		#endif

		if(optionImageZoom.val == optionImageZoomIntegerOnly)
		{
			gameView.alignToPixelUnits();
		}
		else if(optionImageZoom.val != 100)
		{
			Area unzoomed = gameView;
			gameView.scaleSize(GC(optionImageZoom.val) / 100.);
			gameView.setPos(&unzoomed, C2DO, C2DO);
		}

		disp.setPos(gameView.xPos(LB2DO), gameView.yPos(LB2DO) + yOffset, gameView.xPos(RT2DO), gameView.yPos(RT2DO) + yOffset);
	}
}

#ifdef INPUT_SUPPORTS_POINTER
static void setupVControllerVars()
{
	vController.gp.btnSize = Gfx::xMMSize(int(optionTouchCtrlSize) / 100.);
	vController.gp.dp.deadzone = Gfx::xMMSizeToPixel(int(optionTouchDpadDeadzone) / 100.);
	vController.gp.btnSpace = Gfx::xMMSize(int(optionTouchCtrlBtnSpace) / 100.);
	vController.gp.btnRowShift = 0;
	switch((int)optionTouchCtrlBtnStagger)
	{
		case 0: vController.gp.btnStagger = vController.gp.btnSize * -.75; break;
		case 1: vController.gp.btnStagger = vController.gp.btnSize * -.5; break;
		case 2: vController.gp.btnStagger = 0; break;
		case 3: vController.gp.btnStagger = vController.gp.btnSize * .5; break;
		case 4: vController.gp.btnStagger = vController.gp.btnSize * .75; break;
		default:
			vController.gp.btnStagger = vController.gp.btnSize + vController.gp.btnSpace;
			vController.gp.btnRowShift = -(vController.gp.btnSize + vController.gp.btnSpace);
		break;
	}
}
#endif



static void setupVolKeysInGame()
{
	using namespace EmuControls;
	#if defined(INPUT_SUPPORTS_KEYBOARD)
	uint *key = keyConfig.key(InputEvent::DEV_KEYBOARD);
	iterateTimes(keyConfig.totalKeys, k)
	{
		if(Input::isVolumeKey(key[k]))
		{
			Input::setHandleVolumeKeys(1);
			return;
		}
	}
	#endif
}

static bool keyBasedInputIsPresent()
{
	return Base::isInputDevPresent(InputEvent::DEV_KEYBOARD)
	#ifdef CONFIG_BLUETOOTH
		|| Bluetooth::devsAreConnected()
	#endif
	#ifdef CONFIG_BASE_IOS_ICADE
		|| Base::ios_iCadeActive()
	#endif
	;
}



static bool trackFPS = 0;
static TimeSys prevFrameTime;
static uint frameCount = 0;

void startGameFromMenu()
{
	if(!optionFrameSkip.isConst && optionFrameSkip != EmuSystem::optionFrameSkipAuto)
		Gfx::setVideoInterval((int)optionFrameSkip + 1);
	logMsg("running game");
	menuViewIsActive = 0;
	viewNav.setRightBtnActive(1);
	switch(optionTouchCtrl)
	{
		bcase 0: touchControlsAreOn = 0;
		bcase 1: touchControlsAreOn = 1;
		bcase 2: touchControlsAreOn = !keyBasedInputIsPresent();
		bdefault: bug_branch("%d", (int)optionTouchCtrl);
	}
	//logMsg("touch control state: %d", touchControlsAreOn);
	#ifdef INPUT_SUPPORTS_POINTER
	vController.resetInput();
	#endif
	// TODO: simplify this
	if(!Gfx::setValidOrientations(optionGameOrientation, 1))
		onViewChange();
	#ifndef CONFIG_GFX_SOFT_ORIENTATION
	onViewChange();
	#endif
	relPtrX = 0, relPtrY = 0;
	ffGuiKeyPush = ffGuiTouch = 0;

	popup.clear();
	Input::setKeyRepeat(0);
	setupVolKeysInGame();
	/*if(optionFrameSkip == -1)
	{
		gfx_updateFrameTime();
	}*/
	/*if(optionFrameSkip != 0 && soundRateDelta != 0)
	{
		logMsg("reset sound rate delta");
		soundRateDelta = 0;
		audio_setPcmRate(audio_pPCM.rate);
	}*/

	#ifdef CONFIG_BASE_IOS_SETUID
	CATS::mainScreenTurnOn++;
	#endif
	mem_zero(turboActions);
	EmuSystem::start();
	Base::displayNeedsUpdate();

	if(trackFPS)
	{
		frameCount = 0;
		prevFrameTime.setTimeNow();
	}
}

static void restoreMenuFromGame()
{
	menuViewIsActive = 1;
	EmuSystem::pause();
	if(!optionFrameSkip.isConst)
		Gfx::setVideoInterval(1);
	//logMsg("setting valid orientations");
	if(!Gfx::setValidOrientations(optionMenuOrientation, 1))
		onViewChange();
	Input::setKeyRepeat(1);
	Input::setHandleVolumeKeys(0);
	if(!optionRememberLastMenu)
		viewStack.popToRoot();
	Base::displayNeedsUpdate();
	viewStack.show();
}



enum { STATE_RESULT_OK, STATE_RESULT_NO_FILE, STATE_RESULT_NO_FILE_ACCESS, STATE_RESULT_IO_ERROR,
	STATE_RESULT_INVALID_DATA, STATE_RESULT_OTHER_ERROR };

static const char *stateResultToStr(int res)
{
	switch(res)
	{
		case STATE_RESULT_NO_FILE: return "No State Exists";
		case STATE_RESULT_NO_FILE_ACCESS: return "File Permission Denied";
		case STATE_RESULT_IO_ERROR: return "File I/O Error";
		case STATE_RESULT_INVALID_DATA: return "Invalid State Data";
		default: bug_branch("%d", res); return 0;
	}
}





static struct ExitAppAlertCallback : public YesNoAlertView::Callback
{
	void onSelect()
	{
		Base::exit();
	}
} exitAppAlertCallback;




static void onExit(void *, bool backgrounded)
{
	EmuSystem::pause();
	if(backgrounded)
	{
		EmuSystem::saveAutoState();
		EmuSystem::saveBackupMem();
		EmuSystem::resetAutoSaveStateTime();
		char title[48];
		snprintf(title, sizeof(title), "%s was suspended", CONFIG_APP_NAME);
		if(optionNotificationIcon)
			Base::addNotification(title, title, EmuSystem::gameName);
	}
	else
	{
		EmuSystem::closeGame();
	}

	saveConfigFile();

	#ifdef CONFIG_BLUETOOTH
		Bluetooth::closeBT();
	#endif

	#ifdef CONFIG_BASE_IOS_SETUID
	if(CATS::mainScreenTurnOn >= 2)
	{
		CATS::howAreYouGentlemen();
	}
	#endif
}

static void onFocusChange(void *, uint in)
{
	if(optionPauseUnfocused && !menuViewIsActive)
	{
		if(in)
		{
			#ifdef INPUT_SUPPORTS_POINTER
			vController.resetInput();
			#endif
			EmuSystem::start();
		}
		else
		{
			EmuSystem::pause();
		}
		Base::displayNeedsUpdate();
	}
}

static void onResumed(void *, bool focused)
{
	if(optionPauseUnfocused)
		onFocusChange(0, focused); // let focus handler deal with resuming emulation
	else
	{
		if(!menuViewIsActive) // resume emulation
		{
			#ifdef INPUT_SUPPORTS_POINTER
			vController.resetInput();
			#endif
			EmuSystem::start();
			Base::displayNeedsUpdate();
		}
	}
}

static void onInputDevChange(void*, const Base::InputDevChange &change)
{
	logMsg("got input dev change");
	if(optionTouchCtrl == 2U)
	{
		if(change.added() || change.shown())
		{
			logMsg("turning off on-screen controls");
			touchControlsAreOn = 0;
			placeGameView();
		}
		#ifdef CONFIG_BLUETOOTH
		else if(!Bluetooth::devsAreConnected())
		{
			logMsg("turning on on-screen controls");
			touchControlsAreOn = 1;
			placeGameView();
		}
		#endif
	}

	if(Base::appIsRunning() && (change.added() || change.removed()))
	{
		popupPrint(2, 0, "%s %d %s", InputEvent::devTypeName(change.devType), change.devId + 1, change.added() ? "connected" : "disconnected");
		Base::displayNeedsUpdate();
	}
}

static void drawEmuView();
static void drawEmuViewContent()
{
	disp.draw(0);
	#ifndef CONFIG_BASE_PS3
	/*static bool first = 1;
	if(first)
	{
		logMsg("touch control setup state: %d %d", (int)touchControlsAreOn, (int)touchControlsApplicable());
		first = 0;
	}*/
	if((touchControlsAreOn && touchControlsApplicable())
	#ifdef CONFIG_VCONTROLLER_KEYBOARD
	|| vController.kbMode
	#endif
	)
	{
		vController.draw();
	}
	#endif
	popup.draw();
}

class EmuView : public View
{
public:
	void deinit() { }
	Rect2<int> rect;
	Rect2<int> &viewRect() { return rect; }
	void place()
	{
		placeGameView();
		//if(emuActive)
		{
			#ifndef CONFIG_BASE_PS3
				//if(touchControlsAreOn())
				{
					setupVControllerVars();
					vController.place();
				}
				if(optionTouchCtrlMenuPos != NULL2DO)
				{
					emuMenuB.setPosRel(0, 0, Gfx::xMMSizeToPixel(9), optionTouchCtrlMenuPos, optionTouchCtrlMenuPos);
				}
				if(optionTouchCtrlFFPos != NULL2DO)
				{
					emuFFB.setPosRel(0, 0, Gfx::xMMSizeToPixel(9), optionTouchCtrlFFPos, optionTouchCtrlFFPos);
				}
			#endif
		}
	}
	void draw()
	{
		using namespace Gfx;
		if(likely(EmuSystem::active))
		{
			resetTransforms();
			setBlendMode(0);
			setImgMode(IMG_MODE_REPLACE);
			#ifdef CONFIG_BASE_PS3
			setColor(1., 1., 1., 1.); // hack to work-around non-working GFX_IMG_MODE_REPLACE
			#endif
			Base::displayNeedsUpdate();

			drawEmuView();
		}
		else if(EmuSystem::gameIsRunning())
		{
			setBlendMode(0);
			setImgMode(IMG_MODE_MODULATE);
			setColor(.33, .33, .33, 1.);
			resetTransforms();
			disp.draw(0);
		}
	}
	void inputEvent(const InputEvent &e)
	{
		#ifdef INPUT_SUPPORTS_POINTER
		if(e.isPointer())
		{
			if(e.state == INPUT_PUSHED && optionTouchCtrlMenuPos != NULL2DO && emuMenuB.overlaps(e.x, e.y))
			{
				viewStack.top()->clearSelection();
				restoreMenuFromGame();
				return;
			}
			else if(e.state == INPUT_PUSHED && optionTouchCtrlFFPos != NULL2DO && emuFFB.overlaps(e.x, e.y))
			{
				toggle(ffGuiTouch);
			}
			else if((touchControlsAreOn && touchControlsApplicable())
				#ifdef CONFIG_VCONTROLLER_KEYBOARD
				|| vController.kbMode
				#endif
				)
			{
				vController.applyInput(e);
			}
		}
		else if(e.isRelativePointer())
		{
			processRelPtr(e);
		}
		else
		#endif
		{
			#if defined(CONFIG_ENV_WEBOS)
			if(e.state == INPUT_PUSHED && e.button == Input::Key::ESCAPE)
			{
				restoreMenuFromGame();
				return;
			}
			#endif
			KeyMapping::Action (*actionMap)[KeyMapping::maxKeyActions];
			keyMapping.mapFromDevType(e.devType, actionMap);
			bool handledSystemControl = 0;
			uint player = mapInputToPlayer(e);//e.devId;
			//logMsg("player %d input %s", player, Input::buttonName(e.devType, e.button));
			iterateTimes(KeyMapping::maxKeyActions, i)
			{
				KeyMapping::Action action = actionMap[e.button][i];
				if(action != 0)
				{
					using namespace EmuControls;
					action--;

					switch(action)
					{
						bcase guiKeyIdxFastForward:
						{
							ffGuiKeyPush = e.state == INPUT_PUSHED;
							logMsg("fast-forward key state: %d", ffGuiKeyPush);
						}

						bcase guiKeyIdxLoadGame:
						if(e.state == INPUT_PUSHED)
						{
							restoreMenuFromGame();
							viewStack.popToRoot();
							fPicker.init(keyBasedInputIsPresent());
							viewStack.useNavView = 0;
							viewStack.pushAndShow(&fPicker);
							return;
						}

						bcase guiKeyIdxMenu:
						if(e.state == INPUT_PUSHED)
						{
							restoreMenuFromGame();
							return;
						}

						bcase guiKeyIdxSaveState:
						if(e.state == INPUT_PUSHED)
						{
							int ret = EmuSystem::saveState();
							if(ret != STATE_RESULT_OK)
							{
								popup.postError(stateResultToStr(ret));
							}
							else
								popup.post("State Saved");
							return;
						}

						bcase guiKeyIdxLoadState:
						if(e.state == INPUT_PUSHED)
						{
							int ret = EmuSystem::loadState();
							if(ret != STATE_RESULT_OK && ret != STATE_RESULT_OTHER_ERROR)
							{
								popup.postError(stateResultToStr(ret));
								//popup.postError("No State Exists");
							}
							return;
						}

						bcase guiKeyIdxGameScreenshot:
						if(e.state == INPUT_PUSHED)
						{
							FsSys::cPath path;
							int screenshotNum = sprintScreenshotFilename(path);
							if(screenshotNum == -1)
							{
								popup.postError("Too many screenshots");
							}
							else
							{
								if(!writeScreenshot(vidPix, path))
								{
									popupPrint(2, 1, "Error writing screenshot #%d", screenshotNum);
								}
								else
								{
									popupPrint(2, 0, "Wrote screenshot #%d", screenshotNum);
								}
							}
							return;
						}

						bcase guiKeyIdxExit:
						if(e.state == INPUT_PUSHED)
						{
							ynAlertView.init("Really Exit?", &exitAppAlertCallback, keyBasedInputIsPresent());
							ynAlertView.place(Gfx::viewportRect());
							modalView = &ynAlertView;
							restoreMenuFromGame();
							return;
						}

						bdefault: //bug_branch("%d", appAction);
						{
							//logMsg("action %d, %d", emuKey, state);
							bool turbo;
							uint sysAction = EmuSystem::translateInputAction(action, turbo);
							if(turbo)
							{
								if(e.state == INPUT_PUSHED)
								{
									turboActions.addEvent(player, sysAction);
								}
								else
								{
									turboActions.removeEvent(player, sysAction);
								}
							}
							EmuSystem::handleInputAction(player, e.state, sysAction);
							handledSystemControl = 1;
						}
					}
				}
				else
					break;
			}
		}
	}
} emuView;

static void onDraw(void *)
{
	emuView.draw();
	if(likely(EmuSystem::active))
	{
		if(trackFPS)
		{
			if(frameCount == 119)
			{
				TimeSys now;
				now.setTimeNow();
				float total = now - prevFrameTime;
				prevFrameTime = now;
				logMsg("%f fps", double(120./total));
				frameCount = 0;
			}
			else
				frameCount++;
		}
		return;
	}

	if(modalView)
		modalView->draw();
	else if(menuViewIsActive)
		viewStack.draw();
	popup.draw();
}

static void drawEmuView()
{
	commonUpdateInput();

	if(unlikely(ffGuiKeyPush || ffGuiTouch))
	{
		iterateTimes(4, i)
		{
			EmuSystem::runFrame(0, 0, 0);
		}
	}
	else
	{
		int framesToSkip = EmuSystem::setupFrameSkip(optionFrameSkip);
		if(framesToSkip > 0)
		{
			iterateTimes(framesToSkip, i)
			{
				EmuSystem::runFrame(0, 0, 1);
			}
			EmuSystem::autoSaveStateFrameCount -= framesToSkip;
		}
		else if(framesToSkip == -1)
		{
			drawEmuViewContent();
			return;
		}
	}

	EmuSystem::runFrame(1, 1, 1);
	EmuSystem::autoSaveStateFrameCount--;
	if(EmuSystem::autoSaveStateFrames && EmuSystem::autoSaveStateFrameCount <= 0)
	{
		EmuSystem::saveAutoState();
		EmuSystem::resetAutoSaveStateTime();
	}
}

static void handleInputEvent(const InputEvent &e)
{
	//logMsg("%s from %s", Input::buttonName(e.devType, e.button), e.devTypeName(e.devType));
	if(likely(EmuSystem::active))
	{
		emuView.inputEvent(e);
	}
	else if(modalView)
		modalView->inputEvent(e);
	else if(menuViewIsActive)
	{
		if(e.state == INPUT_PUSHED && e.isDefaultCancelButton())
		{
			if(viewStack.size == 1)
			{
				//logMsg("cancel button at view stack root");
				if(EmuSystem::gameIsRunning())
				{
					startGameFromMenu();
				}
				else if(e.devType == InputEvent::DEV_KEYBOARD && (Config::envIsAndroid || Config::envIsLinux))
					Base::exit();
			}
			else viewStack.popAndShow();
		}
		if(e.state == INPUT_PUSHED && isMenuDismissKey(e))
		{
			if(EmuSystem::gameIsRunning())
			{
				startGameFromMenu();
			}
		}
		else viewStack.inputEvent(e);
	}
}

static void onViewChange(void *, Gfx::GfxViewState *)
{
	logMsg("view change");
	GuiTable1D::setDefaultXIndent();
	View::defaultFace->applySettings(FontSettings(Gfx::yMMSizeToPixel(optionLargeFonts ? largeFontMM : fontMM)));
	//View::defaultFace->applySettings(FontSettings(33));
	popup.place();
	emuView.place();
	viewStack.place(Gfx::viewportRect());
	if(modalView)
		modalView->place(Gfx::viewportRect());
	logMsg("done view change");
}

static struct EmuNavViewTarget : public NavView::Target
{
	void onLeftNavBtn(const InputEvent &e)
	{
		viewStack.popAndShow();
	}

	void onRightNavBtn(const InputEvent &e)
	{
		if(EmuSystem::gameIsRunning())
		{
			startGameFromMenu();
		}
	}
} emuNavViewTarget;

ResourceImage *getArrowAsset()
{
	static ResourceImage *res = 0;
	if(!res)
	{
		res = ResourceImagePng::loadAsset("padButton.png");
		res->ref();
	}
	return res;
}

ResourceImage *getXAsset()
{
	static ResourceImage *res = 0;
	if(!res)
	{
		res = ResourceImagePng::loadAsset("xButton.png");
		res->ref();
	}
	return res;
}

static void mainInitCommon()
{
	Gfx::setDither(0);
	Gfx::setClear(1);

	#ifdef CONFIG_AUDIO_COREAUDIO_LARGE_BUFFER_FRAMES
	Audio::hintPcmFramesPerWrite(1600);
	#elif defined(CONFIG_AUDIO_COREAUDIO_MED_BUFFER_FRAMES)
	Audio::hintPcmFramesPerWrite(950);
	#endif

	initOptions();
	EmuSystem::initOptions();
	#ifdef CONFIG_BASE_ANDROID
		if(Base::runningDeviceType() == Base::DEV_TYPE_XPERIA_PLAY)
			EmuControls::profileManager(InputEvent::DEV_KEYBOARD).baseProfile = 1;
	#endif
	resetBaseKeyProfile();

	#ifdef CONFIG_BLUETOOTH
	assert(maxPlayers <= 5);
	Bluetooth::maxGamepadsPerType = maxPlayers;
	#endif

	ResourceFace *faceRes = ResourceFace::loadAsset("Vera.ttf");
	assert(faceRes);
	View::defaultFace = faceRes;

	loadConfigFile();
	EmuSystem::configAudioRate();
	Gfx::setValidOrientations(optionMenuOrientation, 1);
	EmuSystem::setupAutoSaveStateTime(optionAutoSaveState.val);

	gameView.init();
	disp.init();
	keyMapping.build(EmuControls::category);

	faceRes->applySettings(FontSettings(Gfx::yMMSizeToPixel(optionLargeFonts ? largeFontMM : fontMM)));
	popup.init();
	#ifndef CONFIG_BASE_PS3
	vController.init((int)optionTouchCtrlAlpha / 255.0, Gfx::xMMSize(int(optionTouchCtrlSize) / 100.));
	updateVControlImg();
	resolveOnScreenCollisions();
	setupVControllerPosition();
	#endif

	Base::appResumeEventHandler(onResumed);
	gfx_viewChangeHandler(onViewChange);
	gfx_drawHandler(onDraw);
	Base::appExitEventHandler(onExit);
	Base::focusChangeHandler(onFocusChange);
	Base::inputDevChangeHandler(onInputDevChange);

	if(optionDPI != 0U)
		Base::setDPI(optionDPI);

	viewNav.init(&emuNavViewTarget, View::defaultFace, View::needsBackControl ? getArrowAsset() : 0,
			!Config::envIsPS3 ? getArrowAsset() : 0, navViewGrad, sizeofArray(navViewGrad));
	viewNav.setRightBtnActive(0);
	viewStack.init();
	if(optionTitleBar)
		viewStack.setNavView(&viewNav);

	#ifdef CONFIG_BASE_IOS_SETUID
		CATS::whatYouSay();

		const char *emuPatch = "/Applications/EMUPatcher.app/EMUPatcher";
		if(Fs::fileExists(emuPatch))
		{
			Base::setUIDEffective();
			FsSys::remove(emuPatch);
			Base::setUIDReal();
		}
	#endif
}

#ifndef CONFIG_BASE_PS3
#include "TouchConfigView.hh"
static TouchConfigView tcMenu;
#endif

#include "CommonViewControl.hh"

#include "ButtonConfigView.hh"
