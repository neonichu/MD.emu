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

#include <input/interface.h>
#include <util/bits.h>

namespace Input
{
	extern InputEventFunc onInputEventHandler;
	extern void *onInputEventHandlerCtx;
}

static const PackedInputAccess wiimoteDataAccess[] =
{
	{ 0, BIT(0), Input::Wiimote::DOWN }, // map to sideways control
	{ 0, BIT(1), Input::Wiimote::UP },
	{ 0, BIT(2), Input::Wiimote::RIGHT },
	{ 0, BIT(3), Input::Wiimote::LEFT },
	{ 0, BIT(4), Input::Wiimote::PLUS },

	{ 1, BIT(0), Input::Wiimote::_2 },
	{ 1, BIT(1), Input::Wiimote::_1 },
	{ 1, BIT(2), Input::Wiimote::B },
	{ 1, BIT(3), Input::Wiimote::A },
	{ 1, BIT(4), Input::Wiimote::MINUS },
	{ 1, BIT(7), Input::Wiimote::HOME },
};

static const PackedInputAccess wiimoteCCDataAccess[] =
{
	{ 4, BIT(7), Input::Wiimote::RIGHT },
	{ 4, BIT(6), Input::Wiimote::DOWN },
	{ 4, BIT(5), Input::Wiimote::L },
	{ 4, BIT(4), Input::Wiimote::MINUS },
	{ 4, BIT(3), Input::Wiimote::HOME },
	{ 4, BIT(2), Input::Wiimote::PLUS },
	{ 4, BIT(1), Input::Wiimote::R },

	{ 5, BIT(7), Input::Wiimote::ZL },
	{ 5, BIT(6), Input::Wiimote::B },
	{ 5, BIT(5), Input::Wiimote::Y },
	{ 5, BIT(4), Input::Wiimote::A },
	{ 5, BIT(3), Input::Wiimote::X },
	{ 5, BIT(2), Input::Wiimote::ZR },
	{ 5, BIT(1), Input::Wiimote::LEFT },
	{ 5, BIT(0), Input::Wiimote::UP },
};

namespace WiimoteDefs
{
	static const char btNamePrefix[] = "Nintendo RVL-CNT-01";
	static const uchar btClass[] = { 0x04, 0x25, 0x00 };
	static const uchar btClassRemotePlus[] = { 0x08, 0x05, 0x00 };
}

class Wiimote
{
public:
	Wiimote()
	{
		mem_zero(prevBtnData);
		mem_zero(stickBtn);
	}

	static uchar playerLeds(int player)
	{
		switch(player)
		{
			default:
			case 0: return BIT(4);
			case 1: return BIT(5);
			case 2: return BIT(6);
			case 3: return BIT(7);
			case 4: return BIT(4) | BIT(7);
		}
	}

	bool stickBtn[8];
	static void decodeCCSticks(const uchar *ccSticks, int &lX, int &lY, int &rX, int &rY)
	{
		lX = ccSticks[0] & 0x3F;
		lY = ccSticks[1] & 0x3F;
		rX = (ccSticks[0] & 0xC0) >> 3 | (ccSticks[1] & 0xC0) >> 5 | (ccSticks[2] & 0x80) >> 7;
		rY = ccSticks[2] & 0x1F;
	}

	void processStickDataForButtonEmulation(int player, const uchar *data, bool onThread = 0)
	{
		int pos[4];
		decodeCCSticks(data, pos[0], pos[1], pos[2], pos[3]);
		//logMsg("CC sticks left %dx%d right %dx%d", pos[0], pos[1], pos[2], pos[3]);
		forEachInArray(stickBtn, e)
		{
			bool newState;
			switch(e_i)
			{
				case 0: newState = pos[0] < 31-8; break;
				case 1: newState = pos[0] > 31+8; break;
				case 2: newState = pos[1] < 31-8; break;
				case 3: newState = pos[1] > 31+8; break;

				case 4: newState = pos[2] < 16-4; break;
				case 5: newState = pos[2] > 16+4; break;
				case 6: newState = pos[3] < 16-4; break;
				case 7: newState = pos[3] > 16+4; break;
				default: bug_branch("%d", (int)e_i); break;
			}
			if(*e != newState)
			{
				static const uint btnEvent[8] =
				{
					Input::Wiimote::CC_LSTICK_LEFT, Input::Wiimote::CC_LSTICK_RIGHT, Input::Wiimote::CC_LSTICK_DOWN, Input::Wiimote::CC_LSTICK_UP,
					Input::Wiimote::CC_RSTICK_LEFT, Input::Wiimote::CC_RSTICK_RIGHT, Input::Wiimote::CC_RSTICK_DOWN, Input::Wiimote::CC_RSTICK_UP
				};
				//logMsg("%s %s @ wiimote %d", Input::buttonName(InputEvent::DEV_WIIMOTE, btnEvent[e_i]), newState ? "pushed" : "released", player);
				if(onThread)
					Base::sendInputMessageToMain(player, InputEvent::DEV_WIIMOTE, btnEvent[e_i], newState ? INPUT_PUSHED : INPUT_RELEASED);
				else
					callSafe(Input::onInputEventHandler, Input::onInputEventHandlerCtx, InputEvent(player, InputEvent::DEV_WIIMOTE, btnEvent[e_i], newState ? INPUT_PUSHED : INPUT_RELEASED));
			}
			*e = newState;
		}
	}

	uchar prevBtnData[2];
	void processCoreButtons(const uchar *packet, uint player, bool onThread = 0)
	{
		const uchar *btnData = &packet[2];
		forEachInArray(wiimoteDataAccess, e)
		{
			int newState = e->updateState(prevBtnData, btnData);
			if(newState != -1)
			{
				//logMsg("%s %s @ wiimote %d", Input::buttonName(InputEvent::DEV_WIIMOTE, e->keyEvent), newState ? "pushed" : "released", player);
				if(onThread)
					Base::sendInputMessageToMain(player, InputEvent::DEV_WIIMOTE, e->keyEvent, newState ? INPUT_PUSHED : INPUT_RELEASED);
				else
					callSafe(Input::onInputEventHandler, Input::onInputEventHandlerCtx, InputEvent(player, InputEvent::DEV_WIIMOTE, e->keyEvent, newState ? INPUT_PUSHED : INPUT_RELEASED));
			}
		}
		memcpy(prevBtnData, btnData, sizeof(prevBtnData));
	}

	uchar prevCCData[6];
	void processClassicButtons(const uchar *packet, uint player, bool onThread = 0)
	{
		const uchar *ccData = &packet[4];
		processStickDataForButtonEmulation(player, ccData, onThread);
		forEachInArray(wiimoteCCDataAccess, e)
		{
			int newState = e->updateState(prevCCData, ccData);
			if(newState != -1)
			{
				// note: buttons are 0 when pushed
				//logMsg("%s %s @ wiimote cc", e->name, !newState ? "pushed" : "released");
				if(onThread)
					Base::sendInputMessageToMain(player, InputEvent::DEV_WIIMOTE, e->keyEvent, !newState ? INPUT_PUSHED : INPUT_RELEASED);
				else
					callSafe(Input::onInputEventHandler, Input::onInputEventHandlerCtx, InputEvent(player, InputEvent::DEV_WIIMOTE, e->keyEvent, !newState ? INPUT_PUSHED : INPUT_RELEASED));
			}
		}
		memcpy(prevCCData, ccData, sizeof(prevCCData));
	}
};

static const PackedInputAccess iCPDataAccess[] =
{
	{ 0, BIT(2), Input::iControlPad::LEFT },
	{ 0, BIT(1), Input::iControlPad::RIGHT },
	{ 0, BIT(3), Input::iControlPad::DOWN },
	{ 0, BIT(0), Input::iControlPad::UP },
	{ 0, BIT(4), Input::iControlPad::L },

	{ 1, BIT(3), Input::iControlPad::A },
	{ 1, BIT(4), Input::iControlPad::X },
	{ 1, BIT(5), Input::iControlPad::B },
	{ 1, BIT(6), Input::iControlPad::R },
	{ 1, BIT(0), Input::iControlPad::SELECT },
	{ 1, BIT(2), Input::iControlPad::Y },
	{ 1, BIT(1), Input::iControlPad::START },
};

namespace IControlPadDefs
{
	static const char btNamePrefix[] = "iControlPad-";
	static const uchar btClass[] = { 0x00, 0x1F, 0x00 }; // no device class

	static const uchar CMD_SPP_GP_REPORTS = 0xAD;
	static const uchar turnOnReports[2] = { CMD_SPP_GP_REPORTS, 1 };

	static const uchar CMD_SET_LED = 0xFF;
	static const uchar turnOnLED[2] = { CMD_SET_LED, 1 };

	static const uchar CMD_FORCE_LED_CTRL = 0x6D;
	static const uchar turnOnLEDControl[2] = { CMD_FORCE_LED_CTRL, 1 };

	static const uchar CMD_SET_LED_MODE = 0xE4;
	static const uchar LED_PULSE_DOUBLE = 0;
	static const uchar setLEDPulseDouble[2] = { CMD_SET_LED_MODE, LED_PULSE_DOUBLE };
	static const uchar LED_PULSE_INVERSE = 2;
	static const uchar setLEDPulseInverse[2] = { CMD_SET_LED_MODE, LED_PULSE_INVERSE };
	/*const uchar LED_NO_PULSE = 3;
	const uchar setLEDNoPulse[2] = { CMD_SET_LED_MODE, LED_NO_PULSE };*/
	static const uchar LED_PULSE_DQUICK = 5;
	static const uchar setLEDPulseDQuick[2] = { CMD_SET_LED_MODE, LED_PULSE_DQUICK };


	static const uchar CMD_POWER_OFF = 0x94;
	static const uchar PWR_OFF_CHK_BYTE1 = 0x27;
	static const uchar PWR_OFF_CHK_BYTE2 = 0x6A;
	static const uchar PWR_OFF_CHK_BYTE3 = 0xFE;
	static const uchar shutdown[] = { CMD_POWER_OFF, PWR_OFF_CHK_BYTE1, PWR_OFF_CHK_BYTE2, PWR_OFF_CHK_BYTE3 };

	static const uchar RESP_OKAY = 0x80;
};

class IControlPad
{
public:
	IControlPad()
	{
		mem_zero(prevBtnData);
		mem_zero(nubBtn);
	}

	uchar prevBtnData[2];
	void processBtnReport(const uchar *btnData, uint player, bool onThread = 0)
	{
		forEachInArray(iCPDataAccess, e)
		{
			bool oldState = prevBtnData[e->byteOffset] & e->mask,
				newState = btnData[e->byteOffset] & e->mask;
			if(oldState != newState)
			{
				//logMsg("%s %s @ iCP", e->name, newState ? "pushed" : "released");
				if(onThread)
					Base::sendInputMessageToMain(player, InputEvent::DEV_ICONTROLPAD, e->keyEvent, newState ? INPUT_PUSHED : INPUT_RELEASED);
				else
					callSafe(Input::onInputEventHandler, Input::onInputEventHandlerCtx, InputEvent(player, InputEvent::DEV_ICONTROLPAD, e->keyEvent, newState ? INPUT_PUSHED : INPUT_RELEASED));
			}
		}
		memcpy(prevBtnData, btnData, sizeof(prevBtnData));
	}

	bool nubBtn[8];
	static const int nubDeadzone = 64;
	void processNubDataForButtonEmulation(const schar *nubData, uint player, bool onThread = 0)
	{
		//logMsg("iCP nubs %d %d %d %d", (int)nubData[0], (int)nubData[1], (int)nubData[2], (int)nubData[3]);
		forEachInArray(nubBtn, e)
		{
			bool newState;
			if(e_i % 2)
			{
				newState = nubData[e_i/2] > nubDeadzone;
			}
			else
			{
				newState = nubData[e_i/2] < -nubDeadzone;
			}
			if(*e != newState)
			{
				static const uint nubBtnEvent[8] =
				{
					Input::iControlPad::LNUB_LEFT, Input::iControlPad::LNUB_RIGHT, Input::iControlPad::LNUB_UP, Input::iControlPad::LNUB_DOWN,
					Input::iControlPad::RNUB_LEFT, Input::iControlPad::RNUB_RIGHT, Input::iControlPad::RNUB_UP, Input::iControlPad::RNUB_DOWN
				};
				if(onThread)
					Base::sendInputMessageToMain(player, InputEvent::DEV_ICONTROLPAD, nubBtnEvent[e_i], newState ? INPUT_PUSHED : INPUT_RELEASED);
				else
					callSafe(Input::onInputEventHandler, Input::onInputEventHandlerCtx, InputEvent(player, InputEvent::DEV_ICONTROLPAD, nubBtnEvent[e_i], newState ? INPUT_PUSHED : INPUT_RELEASED));
			}
			*e = newState;
		}
	}
};

namespace ZeemoteDefs
{
	static const char btName[] = "Zeemote JS1";
	static const uchar btClass[] = { 0x84, 0x05, 0x00 };
}

class Zeemote
{
public:
	Zeemote()
	{
		mem_zero(prevBtnPush);
		mem_zero(stickBtn);
	}

	static const uint RID_VERSION = 0x03, RID_BTN_METADATA = 0x04, RID_CONFIG_DATA = 0x05,
		RID_BTN_REPORT = 0x07, RID_8BA_2A_JS_REPORT = 0x08, RID_BATTERY_REPORT = 0x11;

	static const uint minPacketSize = 3;

	static const char *reportIDToStr(uint id)
	{
		switch(id)
		{
			case RID_VERSION: return "Version Report";
			case RID_BTN_METADATA: return "Button Metadata";
			case RID_CONFIG_DATA: return "Configuration Data";
			case RID_BTN_REPORT: return "Button Report";
			case RID_8BA_2A_JS_REPORT: return "8-bit Analog 2-Axis Joystick Report";
			case RID_BATTERY_REPORT: return "Battery Report";
		}
		return "Unknown";
	}

	bool prevBtnPush[4];
	void processBtnReport(const uchar *btnData, uint player, bool onThread = 0)
	{
		uchar btnPush[4] = { 0 };
		iterateTimes(4, i)
		{
			if(btnData[i] >= 4)
				break;
			btnPush[btnData[i]] = 1;
		}
		iterateTimes(4, i)
		{
			if(prevBtnPush[i] != btnPush[i])
			{
				bool newState = btnPush[i];
				uint code = i + 1;
				//logMsg("%s %s @ Zeemote", e->name, newState ? "pushed" : "released");
				if(onThread)
					Base::sendInputMessageToMain(player, InputEvent::DEV_ZEEMOTE, code, newState ? INPUT_PUSHED : INPUT_RELEASED);
				else
					callSafe(Input::onInputEventHandler, Input::onInputEventHandlerCtx, InputEvent(player, InputEvent::DEV_ZEEMOTE, code, newState ? INPUT_PUSHED : INPUT_RELEASED));
			}
		}
		memcpy(prevBtnPush, btnPush, sizeof(prevBtnPush));
	}

	bool stickBtn[4];
	void processStickDataForButtonEmulation(const schar *pos, int player, bool onThread = 0)
	{
		//logMsg("CC sticks left %dx%d right %dx%d", pos[0], pos[1], pos[2], pos[3]);
		forEachInArray(stickBtn, e)
		{
			bool newState;
			switch(e_i)
			{
				case 0: newState = pos[0] < -63; break;
				case 1: newState = pos[0] > 63; break;
				case 2: newState = pos[1] > 63; break;
				case 3: newState = pos[1] < -63; break;
				default: bug_branch("%d", (int)e_i); break;
			}
			if(*e != newState)
			{
				static const uint btnEvent[] =
				{
					Input::Zeemote::LEFT, Input::Zeemote::RIGHT, Input::Zeemote::DOWN, Input::Zeemote::UP,
				};
				if(onThread)
					Base::sendInputMessageToMain(player, InputEvent::DEV_ZEEMOTE, btnEvent[e_i], newState ? INPUT_PUSHED : INPUT_RELEASED);
				else
					callSafe(Input::onInputEventHandler, Input::onInputEventHandlerCtx, InputEvent(player, InputEvent::DEV_ZEEMOTE, btnEvent[e_i], newState ? INPUT_PUSHED : INPUT_RELEASED));
			}
			*e = newState;
		}
	}
};

static bool testSupportedBTDevClasses(uchar *devClass)
{
	return mem_equal(devClass, WiimoteDefs::btClass, 3)
		|| mem_equal(devClass, WiimoteDefs::btClassRemotePlus, 3)
		|| mem_equal(devClass, IControlPadDefs::btClass, 3)
		|| mem_equal(devClass, ZeemoteDefs::btClass, 3);
}
