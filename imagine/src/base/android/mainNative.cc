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

#define thisModuleName "base:android"
#include <stdlib.h>
#include <errno.h>
#include <util/egl.hh>
#include <logger/interface.h>
#include <engine-globals.h>
#include <base/android/sdk.hh>
#include <base/interface.h>
#include <base/common/funcs.h>
#include <base/common/PollWaitTimer.hh>
#include <android/window.h>
#include "nativeGlue.hh"

namespace Base
{

static const EGLint attribs16BPP[] =
{
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	//EGL_DEPTH_SIZE, 24,
	EGL_NONE
};

static const EGLint attribs24BPP[] =
{
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_BLUE_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_RED_SIZE, 8,
	EGL_NONE
};

static const EGLint attribs32BPP[] =
{
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_BLUE_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_RED_SIZE, 8,
	EGL_ALPHA_SIZE, 8,
	EGL_NONE
};

struct EGLWindow
{
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	EGLConfig config;
	int winFormat;
	bool gotFormat;
	static constexpr int defaultWinFormat = WINDOW_FORMAT_RGB_565;

	constexpr EGLWindow(): display(EGL_NO_DISPLAY), surface(EGL_NO_SURFACE), context(EGL_NO_CONTEXT),
			config(0), winFormat(defaultWinFormat), gotFormat(0) { }

	void initEGL()
	{
		logMsg("doing EGL init");
		display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		assert(display != EGL_NO_DISPLAY);
		eglInitialize(display, 0, 0);

		//printEGLConfs(display);

		logMsg("%s (%s), extensions: %s", eglQueryString(display, EGL_VENDOR), eglQueryString(display, EGL_VERSION), eglQueryString(display, EGL_EXTENSIONS));
	}

	void init(ANativeWindow *win)
	{
		assert(display != EGL_NO_DISPLAY);
		int currFormat = ANativeWindow_getFormat(win);
		logMsg("got window format %d", currFormat);
		if(currFormat != winFormat)
		{
			logMsg("setting to %d", winFormat);
			ANativeWindow_setBuffersGeometry(win, 0, 0, winFormat);
		}

		if(!gotFormat)
		{
			const EGLint *attribs = attribs16BPP;

			switch(winFormat)
			{
				bcase WINDOW_FORMAT_RGBX_8888: attribs = attribs24BPP;
				bcase WINDOW_FORMAT_RGBA_8888: attribs = attribs32BPP;
			}

			//EGLConfig config;
			EGLint configs = 0;
			eglChooseConfig(display, attribs, &config, 1, &configs);
			#ifndef NDEBUG
			printEGLConf(display, config);
			#endif
			//EGLint format;
			//eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
			gotFormat = 1;
		}

		if(context == EGL_NO_CONTEXT)
		{
			logMsg("creating GL context");
			context = eglCreateContext(display, config, 0, 0);
		}

		assert(surface == EGL_NO_SURFACE);
		logMsg("creating window surface");
		surface = eglCreateWindowSurface(display, config, win, 0);

		if(eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
		{
			logErr("error in eglMakeCurrent");
			abort();
		}
		logMsg("window size: %d,%d", ANativeWindow_getWidth(win), ANativeWindow_getHeight(win));

		/*if(eglSwapInterval(display, 3) != EGL_TRUE)
		{
			logErr("error in eglSwapInterval");
		}*/
	}

	void destroySurface()
	{
		if(isDrawable())
		{
			logMsg("destroying window surface");
			eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			eglDestroySurface(display, surface);
			surface = EGL_NO_SURFACE;
		}
	}

	bool verifyContext()
	{
		return eglGetCurrentContext() != EGL_NO_CONTEXT;
	}

	bool isDrawable()
	{
		return surface != EGL_NO_SURFACE;
	}

	void swap()
	{
		eglSwapBuffers(display, surface);
	}
} eglWin;

}

DLList<PollWaitTimer*>::Node PollWaitTimer::timerListNode[4];
DLList<PollWaitTimer*> PollWaitTimer::timerList(timerListNode);

#include "common.hh"

namespace Base
{

static JavaInstMethod jGetRotation, jSetupEnv;
static jclass jDisplayCls;
static jobject jDpy;

uint appState = APP_PAUSED;

#ifdef CONFIG_BLUETOOTH
void sendMessageToMain(int type, int shortArg, int intArg, int intArg2)
{
	assert(appInstance()->msgwrite);
	assert(type != MSG_INPUT); // currently all code should use main event loop for input events
	uint msg[3] = { (shortArg << 16) | type, intArg, intArg2 };
	logMsg("sending msg type %d with args %d %d %d", msg[0] & 0xFFFF, msg[0] >> 16, msg[1], msg[2]);
	if(::write(appInstance()->msgwrite, &msg, sizeof(msg)) != sizeof(msg))
	{
		logErr("unable to write message to pipe: %s", strerror(errno));
	}
}
#endif

static PollWaitTimer timerCallback;
void setTimerCallback(TimerCallbackFunc f, void *ctx, int ms)
{
	timerCallback.setCallback(f, ctx, ms);
}

void addPollEvent2(int fd, PollHandler &handler)
{
	logMsg("adding fd %d to looper", fd);
	assert(appInstance()->looper);
	int ret = ALooper_addFd(appInstance()->looper, fd, LOOPER_ID_USER, ALOOPER_EVENT_INPUT, 0, &handler);
	assert(ret == 1);
}

void removePollEvent(int fd)
{
	logMsg("removing fd %d from looper", fd);
	int ret = ALooper_removeFd(appInstance()->looper, fd);
	assert(ret != -1);
}

void destroyPollEvent(int fd)
{
	logMsg("removing fd %d from looper", fd);
	ALooper_removeFd(appInstance()->looper, fd);
}

EGLDisplay getAndroidEGLDisplay()
{
	assert(eglWin.display != EGL_NO_DISPLAY);
	return eglWin.display;
}

bool windowIsDrawable()
{
	return eglWin.isDrawable();
}

// runs from activity thread
static void JNICALL jEnvConfig(JNIEnv* env, jobject thiz, jint xMM, jint yMM, jint refreshRate, jobject dpy,
		jstring devName, jstring filesPath, jstring eStoragePath, jstring apkPath, jobject sysVibrator)
{
	logMsg("doing java env config");
	//logMsg("set screen MM size %d,%d", xMM, yMM);
	// these are values are rotated according to OS orientation
	Gfx::viewMMWidth_ = androidViewXMM = xMM;
	Gfx::viewMMHeight_ = androidViewYMM = yMM;

	jDpy = env->NewGlobalRef(dpy);
	refreshRate_ = refreshRate;
	logMsg("refresh rate: %d", refreshRate);

	const char *devNameStr = env->GetStringUTFChars(devName, 0);
	logMsg("device name: %s", devNameStr);
	setDeviceType(devNameStr);
	env->ReleaseStringUTFChars(devName, devNameStr);

	filesDir = env->GetStringUTFChars(filesPath, 0);
	eStoreDir = env->GetStringUTFChars(eStoragePath, 0);
	appPath = env->GetStringUTFChars(apkPath, 0);
	logMsg("apk @ %s", appPath);

	if(sysVibrator)
	{
		logMsg("Vibrator object present");
		vibrator = env->NewGlobalRef(sysVibrator);
		setupVibration(env);
	}
}

static void envConfig(int orientation, int hardKeyboardState, int navigationState)
{
	logMsg("doing native env config");
	osOrientation = orientation;

	aHardKeyboardState = (devType == DEV_TYPE_XPERIA_PLAY) ? navigationState : hardKeyboardState;
	logMsg("keyboard/nav hidden: %s", hardKeyboardNavStateToStr(aHardKeyboardState));
}

static bool engineIsInit = 0;

static void nativeInit(jint w, jint h)
{
	doOrExit(logger_init());
	if(unlikely(Gfx::viewMMWidth_ > 9000)) // hack for Archos Tablets
	{
		logMsg("screen size over 9000! setting to something sane");
		Gfx::viewMMWidth_ = androidViewXMM = 50;
		Gfx::viewMMHeight_ = androidViewYMM = Gfx::viewMMWidth_/(w/float(h));
	}
	newXSize = w;
	newYSize = h;

	engineInit();
	logMsg("done init");
	engineIsInit = 1;
}

void openGLUpdateScreen()
{
	eglWin.swap();
}

static bool aHasFocus = 1;

static bool appFocus(bool hasFocus)
{
	aHasFocus = hasFocus;
	logMsg("focus change: %d", (int)hasFocus);
	var_copy(prevGfxUpdateState, gfxUpdate);
	callSafe(onFocusChangeHandler, onFocusChangeHandlerCtx, hasFocus);
	return appState == APP_RUNNING && prevGfxUpdateState == 0 && gfxUpdate;
}

static bool updateViewportAfterRotation = 0;

static void configChange(struct android_app* app, jint hardKeyboardState, jint navState, jint orientation)
{
	logMsg("config change, keyboard: %s, navigation: %s", hardKeyboardNavStateToStr(hardKeyboardState), hardKeyboardNavStateToStr(navState));
	setHardKeyboardState((devType == DEV_TYPE_XPERIA_PLAY) ? navState : hardKeyboardState);

	if(setOrientationOS(orientation))
	{
		updateViewportAfterRotation = 1;
		// surface resized event isn't sent (bug in Android?),
		// but window size appaears correct after next eglSwapBuffers()
		// so update viewport then
	}
}

static void appPaused()
{
	logMsg("backgrounding");
	appState = APP_PAUSED;
	callSafe(onAppExitHandler, onAppExitHandlerCtx, 1);
}

static void appResumed()
{
	logMsg("resumed");
	appState = APP_RUNNING;
	if(engineIsInit)
	{
		callSafe(onAppResumeHandler, onAppResumeHandlerCtx, aHasFocus);
		displayNeedsUpdate();
	}
}

void onAppCmd(struct android_app* app, uint32 cmd)
{
	switch (cmd & 0xFFFF)
	{
		bcase APP_CMD_START:
		logMsg("got APP_CMD_START");
		bcase APP_CMD_SAVE_STATE:
		logMsg("got APP_CMD_SAVE_STATE");
		/*app->savedState = malloc(sizeof(struct saved_state));
		*((struct saved_state*)app->savedState) = engine->state;
		app->savedStateSize = sizeof(struct saved_state);*/
		bcase APP_CMD_INIT_WINDOW:
		if(app->window)
		{
			eglWin.init(app->window);
			int w = ANativeWindow_getWidth(app->window);
			int h = ANativeWindow_getHeight(app->window);
			if(!engineIsInit)
				nativeInit(w, h);
			else
				generic_resizeEvent(w, h);
		}
		bcase APP_CMD_TERM_WINDOW:
		//logMsg("got APP_CMD_TERM_WINDOW");
		pthread_mutex_lock(&app->mutex);
		eglWin.destroySurface();
		pthread_cond_signal(&app->cond);
		pthread_mutex_unlock(&app->mutex);
		bcase APP_CMD_GAINED_FOCUS:
		if(app->window)
		{
			int w = ANativeWindow_getWidth(app->window);
			int h = ANativeWindow_getHeight(app->window);
			generic_resizeEvent(w, h);
		}
		appFocus(1);
		bcase APP_CMD_LOST_FOCUS:
		appFocus(0);
		bcase APP_CMD_WINDOW_REDRAW_NEEDED:
		{
			gfxUpdate = 1;
			int w = ANativeWindow_getWidth(app->window);
			int h = ANativeWindow_getHeight(app->window);
			logMsg("updating viewport from redraw with size %d,%d", w, h);
			generic_resizeEvent(w, h);
		}
		bcase APP_CMD_WINDOW_RESIZED:
		logMsg("got APP_CMD_WINDOW_RESIZED");
		{
			gfxUpdate = 1;
			int w = ANativeWindow_getWidth(app->window);
			int h = ANativeWindow_getHeight(app->window);
			generic_resizeEvent(w, h);
		}
		bcase APP_CMD_CONTENT_RECT_CHANGED:
		{
			var_ref(rect, app->contentRect);
			logMsg("got APP_CMD_CONTENT_RECT_CHANGED, %d,%d %d,%d", rect.left, rect.top, rect.right, rect.bottom);
			gfxUpdate = 1;
			int w = ANativeWindow_getWidth(app->window);
			int h = ANativeWindow_getHeight(app->window);
			generic_resizeEvent(w, h);
		}
		bcase APP_CMD_CONFIG_CHANGED:
		configChange(app, AConfiguration_getKeysHidden(app->config),
				AConfiguration_getNavHidden(app->config),
				jEnv->CallIntMethod(jDpy, jGetRotation.m));
		bcase APP_CMD_PAUSE:
		appPaused();
		bcase APP_CMD_RESUME:
		appResumed();
		bcase APP_CMD_STOP:
		logMsg("got APP_CMD_STOP");
		bcase APP_CMD_DESTROY:
		logMsg("got APP_CMD_DESTROY");
		pthread_exit(0);
		bcase MSG_BT:
		case MSG_INPUTDEV_CHANGE:
		{
			uint32 arg[2];
			read(app->msgread, arg, sizeof(arg));
			logMsg("got msg type %d with args %d %d %d", cmd & 0xFFFF, cmd >> 16, arg[0], arg[1]);
			Base::processAppMsg(cmd & 0xFFFF, cmd >> 16, arg[0], arg[1]);
		}
		bdefault:
		logWarn("got unknown cmd %d", cmd);
		break;
	}
}

static int getPollTimeout()
{
	// When waiting for events:
	// If rendering, don't block. If a timer is active, block for its time remaining. Else block until next event
	int pollTimeout = gfxUpdate ? 0 :
		PollWaitTimer::hasCallbacks() ? PollWaitTimer::getNextCallback()->calcPollWaitForFunc() :
		-1;
	if(pollTimeout >= 2000)
		logMsg("will poll for at most %d ms", pollTimeout);
	return pollTimeout;
}

}

void android_main(struct android_app* state)
{
	using namespace Base;
	logMsg("started native thread");
	//state->onAppCmd = Base::onAppCmd;
	//state->onInputEvent = Input::onInputEvent;
	{
		var_copy(act, state->activity);
		var_copy(config, state->config);
		jBaseActivity = act->clazz;
		if(act->vm->AttachCurrentThread(&jEnv, 0) != 0)
		{
			bug_exit("error attaching jEnv to thread");
		}
		envConfig(jEnv->CallIntMethod(jDpy, jGetRotation.m),
			AConfiguration_getKeysHidden(config), AConfiguration_getNavHidden(config));
	}
	eglWin.initEGL();

	for(;;)
	{
		int ident;
		int events;
		int fd;
		struct PollHandler* source;
		//logMsg("entering looper");
		while((ident=ALooper_pollAll(getPollTimeout(), &fd, &events, (void**)&source)) >= 0)
		{
			//logMsg("out of looper with event id %d", ident);
			switch(ident)
			{
				bcase LOOPER_ID_MAIN: process_cmd(state);
				bcase LOOPER_ID_INPUT: process_input(state);
				bdefault: // LOOPER_ID_USER
					assert(source);
					source->func(source->data, events);
				break;
			}
		}
		//logMsg("out of looper with return %d", ident);
		assert(engineIsInit);

		PollWaitTimer::processCallbacks();

		if(!gfxUpdate)
			logMsg("out of event loop without gfxUpdate");

		if(unlikely(appState != APP_RUNNING || !eglWin.isDrawable()))
			gfxUpdate = 0;
		runEngine();
		if(unlikely(updateViewportAfterRotation && state->window))
		{
			int w = ANativeWindow_getWidth(state->window);
			int h = ANativeWindow_getHeight(state->window);
			logMsg("updating viewport after rotation with size %d,%d", w, h);
			generic_resizeEvent(w, h);
			gfxUpdate = 1;
			runEngine();
			updateViewportAfterRotation = 0;
		}
	}
}

void javaInit(JNIEnv *jEnv, jobject inst) // use JNIEnv from Activity thread
{
	using namespace Base;
	logMsg("doing pre-app JNI setup");

	// get class loader instance from Activity
	jclass jNativeActivityCls = jEnv->FindClass("android/app/NativeActivity");
	assert(jNativeActivityCls);
	jmethodID jGetClassLoaderID = jEnv->GetMethodID(jNativeActivityCls, "getClassLoader", "()Ljava/lang/ClassLoader;");
	assert(jGetClassLoaderID);
	jobject jClsLoader = jEnv->CallObjectMethod(inst, jGetClassLoaderID);
	assert(jClsLoader);

	jclass jClsLoaderCls = jEnv->FindClass("java/lang/ClassLoader");
	assert(jClsLoaderCls);
	jmethodID jLoadClassID = jEnv->GetMethodID(jClsLoaderCls, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
	assert(jLoadClassID);

	// BaseActivity members
	jstring baseActivityStr = jEnv->NewStringUTF("com/imagine/BaseActivity");
	jBaseActivityCls = (jclass)jEnv->NewGlobalRef(jEnv->CallObjectMethod(jClsLoader, jLoadClassID, baseActivityStr));
	jSetRequestedOrientation.setup(jEnv, jBaseActivityCls, "setRequestedOrientation", "(I)V");
	jSetupEnv.setup(jEnv, jBaseActivityCls, "setupEnv", "()V");
	jAddNotification.setup(jEnv, jBaseActivityCls, "addNotification", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	jRemoveNotification.setup(jEnv, jBaseActivityCls, "removeNotification", "()V");

	// Display members
	jclass jDisplayCls = (jclass)jEnv->NewGlobalRef(jEnv->FindClass("android/view/Display"));
	jGetRotation.setup(jEnv, jDisplayCls, "getRotation", "()I");

	static JNINativeMethod activityMethods[] =
	{
	    {"jEnvConfig", "(IIILandroid/view/Display;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Landroid/os/Vibrator;)V", (void *)&Base::jEnvConfig},
	};

	jEnv->RegisterNatives(jBaseActivityCls, activityMethods, sizeofArray(activityMethods));
	jEnv->CallVoidMethod(inst, jSetupEnv.m);
}

#undef thisModuleName
