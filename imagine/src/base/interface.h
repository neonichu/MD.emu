#pragma once

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

#include <engine-globals.h>

#ifdef CONFIG_BASE_IOS
	#include <base/iphone/iOS.hh>
#elif defined(CONFIG_BASE_ANDROID)
	#include <base/android/sdk.hh>
#endif

// app startup
CallResult main_init() ATTRS(cold);

namespace Base
{

// app exit
void exitVal(int returnVal) ATTRS(noreturn);;
static void exit() { exitVal(0); }
void abort() ATTRS(noreturn);;

typedef void(*AppExitFunc)(void*, bool backgrounded);
void appExitEventHandler(AppExitFunc handler, void *userPtr = 0);

typedef void(*AppResumeFunc)(void*, bool hasFocus);
void appResumeEventHandler(AppResumeFunc handler, void *userPtr = 0);

// console args
#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_WIN32) \
	|| defined (CONFIG_BASE_SDL) || defined(CONFIG_BASE_GENERIC)
	uint numArgs();
	char * getArg(uint arg);
#else
	static uint numArgs() { return 0; }
	static char * getArg(uint arg) { return 0; }
#endif

// drag & drop
typedef void(*DragDropFunc)(void*, void*);
#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_WIN32)
	uint getDragDropNumFiles(void *ddHnd);
	void getDragDropFilename(void *ddHnd, uint i, char* filename, uint bufferSize);
	void dragDropEventHandler(DragDropFunc handler, void *userPtr);
#else
	static uint getDragDropNumFiles(void *ddHnd) { return 0; };
	static void getDragDropFilename(void *ddHnd, uint i, char* filename, uint bufferSize) { }
	static void dragDropEventHandler(DragDropFunc handler, void *userPtr) { }
#endif

// app instance management
typedef void(*AltInstanceFunc)(void*, void*);
#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_WIN32)
	void altInstanceEventHandler(AltInstanceFunc handler, void *userPtr);
#else
	static void altInstanceEventHandler(AltInstanceFunc handler, void *userPtr) { }
#endif

// app events
typedef void(*FocusChangeFunc)(void*, uint);
#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_PS3) || defined (CONFIG_BASE_SDL) || defined (CONFIG_BASE_ANDROID)
	void focusChangeHandler(FocusChangeFunc handler, void *userPtr = 0);
#else
	static void focusChangeHandler(FocusChangeFunc handler, void *userPtr = 0) { }
#endif

struct InputDevChange
{
	uint devId, devType;
	uint state;
	enum { ADDED, REMOVED, SHOWN, HIDDEN };

	bool added() const { return state == ADDED; }
	bool removed() const { return state == REMOVED; }
	bool shown() const { return state == SHOWN; }
	bool hidden() const { return state == HIDDEN; }
};
typedef void(*InputDevChangeFunc)(void*, const InputDevChange &change);
#if defined(CONFIG_ENV_WEBOS)
	static void inputDevChangeHandler(InputDevChangeFunc handler, void *userPtr = 0) { }
	static bool isInputDevPresent(uint type) { return 0; }
#else
	void inputDevChangeHandler(InputDevChangeFunc handler, void *userPtr = 0);
	bool isInputDevPresent(uint type);
#endif

static const ushort MSG_INPUT = 0, MSG_INPUTDEV_CHANGE = 1, MSG_BT = 2;
#if defined(CONFIG_BASE_X11) || defined(CONFIG_BASE_ANDROID)
void sendMessageToMain(int type, int shortArg, int intArg, int intArg2);
#else
static void sendMessageToMain(int type, int shortArg, int intArg, int intArg2) { }
#endif

static void sendInputMessageToMain(uint dev, uint devType, uint btn, uint action)
{
	sendMessageToMain(MSG_INPUT, dev, devType + (action << 16), btn);
}

static void sendInputDevChangeMessageToMain(uint id, uint type, uint state)
{
	sendMessageToMain(MSG_INPUTDEV_CHANGE, state, type, id);
}

static void sendBTMessageToMain(uint event)
{
	sendMessageToMain(MSG_BT, 0, event, 0);
}

void displayNeedsUpdate();

static const uint APP_RUNNING = 0, APP_PAUSED = 1, APP_EXITING = 2;
extern uint appState;
static bool appIsRunning() { return appState == APP_RUNNING; }

// sleeping
void sleepMs(int ms); //sleep for ms milliseconds
void sleepUs(int us);

// window management
#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_WIN32)
	void setWindowTitle(char *name);
#else
	static void setWindowTitle(char *name) { }
#endif

// DPI override
#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_ANDROID)
	#define CONFIG_SUPPORTS_DPI_OVERRIDE
	void setDPI(float dpi);
#else
	static void setDPI(float dpi) { }
#endif

// display refresh rate
uint refreshRate();

// external services
#if defined (CONFIG_BASE_IOS)
	void openURL(const char *url);
#else
	static void openURL(const char *url) { }
#endif

// OpenGL windowing system support
#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_WIN32) || defined (CONFIG_BASE_SDL)
	CallResult openGLInit();
	CallResult openGLSetOutputVideoMode(uint x, uint y);
	CallResult openGLSetMultisampleVideoMode(uint x, uint y);
#else
	static CallResult openGLInit() { return OK; }
	static CallResult openGLSetOutputVideoMode(uint x, uint y) { return OK; }
	static CallResult openGLSetMultisampleVideoMode(uint x, uint y) { return OK; }
#endif

// poll()-like event system support (WIP API)

#define base_pollHandlerFuncProto(name) int name(void* data, int events)
struct PollHandler
{
	constexpr PollHandler(base_pollHandlerFuncProto((*func)), void *data) : func(func), data(data) { }
	constexpr PollHandler(base_pollHandlerFuncProto((*func))) : func(func), data(0) { }
	constexpr PollHandler() : func(0), data(0) { }
	base_pollHandlerFuncProto((*func));
	void *data;
};

#if defined (CONFIG_BASE_X11) || (defined(CONFIG_BASE_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9)
	#define CONFIG_BASE_HAS_FD_EVENTS
	static const bool hasFDEvents = 1;
	void addPollEvent2(int fd, PollHandler &handler); // caller is in charge of handler's memory
	void removePollEvent(int fd); // unregister the fd (must still be open)
#else
	static const bool hasFDEvents = 0;
	static void addPollEvent2(int fd, void *func, void* data) { }
	static void removePollEvent(int fd) { }
#endif

// timer event support
typedef void(*TimerCallbackFunc)(void*);
void setTimerCallback(TimerCallbackFunc f, void *ctx, int ms);

static void setTimerCallbackSec(TimerCallbackFunc f, void *ctx, int s)
{
	setTimerCallback(f, ctx, s * 1000);
}

// app file system paths
extern const char *appPath;
#if defined (CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS)
	const char *documentsPath();
#else
	static const char *documentsPath() { return appPath; }
#endif

#if defined (CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS) || defined(CONFIG_ENV_WEBOS)
	const char *storagePath();
#else
	static const char *storagePath() { return appPath; }
#endif

#if defined(CONFIG_BASE_IOS) && defined(CONFIG_BASE_IOS_JB)
	#define CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
#endif

// status bar management (phone-like devices)
#if defined(CONFIG_BASE_IOS) || defined(CONFIG_ENV_WEBOS)// || defined(CONFIG_BASE_ANDROID)
	void statusBarHidden(uint hidden);
	void statusBarOrientation(uint o);
#else
	static void statusBarHidden(uint hidden) { }
	static void statusBarOrientation(uint o) { }
#endif

// orientation sensor support
#if defined(CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS)
	void setAutoOrientation(bool on);
#else
	static void setAutoOrientation(bool on) { }
#endif

// vibration support
#if defined(CONFIG_BASE_ANDROID)
	void vibrate(uint ms);
#else
	static void vibrate(uint ms) { }
#endif

// UID functions
#ifdef CONFIG_BASE_IOS_SETUID
	extern uid_t realUID, effectiveUID;
	void setUIDReal();
	bool setUIDEffective();
#else
	static int realUID = 0, effectiveUID = 0;
	static void setUIDReal() { };
	static bool setUIDEffective() { return 0; };
#endif

// Device Identification
enum { DEV_TYPE_GENERIC,
#if defined (CONFIG_BASE_ANDROID)
	DEV_TYPE_XPERIA_PLAY, DEV_TYPE_XOOM,
#elif defined(CONFIG_BASE_IOS)
	DEV_TYPE_IPAD,
#endif
};

#if defined (CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS)
	int runningDeviceType();
#else
	static int runningDeviceType() { return DEV_TYPE_GENERIC; }
#endif

// Notification icon
#if defined (CONFIG_BASE_ANDROID)
	void addNotification(const char *onShow, const char *title, const char *message);
#else
	static void addNotification(const char *onShow, const char *title, const char *message) { }
#endif
}
