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
#include <util/cLang.h>
#include <util/memory.h>
#ifdef CONFIG_BLUETOOTH
	#include <bluetooth/Bluetooth.hh>
#endif
#include "KeyCategory.hh"
#include <TurboInput.hh>

static const int guiKeyIdxLoadGame = 0;
static const int guiKeyIdxMenu = 1;
static const int guiKeyIdxSaveState = 2;
static const int guiKeyIdxLoadState = 3;
static const int guiKeyIdxFastForward = 4;
static const int guiKeyIdxGameScreenshot = 5;
static const int guiKeyIdxExit = 6;

#include <inGameActionKeys.hh>
#include <main/EmuControls.hh>

namespace EmuControls
{

static KeyProfileManager keyProfileManager[supportedInputDevs] =
{
	#ifdef INPUT_SUPPORTS_KEYBOARD
	KeyProfileManager(kb),
	#endif
	#ifdef CONFIG_BASE_PS3
	KeyProfileManager(ps3Pad),
	#endif
	#ifdef CONFIG_BLUETOOTH
	KeyProfileManager(wii),
	KeyProfileManager(iCP),
	KeyProfileManager(zeemote),
	#endif
	#ifdef CONFIG_BASE_IOS_ICADE
	KeyProfileManager(iCade),
	#endif
};

static KeyProfileManager &profileManager(uint devType)
{
	return keyProfileManager[inputDevTypeSlot(devType)];
}

}

template <uint KEYS>
void KeyConfig<KEYS>::loadProfile(uint devType, int idx)
{
	EmuControls::profileManager(devType).loadProfile(key(devType), totalKeys, idx);
}

template <uint KEYS>
void KeyConfig<KEYS>::loadBaseProfile(uint devType)
{
	EmuControls::profileManager(devType).loadBaseProfile(key(devType), totalKeys);
}

KeyConfig<EmuControls::systemTotalKeys> keyConfig;

template <size_t S>
void KeyMapping::build(KeyCategory (&category)[S])
{
	using namespace EmuControls;
	logMsg("rebuilding input maps");
	#ifdef INPUT_SUPPORTS_KEYBOARD
	mem_zero(keyActions);
	#endif
	#ifdef CONFIG_BASE_PS3
	mem_zero(ps3Actions);
	#endif
	#ifdef CONFIG_BLUETOOTH
	mem_zero(wiimoteActions);
	mem_zero(icpActions);
	mem_zero(zeemoteActions);
	#endif
	#ifdef CONFIG_BASE_IOS_ICADE
	mem_zero(iCadeActions);
	#endif

	{
		forEachDInArray(supportedInputDev, dev)
		{
			logMsg("mapping actions to %s", InputEvent::devTypeName(dev));
			Action (*actionMap)[maxKeyActions];
			mapFromDevType(dev, actionMap);
			uint *keyArr = keyConfig.key(dev);

			iterateTimes(keyConfig.totalKeys, k)
			{
				uint key = keyArr[k];
				//logMsg("key %u %s", key, Input::buttonName(dev, key));
				assert(key < InputEvent::devTypeNumKeys(dev));
				Action *slot = mem_findFirstZeroValue(actionMap[key]);
				if(slot)
					*slot = k+1; // add 1 to avoid 0 value (considered unmapped)
			}
		}
	}
}

KeyMapping keyMapping;

void buildKeyMapping()
{
	keyMapping.build(EmuControls::category);
}

static void resetBaseKeyProfile(uint devType = InputEvent::DEV_NULL)
{
	using namespace EmuControls;

	logMsg("setting base keys (%d total)", keyConfig.totalKeys);
	forEachDInArray(supportedInputDev, dev)
	{
		// devType == InputEvent::DEV_NULL sets all device types
		if(devType != InputEvent::DEV_NULL && devType != dev)
			continue;
		logMsg("setting base keys for %s, profile %d, %p", InputEvent::devTypeName(dev),
				EmuControls::profileManager(/**cat,*/ dev).baseProfile, keyConfig.key(/**cat,*/ dev));
		keyConfig.loadBaseProfile(/**cat,*/ dev);

		/*iterateTimes(cat->keys, k)
		{
			uint key = keyConfig.key(cat_i, dev)[k];
			logMsg("key %u %s", key, Input::buttonName(dev, key));
		}*/
		/*iterateTimes(category[0].keys, k)
		{
			uint key = keyConfig.key(0, InputEvent::DEV_WIIMOTE)[k];
			logMsg("current key %u %s", key, Input::buttonName(InputEvent::DEV_WIIMOTE, key));
		}*/
	}

	/*iterateTimes(keyConfig.totalKeys, k)
	{
		uint dev = InputEvent::DEV_KEYBOARD;
		uint key = keyConfig.key(dev)[k];
		logMsg("key %u %s", key, Input::buttonName(dev, key));
	}*/

}

#ifdef INPUT_SUPPORTS_POINTER
static uint pointerInputPlayer = 0;
#endif
#ifdef INPUT_SUPPORTS_KEYBOARD
static uint keyboardInputPlayer = 0;
#endif
#ifdef CONFIG_BASE_PS3
static uint gamepadInputPlayer[5] = { 0, 1, 2, 3, 4 };
#endif
#ifdef CONFIG_BLUETOOTH
static uint wiimoteInputPlayer[5] = { 0, 1, 2, 3, 4 };
static uint iControlPadInputPlayer[5] = { 0, 1, 2, 3, 4 };
static uint zeemoteInputPlayer[5] = { 0, 1, 2, 3, 4 };
#endif
#ifdef CONFIG_BASE_IOS_ICADE
static uint iCadeInputPlayer = 0;
#endif

static int relPtrX = 0, relPtrY = 0; // for Android trackball
static void processRelPtr(const InputEvent &e)
{
	using namespace IG;
	if(relPtrX != 0 && signOf(relPtrX) != signOf(e.x))
	{
		//logMsg("reversed trackball X direction");
		relPtrX = e.x;
	}
	else
		relPtrX += e.x;

	if(relPtrY != 0 && signOf(relPtrY) != signOf(e.y))
	{
		//logMsg("reversed trackball Y direction");
		relPtrY = e.y;
	}
	else
		relPtrY += e.y;

	//logMsg("trackball event %d,%d, rel ptr %d,%d", e.x, e.y, relPtrX, relPtrY);
}

static uint mapInputToPlayer(const InputEvent &e)
{
	switch(e.devType)
	{
	#ifdef CONFIG_BLUETOOTH
		case InputEvent::DEV_WIIMOTE:
			assert(e.devId < Bluetooth::wiimotes());
			return wiimoteInputPlayer[e.devId];
		case InputEvent::DEV_ICONTROLPAD:
			assert(e.devId < Bluetooth::iCPs());
			return iControlPadInputPlayer[e.devId];
		case InputEvent::DEV_ZEEMOTE:
			assert(e.devId < Bluetooth::zeemotes());
			return zeemoteInputPlayer[e.devId];
	#endif
	#ifdef CONFIG_BASE_IOS_ICADE
		case InputEvent::DEV_ICADE:
			return iCadeInputPlayer;
	#endif
	#ifdef CONFIG_BASE_PS3
		case InputEvent::DEV_PS3PAD:
			return (e.devId < maxPlayers) ? gamepadInputPlayer[e.devId] : 0;
	#endif
		default: // all other keys from devices map to keyboard
		#ifdef INPUT_SUPPORTS_KEYBOARD
			return keyboardInputPlayer;
		#endif
			bug_branch("%d", e.devType); return 0;
	}
}

static TurboInput turboActions;

static void commonUpdateInput()
{
	using namespace IG;
	static const uint turboFrames = 4;
	static uint turboClock = 0;

	forEachInArray(turboActions.activeAction, e)
	{
		if(e->action)
		{
			if(turboClock == 0)
			{
				//logMsg("turbo push for player %d, action %d", e->player, e->action);
				EmuSystem::handleInputAction(e->player, INPUT_PUSHED, e->action);
			}
			else if(turboClock == turboFrames/2)
			{
				//logMsg("turbo release for player %d, action %d", e->player, e->action);
				EmuSystem::handleInputAction(e->player, INPUT_RELEASED, e->action);
			}
		}
	}
	turboClock++;
	if(turboClock == turboFrames) turboClock = 0;

	relPtrX = clipToZeroSigned(relPtrX, (int)optionRelPointerDecel * -signOf(relPtrX));
	relPtrY = clipToZeroSigned(relPtrY, (int)optionRelPointerDecel * -signOf(relPtrY));
}
