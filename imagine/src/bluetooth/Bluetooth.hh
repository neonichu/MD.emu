#pragma once

#include <input/interface.h>

typedef void(*BTConnectionEventFunc)(int msg);

#define CONFIG_BLUETOOTH_ICP

namespace Bluetooth
{

static const uint MSG_NO_DEVS = 2,
	MSG_ERR_CHANNEL = 3, MSG_ERR_NAME = 4, MSG_UNKNOWN_DEV = 5, MSG_NO_PERMISSION = 6;

bool startBT(BTConnectionEventFunc btConnectFunc);
CallResult initBT();
void closeBT();
bool devsAreConnected();

extern BTConnectionEventFunc connectFunc;
static const uint maxGamepadsPerTypeStorage = 5;
extern uint maxGamepadsPerType;
uint wiimotes();
uint iCPs();
uint zeemotes();
extern uint scanSecs;

}
