#pragma once

#include <EGL/egl.h>
#include <logger/interface.h>
#include <util/cLang.h>

static void printEGLConf(EGLDisplay display, EGLConfig config)
{
	EGLint buffSize, redSize, greenSize, blueSize, alphaSize, cav, depthSize, nID, nRend, sType,
		minSwap, maxSwap;
	eglGetConfigAttrib(display, config, EGL_BUFFER_SIZE, &buffSize);
	eglGetConfigAttrib(display, config, EGL_RED_SIZE, &redSize);
	eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &greenSize);
	eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blueSize);
	eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE, &alphaSize);
	eglGetConfigAttrib(display, config, EGL_CONFIG_CAVEAT, &cav);
	eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depthSize);
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &nID);
	eglGetConfigAttrib(display, config, EGL_NATIVE_RENDERABLE, &nRend);
	eglGetConfigAttrib(display, config, EGL_SURFACE_TYPE, &sType);
	eglGetConfigAttrib(display, config, EGL_MIN_SWAP_INTERVAL, &minSwap);
	eglGetConfigAttrib(display, config, EGL_MAX_SWAP_INTERVAL, &maxSwap);
	logMsg("config %d %d:%d:%d:%d cav:0x%X d:%d nid:%d nrend:%d stype:0x%X swap:%d-%d",
			buffSize, redSize, greenSize, blueSize, alphaSize, cav, depthSize, nID, nRend, sType, minSwap, maxSwap);
}

static void printEGLConfs(EGLDisplay display)
{
	EGLConfig conf[96];
	EGLint num = 0;
	eglGetConfigs(display, conf, sizeofArray(conf), &num);
	logMsg("got %d configs", num);
	iterateTimes(num, i)
	{
		printEGLConf(display, conf[i]);
	}
}
