#pragma once

#include <util/jni.hh>
#include <util/Motion.hh>

extern TimedMotion<GC> projAngleM;
extern bool glSyncHackBlacklisted, glSyncHackEnabled;
extern bool glPointerStateHack;

#ifdef CONFIG_BLUETOOTH
	#include <bluetooth/Bluetooth.hh>
	#include "bluez.hh"
#endif

namespace Base
{

JNIEnv* jEnv = 0;
jclass jBaseActivityCls = 0;
jobject jBaseActivity = 0;
static JavaInstMethod jSetRequestedOrientation;
static JavaInstMethod jAddNotification, jRemoveNotification;

const char *appPath = 0;
static uint aSDK = aMinSDK;

void setSDK(uint sdk)
{
	aSDK = sdk;
	logMsg("SDK API Level: %d", aSDK);
}

uint androidSDK()
{
	return IG::max(aMinSDK, aSDK);
}

DefCallbackWithCtx(FocusChangeFunc, focusChangeHandler, onFocusChangeHandler);

void exitVal(int returnVal)
{
	logMsg("called exit");
	appState = APP_EXITING;
	callSafe(onAppExitHandler, onAppExitHandlerCtx, 0);
	jEnv->CallVoidMethod(jBaseActivity, jRemoveNotification.m);
	logMsg("exiting");
	::exit(returnVal);
}
void abort() { ::abort(); }

void displayNeedsUpdate() { generic_displayNeedsUpdate(); }

//void openURL(const char *url) { };

static const char *filesDir = 0, *eStoreDir = 0;

const char *documentsPath() { return filesDir; }
const char *storagePath() { return eStoreDir; }

static uint androidViewXMM = 0, androidViewYMM = 0;

void setDPI(float dpi)
{
	if(dpi == 0)
	{
		assert(androidViewXMM);
		Gfx::viewMMWidth_ = androidViewXMM;
		Gfx::viewMMHeight_ = androidViewYMM;
	}
	else
	{
		Gfx::viewMMWidth_ = ((float)newXSize / dpi) * 25.4;
		Gfx::viewMMHeight_ = ((float)newYSize / dpi) * 25.4;
	}
	Gfx::setupScreenSize();
}

int devType = DEV_TYPE_GENERIC;

static void setDeviceType(const char *dev)
{
	#ifdef __ARM_ARCH_7A__
	//if(strstr(board, "sholes"))
	if(androidSDK() > 8 && strstr(dev, "R800"))
	{
		logMsg("running on Xperia Play");
		devType = DEV_TYPE_XPERIA_PLAY;
	}
	/*else if(testSDKGreater(10) && strstr(dev, "wingray"))
	{
		logMsg("running on Xoom");
		devType = DEV_TYPE_XOOM;
	}*/
	else if(androidSDK() < 11 && (strstr(dev, "shooter") || string_equal(dev, "inc")))
	{
		// Evo 3D/Shooter, & HTC Droid Incredible hack
		logMsg("device needs glFinish() hack");
		glSyncHackBlacklisted = 1;
	}
	else if(/*testSDKLess(11) &&*/ string_equal(dev, "GT-B5510"))
	{
		logMsg("device needs gl*Pointer() hack");
		glPointerStateHack = 1;
	}
	#endif
}

int runningDeviceType()
{
	return devType;
}

//static const int HARDKEYBOARDHIDDEN_NO = 1, HARDKEYBOARDHIDDEN_YES = 2;
// NAVHIDDEN_* mirrors KEYSHIDDEN_*

static const char *hardKeyboardNavStateToStr(int state)
{
	switch(state)
	{
		case ACONFIGURATION_KEYSHIDDEN_NO: return "no";
		case ACONFIGURATION_KEYSHIDDEN_YES: return "yes";
		default: return "undefined";
	}
}

static int aHardKeyboardState = 0;

static bool hardKeyboardIsPresent()
{
	return aHardKeyboardState == ACONFIGURATION_KEYSHIDDEN_NO;
}

static void setHardKeyboardState(int hardKeyboardState)
{
	if(aHardKeyboardState != hardKeyboardState)
	{
		aHardKeyboardState = hardKeyboardState;
		logMsg("hard keyboard hidden: %s", hardKeyboardNavStateToStr(aHardKeyboardState));
		const InputDevChange change = { 0, InputEvent::DEV_KEYBOARD, hardKeyboardIsPresent() ? InputDevChange::SHOWN : InputDevChange::HIDDEN };
		callSafe(onInputDevChangeHandler, onInputDevChangeHandlerCtx, change);
	}
}

bool isInputDevPresent(uint type)
{
	switch(type)
	{
		case InputEvent::DEV_KEYBOARD:
			if(hardKeyboardIsPresent()) logMsg("hard keyboard present");
			return hardKeyboardIsPresent();
		#ifdef CONFIG_BLUETOOTH
		case InputEvent::DEV_WIIMOTE: return Bluetooth::wiimotes();
		case InputEvent::DEV_ICONTROLPAD: return Bluetooth::iCPs();
		#endif
	}
	return 0;
}

// Vibration
static jobject vibrator = 0;
static jclass vibratorCls = 0;
static JavaInstMethod jVibrate;

static void setupVibration(JNIEnv* jEnv)
{
	if(!vibratorCls)
	{
		//logMsg("setting up Vibrator class");
		vibratorCls = (jclass)jEnv->NewGlobalRef(jEnv->FindClass("android/os/Vibrator"));
		jVibrate.setup(jEnv, vibratorCls, "vibrate", "(J)V");
	}
}

void vibrate(uint ms)
{
	assert(vibratorCls);
	if(vibrator)
	{
		//logDMsg("vibrating for %u ms", ms);
		jEnv->CallVoidMethod(vibrator, jVibrate.m, (jlong)ms);
	}
}

void addNotification(const char *onShow, const char *title, const char *message)
{
	logMsg("adding notificaion icon");
	jEnv->CallVoidMethod(jBaseActivity, jAddNotification.m, jEnv->NewStringUTF(onShow), jEnv->NewStringUTF(title), jEnv->NewStringUTF(message));
}

static const int Surface_ROTATION_0 = 0, Surface_ROTATION_90 = 1, Surface_ROTATION_180 = 2, Surface_ROTATION_270 = 3;
static const GC orientationDiffTable[4][4] =
{
		{ 0, -90, 180, 90 },
		{ 90, 0, -90, 180 },
		{ 180, 90, 0, -90 },
		{ -90, 180, 90, 0 },
};
static int osOrientation;

#ifdef CONFIG_GFX_SOFT_ORIENTATION
/*CLINK JNIEXPORT void JNICALL LVISIBLE Java_com_imagine_BaseActivity_setOrientation(JNIEnv* env, jobject thiz, jint o)
{
	logMsg("orientation sensor change");
	preferedOrientation = o;
	if(gfx_setOrientation(o))
		postRenderUpdate();
}*/

static bool autoOrientationState = 0; // Defaults to off in Java code

void setAutoOrientation(bool on)
{
	if(autoOrientationState == on)
	{
		//logMsg("auto orientation already in state: %d", (int)on);
		return;
	}
	autoOrientationState = on;
	if(!on)
		preferedOrientation = rotateView;
	logMsg("setting auto-orientation: %s", on ? "on" : "off");
	jEnv->CallStaticVoidMethod(jSetAutoOrientation.c, jSetAutoOrientation.m, (jbyte)on, (jint)rotateView);
}

#else

void setAutoOrientation(bool on)
{
	logMsg("called Android setAutoOrientation, no-op");
}

static bool isSidewaysOrientation(int o)
{
	return o == Surface_ROTATION_90 || o == Surface_ROTATION_270;
}

static bool isStraightOrientation(int o)
{
	return o == Surface_ROTATION_0 || o == Surface_ROTATION_180;
}

static bool orientationsAre90DegreesAway(int o1, int o2)
{
	switch(o1)
	{
		case Surface_ROTATION_0:
		case Surface_ROTATION_180:
			return isSidewaysOrientation(o2);
		case Surface_ROTATION_90:
		case Surface_ROTATION_270:
			return isStraightOrientation(o2);
	}
	assert(0);
	return 0;
}

static bool setOrientationOS(int o)
{
	logMsg("OS orientation change");
	if(osOrientation != o)
	{
		/*if((isSidewaysOrientation(osOrientation) && !isSidewaysOrientation(o)) ||
			(!isSidewaysOrientation(osOrientation) && isSidewaysOrientation(o)))*/
		if(orientationsAre90DegreesAway(o, osOrientation))
		{
			logMsg("rotating screen MM");
			// TODO: DPI should be re-calculated upon new surface creation
			IG::swap(Gfx::viewMMWidth_, Gfx::viewMMHeight_);
			IG::swap(androidViewXMM, androidViewYMM);
			Gfx::setupScreenSize();
		}

		if(androidSDK() < 11)
		{
			GC rotAngle = orientationDiffTable[osOrientation][o];
			logMsg("animating from %d degrees", (int)rotAngle);
			projAngleM.initLinear(IG::toRadians(rotAngle), 0, 10);
		}
		//logMsg("new value %d", o);
		osOrientation = o;
		return 1;
	}
	return 0;

}

#endif

}

#ifndef CONFIG_GFX_SOFT_ORIENTATION
namespace Gfx
{

uint setOrientation(uint o)
{
	using namespace Base;
	logMsg("requested orientation change to %s", orientationName(o));
	if(o == VIEW_ROTATE_AUTO)
	{
		// set auto
		#if !defined(__i386__) // TODO: segfault on Google TV, cause unknown
		jEnv->CallVoidMethod(jBaseActivity, jSetRequestedOrientation.m, -1); // SCREEN_ORIENTATION_UNSPECIFIED
		#endif
	}
	else
	{
		int toSet = -1;
		switch(o)
		{
			case VIEW_ROTATE_0: toSet = 1; break; // SCREEN_ORIENTATION_PORTRAIT
			case VIEW_ROTATE_90: toSet = 0; break; // SCREEN_ORIENTATION_LANDSCAPE
			case VIEW_ROTATE_270: toSet = androidSDK() > 8 ? 8 : 0; break; // SCREEN_ORIENTATION_REVERSE_LANDSCAPE
			case VIEW_ROTATE_90 | VIEW_ROTATE_270: toSet = androidSDK() > 8 ? 6 : 1; break; // SCREEN_ORIENTATION_SENSOR_LANDSCAPE
			default: logWarn("unhandled manual orientation specified");
		}
		//if(toSet >= 0)
			jEnv->CallVoidMethod(jBaseActivity, jSetRequestedOrientation.m, toSet);
	}
	return 1;
}

uint setValidOrientations(uint oMask, bool manageAutoOrientation)
{
	return setOrientation(oMask);
}

}
#endif
