#pragma once

#include <mem/interface.h>

#ifdef CONFIG_FS
	#include <fs/Fs.h>
#endif

#ifdef CONFIG_INPUT
	#include <input/interface.h>
	namespace Input
	{
		extern InputEventFunc onInputEventHandler;
		extern void *onInputEventHandlerCtx;
	}
#endif

#ifdef CONFIG_GFX
	#include <gfx/interface.h>
#endif

#ifdef CONFIG_AUDIO
	#include <audio/Audio.hh>
#endif

#ifdef CONFIG_BLUEZ
#include <bluetooth/Bluetooth.hh>
#endif

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

namespace Base
{

#if defined(CONFIG_BASE_PS3)
	uint refreshRate() { return 60; } // hard-code for now
#else
	static uint refreshRate_ = 0;
	uint refreshRate() { return refreshRate_; }
#endif

static int triggerGfxResize = 0;
static uint newXSize = 0, newYSize = 0;

DefCallbackWithCtxStatic(DragDropFunc, generic_dragDropEventHandler, onDragDropHandler);
DefCallbackWithCtxStatic(AltInstanceFunc, generic_altInstanceEventHandler, onAltInstanceHandler);
DefCallbackWithCtx(AppExitFunc, appExitEventHandler, onAppExitHandler);
DefCallbackWithCtx(AppExitFunc, appResumeEventHandler, onAppResumeHandler);
#if !defined(CONFIG_ENV_WEBOS)
DefCallbackWithCtx(InputDevChangeFunc, inputDevChangeHandler, onInputDevChangeHandler);
#endif

int gfxUpdate = 0;
static void generic_displayNeedsUpdate()
{
	//logMsg("posting display update");
	gfxUpdate = 1;
}

#ifdef CONFIG_GFX
static int generic_resizeEvent(uint x, uint y, bool force = 0)
{
	newXSize = x; newYSize = y;
	// do gfx_resizeDisplay only if our window-size changed
	if(force || (newXSize != Gfx::viewPixelWidth()) || (newYSize != Gfx::viewPixelHeight()))
	{
		logMsg("resizing display area to %dx%d", x, y);
		triggerGfxResize = 1;
		gfxUpdate = 1;
		return 1;
	}
	return 0;
}
#endif

const char copyright[] = "Imagine is Copyright 2010, 2011 Robert Broglia";

static void engineInit() ATTRS(cold);
static void engineInit()
{
	logDMsg("%s", copyright);
	logDMsg("compiled on %s %s", __DATE__, __TIME__);
	mem_init();
	
	#ifdef CONFIG_GFX
		doOrExit(Gfx::init());
		doOrExit(Gfx::setOutputVideoMode(newXSize, newYSize));
	#endif

	#ifdef CONFIG_INPUT
		doOrExit(Input::init());
	#endif	
		
	#ifdef CONFIG_AUDIO
		doOrExit(Audio::init());
	#endif

	doOrExit(main_init());
}

static uint runEngine()
{
	#ifdef CONFIG_GFX
	if(unlikely(triggerGfxResize))
	{
		Gfx::resizeDisplay(newXSize, newYSize);
		triggerGfxResize = 0;
	}
	#endif

	int frameRendered = 0;
	#ifdef CONFIG_GFX
		if(likely(gfxUpdate))
		{
			gfxUpdate = 0;
			Gfx::renderFrame();
			frameRendered = 1;
		}
		else
		{
			//logDMsg("gfx not updating, just using video sync");
			//gfx_waitVideoSync();
		}
	#endif

	return frameRendered;
}

#ifndef CONFIG_SUPCXX
	// needed by GCC when not compiling with libstdc++/libsupc++
	CLINK EVISIBLE void __cxa_pure_virtual() { bug_exit("called pure virtual"); }
#endif

#if defined(__unix__) || defined(__APPLE__)
void sleepUs(int us)
{
	usleep(us);
}

void sleepMs(int ms)
{
	sleepUs(ms*1000);
}
#endif

static void processAppMsg(int type, int shortArg, int intArg, int intArg2)
{
	switch(type)
	{
		#ifdef CONFIG_BLUEZ
		case MSG_INPUT:
		{
			callSafe(Input::onInputEventHandler, Input::onInputEventHandlerCtx, InputEvent(shortArg, intArg & 0xFFFF, intArg2, intArg >> 16));
		}
		break;
		case MSG_INPUTDEV_CHANGE:
		{
			logMsg("got input dev change message");
			callSafe(onInputDevChangeHandler, onInputDevChangeHandlerCtx, (InputDevChange){ (uint)intArg2, (uint)intArg, (uint)shortArg });
		}
		break;
		case MSG_BT:
		{
			logMsg("got bluetooth connect message");
			Bluetooth::connectFunc(intArg);
		}
		break;
		#endif
	}
}

}

#ifndef CONFIG_SUPCXX
	EVISIBLE void* operator new (size_t size) { return mem_alloc(size); }
	EVISIBLE void* operator new[] (size_t size)	{ return mem_alloc(size); }
	EVISIBLE void operator delete (void *o) { mem_free(o); }
	EVISIBLE void operator delete[] (void *o) { mem_free(o); }
#endif

#ifdef CONFIG_BASE_PS3
	void* operator new (size_t size) throw () { return mem_alloc(size); }
	void* operator new[] (size_t size)	throw () { return mem_alloc(size); }
	void operator delete (void *o) { mem_free(o); }
	void operator delete[] (void *o) { mem_free(o); }
#endif

void *operator new (size_t size, void *o)
#ifdef CONFIG_BASE_PS3
throw ()
#endif
{
	//logMsg("called placement new, %d bytes @ %p", (int)size, o);
	return o;
}
