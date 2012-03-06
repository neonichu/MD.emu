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
#include <engine-globals.h>
#include <base/android/sdk.hh>
#include <logger/interface.h>

#include <base/interface.h>
#include <base/common/funcs.h>

// definitions from SDK 9 configuration.h
static const int ACONFIGURATION_KEYSHIDDEN_NO = 1, ACONFIGURATION_KEYSHIDDEN_YES = 2;

#include "common.hh"

namespace Input
{
#ifndef CONFIG_INPUT
static jboolean JNICALL touchEvent(JNIEnv *env, jobject thiz, jint action, jint x, jint y, jint pid) { return 0; }
static jboolean JNICALL trackballEvent(JNIEnv *env, jobject thiz, jint action, jfloat x, jfloat y) { return 0; }
static jboolean JNICALL keyEvent(JNIEnv *env, jobject thiz, jint key, jint down) { return 0; }
#else
jboolean JNICALL touchEvent(JNIEnv *env, jobject thiz, jint action, jint x, jint y, jint pid);
jboolean JNICALL trackballEvent(JNIEnv *env, jobject thiz, jint action, jfloat x, jfloat y);
jboolean JNICALL keyEvent(JNIEnv *env, jobject thiz, jint key, jint down);
#endif
}

namespace Base
{

// JNI methods/variables to access Java component
static jclass jMessageCls = 0, jGLViewCls = 0, jHandlerCls = 0;
static jobject jHandler = 0;
#ifdef CONFIG_GFX_SOFT_ORIENTATION
static JavaClassMethod jSetAutoOrientation;
#endif
static JavaInstMethod jPostDelayed, jRemoveCallbacks /*, jPost, jHasMessages*/;
static JavaClassMethod jSwapBuffers;
static jfieldID jAllowKeyRepeatsId, jHandleVolumeKeysId;
static jobject jTimerCallbackRunnable;

uint appState = APP_RUNNING; //TODO: should start as APP_PAUSED, but BaseActivitiy needs some changes

#ifdef CONFIG_BLUEZ

JavaClassMethod jObtain;
JavaInstMethod jSendToTarget;
static jfieldID msgHandlerId;
jobject msgHandler;

#endif

static JavaVM* jVM = 0;

/*void statusBarHidden(uint hidden)
{
	jclass cls = (*jEnv)->FindClass(jEnv, "com/imagine/BaseActivity");
	if(cls == NULL)
	{
		logMsg("can't find class");
	}
	jmethodID mid = (*jEnv)->GetStaticMethodID(jEnv, cls, "setStatusBar", "(B)V");
	if (mid == 0)
	{
		logMsg("can't find method");
		return;
	}
	(*jEnv)->CallStaticVoidMethod(jEnv, cls, mid, (char)hidden);
}*/

static TimerCallbackFunc timerCallbackFunc = 0;
static void *timerCallbackFuncCtx = 0;
void setTimerCallback(TimerCallbackFunc f, void *ctx, int ms)
{
	if(timerCallbackFunc)
	{
		logMsg("canceling callback");
		timerCallbackFunc = 0;
		jEnv->CallBooleanMethod(jHandler, jRemoveCallbacks.m, jTimerCallbackRunnable);
	}
	if(!f)
		return;
	logMsg("setting callback to run in %d ms", ms);
	timerCallbackFunc = f;
	timerCallbackFuncCtx = ctx;
	jEnv->CallBooleanMethod(jHandler, jPostDelayed.m, jTimerCallbackRunnable, (jlong)ms);
}

static jboolean JNICALL timerCallback(JNIEnv*  env, jobject thiz, jboolean isPaused)
{
	if(timerCallbackFunc)
	{
		if(isPaused)
			logMsg("running callback with app paused");
		else
			logMsg("running callback");
		timerCallbackFunc(timerCallbackFuncCtx);
		timerCallbackFunc = 0;
		return !isPaused && gfxUpdate;
	}
	return 0;
}

#ifdef CONFIG_BLUEZ

static JNIEnv* inputThreadJEnv = 0;

CallResult android_attachInputThreadToJVM()
{
	if(jVM->AttachCurrentThread(&inputThreadJEnv, 0) != 0)
	{
		logErr("error attaching jEnv to thread");
		return INVALID_PARAMETER;
	}
	return OK;
}

void android_detachInputThreadToJVM()
{
	jVM->DetachCurrentThread();
}

#endif

static void JNICALL envConfig(JNIEnv*  env, jobject thiz, jstring filesPath, jstring eStoragePath,
	jint xMM, jint yMM, jint orientation, jint refreshRate, jint apiLevel, jint hardKeyboardState,
	jint navigationState, jstring devName, jstring apkPath, jobject sysVibrator)
{
	assert(thiz);
	jBaseActivity = env->NewGlobalRef(thiz);
	filesDir = env->GetStringUTFChars(filesPath, 0);
	eStoreDir = env->GetStringUTFChars(eStoragePath, 0);
	appPath = env->GetStringUTFChars(apkPath, 0);
	logMsg("apk @ %s", appPath);

	osOrientation = orientation;
	refreshRate_ = refreshRate;
	logMsg("refresh rate: %d", refreshRate);

	//logMsg("set screen MM size %d,%d", xMM, yMM);
	// these are values are rotated according to OS orientation
	Gfx::viewMMWidth_ = androidViewXMM = xMM;
	Gfx::viewMMHeight_ = androidViewYMM = yMM;

	setSDK(apiLevel);

	const char *devNameStr = env->GetStringUTFChars(devName, 0);
	logMsg("device name: %s", devNameStr);
	setDeviceType(devNameStr);
	env->ReleaseStringUTFChars(devName, devNameStr);

	aHardKeyboardState = (devType == DEV_TYPE_XPERIA_PLAY) ? navigationState : hardKeyboardState;
	logMsg("keyboard/nav hidden: %s", hardKeyboardNavStateToStr(aHardKeyboardState));

	if(sysVibrator)
	{
		logMsg("Vibrator object present");
		vibrator = jEnv->NewGlobalRef(sysVibrator);
		setupVibration(jEnv);
	}
}

static void JNICALL nativeInit(JNIEnv*  env, jobject thiz, jint w, jint h)
{
	logMsg("native code init");
	assert(jVM);
	assert(jBaseActivity);
	#ifdef CONFIG_BLUEZ
	msgHandler = env->NewGlobalRef(env->GetStaticObjectField(jBaseActivityCls, msgHandlerId));
	//logMsg("msgHandler %p", msgHandler);
	#endif
	jfieldID timerCallbackRunnableId = env->GetStaticFieldID(jBaseActivityCls, "timerCallbackRunnable", "Ljava/lang/Runnable;");
	assert(timerCallbackRunnableId);
	jTimerCallbackRunnable = env->NewGlobalRef(env->GetStaticObjectField(jBaseActivityCls, timerCallbackRunnableId));
	//logMsg("jTimerCallbackRunnable %p", jTimerCallbackRunnable);
	jfieldID handlerID = env->GetStaticFieldID(jGLViewCls, "handler", "Landroid/os/Handler;");
	assert(handlerID);
	jHandler = env->NewGlobalRef(env->GetStaticObjectField(jGLViewCls, handlerID));
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
	//logMsg("doc files path: %s", filesDir);
	//logMsg("external storage path: %s", eStoreDir);
	return;
}

static void JNICALL nativeResize(JNIEnv*  env, jobject thiz, jint w, jint h)
{
	//logMsg("resize event");
	generic_resizeEvent(w, h);
	//logMsg("done resize");
}

static jboolean JNICALL nativeRender(JNIEnv*  env, jobject thiz)
{
	//logMsg("doing render");
	runEngine();
	//if(ret) logMsg("frame rendered");
	return gfxUpdate;
}

void openGLUpdateScreen()
{
	//if(!gfxUpdate) logMsg("sleeping after this frame");
	jEnv->CallStaticVoidMethod(jSwapBuffers.c, jSwapBuffers.m, gfxUpdate);
}

#ifdef CONFIG_BLUETOOTH
void sendMessageToMain(int type, int shortArg, int intArg, int intArg2)
{
	assert(inputThreadJEnv);
	jobject jMsg = inputThreadJEnv->CallStaticObjectMethod(jObtain.c, jObtain.m, msgHandler, shortArg + (type << 16), intArg, intArg2);
	inputThreadJEnv->CallVoidMethod(jMsg, jSendToTarget.m);
	inputThreadJEnv->DeleteLocalRef(jMsg);
}
#endif

static void JNICALL configChange(JNIEnv*  env, jobject thiz, jint hardKeyboardState, jint navState, jint orientation)
{
	logMsg("config change, keyboard: %s, navigation: %s", hardKeyboardNavStateToStr(hardKeyboardState), hardKeyboardNavStateToStr(navState));
	setHardKeyboardState((devType == DEV_TYPE_XPERIA_PLAY) ? navState : hardKeyboardState);
	setOrientationOS(orientation);
}

static void JNICALL appPaused(JNIEnv*  env, jobject thiz)
{
	logMsg("backgrounding");
	displayNeedsUpdate(); // display needs a redraw upon resume
	appState = APP_PAUSED;

	callSafe(onAppExitHandler, onAppExitHandlerCtx, 1);
}

static void JNICALL appResumed(JNIEnv*  env, jobject thiz, jboolean hasFocus)
{
	logMsg("resumed");
	appState = APP_RUNNING;
	callSafe(onAppResumeHandler, onAppResumeHandlerCtx, hasFocus);
}

static jboolean JNICALL appFocus(JNIEnv*  env, jobject thiz, jboolean hasFocus)
{
	logMsg("focus change: %d", (int)hasFocus);
	var_copy(prevGfxUpdateState, gfxUpdate);
	callSafe(onFocusChangeHandler, onFocusChangeHandlerCtx, hasFocus);
	return appState == APP_RUNNING && prevGfxUpdateState == 0 && gfxUpdate;
}

static jboolean JNICALL handleAndroidMsg(JNIEnv *env, jobject thiz, jint arg1, jint arg2, jint arg3)
{
	var_copy(prevGfxUpdateState, gfxUpdate);
	processAppMsg(arg1 >> 16, arg1 & 0xFFFF, arg2, arg3);
	return prevGfxUpdateState == 0 && gfxUpdate;
}

}

CLINK JNIEXPORT jint JNICALL LVISIBLE JNI_OnLoad(JavaVM *vm, void*)
{
	using namespace Base;
	logMsg("in JNI_OnLoad");
	jVM = vm;
	int getEnvRet = vm->GetEnv((void**) &jEnv, JNI_VERSION_1_6);
	assert(getEnvRet == JNI_OK);

	// BaseActivity members
	jBaseActivityCls = (jclass)jEnv->NewGlobalRef(jEnv->FindClass("com/imagine/BaseActivity"));
	jSetRequestedOrientation.setup(jEnv, jBaseActivityCls, "setRequestedOrientation", "(I)V");
	jAddNotification.setup(jEnv, jBaseActivityCls, "addNotification", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	jRemoveNotification.setup(jEnv, jBaseActivityCls, "removeNotification", "()V");
	jAllowKeyRepeatsId = jEnv->GetStaticFieldID(jBaseActivityCls, "allowKeyRepeats", "Z");
	jHandleVolumeKeysId = jEnv->GetStaticFieldID(jBaseActivityCls, "handleVolumeKeys", "Z");
	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	jSetAutoOrientation.setup(jEnv, jBaseActivityCls, "setAutoOrientation", "(ZI)V");
	#endif

	// Handler members
	jHandlerCls = (jclass)jEnv->NewGlobalRef(jEnv->FindClass("android/os/Handler"));
	jPostDelayed.setup(jEnv, jHandlerCls, "postDelayed", "(Ljava/lang/Runnable;J)Z");
	jRemoveCallbacks.setup(jEnv, jHandlerCls, "removeCallbacks", "(Ljava/lang/Runnable;)V");
	//jPost.setup(env, jHandlerCls, "post", "(Ljava/lang/Runnable;)Z");
	//jHasMessages.setup(env, jHandlerCls, "hasMessages", "(I)Z");

	// GLView members
	jGLViewCls = (jclass)jEnv->NewGlobalRef(jEnv->FindClass("com/imagine/GLView"));
	jSwapBuffers.setup(jEnv, jGLViewCls, "swapBuffers", "()V");
	//jFinish.setup(env, jBaseActivityCls, "finish", "()V");

	#ifdef CONFIG_BLUEZ
	jMessageCls = (jclass)jEnv->NewGlobalRef(jEnv->FindClass("android/os/Message"));
	jObtain.setup(jEnv, jMessageCls, "obtain", "(Landroid/os/Handler;III)Landroid/os/Message;");
	jSendToTarget.setup(jEnv, jMessageCls, "sendToTarget", "()V");
	msgHandlerId = jEnv->GetStaticFieldID(jBaseActivityCls, "msgHandler", "Landroid/os/Handler;");
	//logMsg("msgHandlerId %d", (int)msgHandlerId);
	#endif

	static JNINativeMethod activityMethods[] =
	{
	    {"timerCallback", "(Z)Z", (void *)&timerCallback},
	    {"appPaused", "()V", (void *)&appPaused},
	    {"appResumed", "(Z)V", (void *)&appResumed},
	    {"appFocus", "(Z)Z", (void *)&appFocus},
	    {"configChange", "(III)V", (void *)&configChange},
	    {"envConfig", "(Ljava/lang/String;Ljava/lang/String;IIIIIIILjava/lang/String;Ljava/lang/String;Landroid/os/Vibrator;)V", (void *)&envConfig},
	    {"handleAndroidMsg", "(III)Z", (void *)&handleAndroidMsg},
	    {"keyEvent", "(II)Z", (void *)&Input::keyEvent},
	};

	jEnv->RegisterNatives(jBaseActivityCls, activityMethods, sizeofArray(activityMethods));

	static JNINativeMethod glViewMethods[] =
	{
		{"nativeInit", "(II)V", (void *)&nativeInit},
		{"nativeResize", "(II)V", (void *)&nativeResize},
		{"nativeRender", "()Z", (void *)&nativeRender},
		{"touchEvent", "(IIII)Z", (void *)&Input::touchEvent},
		{"trackballEvent", "(IFF)Z", (void *)&Input::trackballEvent},
	};

	jEnv->RegisterNatives(jGLViewCls, glViewMethods, sizeofArray(glViewMethods));

	return JNI_VERSION_1_6;
}

namespace Input
{
using namespace Base;

void setKeyRepeat(bool on)
{
	logMsg("set key repeat %s", on ? "On" : "Off");
	jEnv->SetStaticBooleanField(jBaseActivityCls, jAllowKeyRepeatsId, on ? 1 : 0);
}

void setHandleVolumeKeys(bool on)
{
	logMsg("set volume key use %s", on ? "On" : "Off");
	jEnv->SetStaticBooleanField(jBaseActivityCls, jHandleVolumeKeysId, on ? 1 : 0);
}

}



#undef thisModuleName
