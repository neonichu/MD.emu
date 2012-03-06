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

package com.imagine;

import android.app.*;
import android.content.*;
import android.os.*;
import android.view.*;
import android.graphics.*;
import android.util.*;
import android.hardware.*;
import android.media.*;
import android.content.res.Configuration;

// This class is also named BaseActivity to prevent shortcuts from breaking with previous SDK < 9 APKs

public final class BaseActivity extends NativeActivity
{
	private static String logTag = "BaseActivity";
	private native void jEnvConfig(int xMM, int yMM, int refreshRate, Display dpy, String devName, String filesPath, String eStoragePath, String apkPath, Vibrator sysVibrator);
	
	public void setupEnv()
	{
		Log.i(logTag, "got focus view " + getWindow().getDecorView());
		getWindow().getDecorView().setKeepScreenOn(true);
		Display dpy = getWindowManager().getDefaultDisplay();
		DisplayMetrics metrics = new DisplayMetrics();
		dpy.getMetrics(metrics);
		int xMM = (int)(((float)metrics.widthPixels / metrics.xdpi) * 25.4);
		int yMM = (int)(((float)metrics.heightPixels / metrics.ydpi) * 25.4);
		Context context = getApplicationContext();
		Vibrator vibrator = (Vibrator)context.getSystemService(Context.VIBRATOR_SERVICE);
		jEnvConfig(xMM, yMM, (int)dpy.getRefreshRate(), dpy, android.os.Build.DEVICE,
			context.getFilesDir().getAbsolutePath(), Environment.getExternalStorageDirectory().getAbsolutePath(),
			getApplicationInfo().sourceDir, vibrator);
	}
	
	public void addNotification(String onShow, String title, String message)
	{
		NotificationHelper.addNotification(getApplicationContext(), onShow, title, message);
	}
	
	public void removeNotification()
	{
		NotificationHelper.removeNotification();
	}
	
	/*@Override protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
	}*/
	
	@Override protected void onResume()
	{
		removeNotification();
		super.onResume();
	}
	
	@Override protected void onDestroy()
	{
		//Log.i(logTag, "onDestroy");
		removeNotification();
		super.onDestroy();
	}
	
	@Override public void surfaceDestroyed(SurfaceHolder holder)
	{
		super.surfaceDestroyed(holder);
		// In testing with CM7 on a Droid, the surface is re-created in RGBA8888 upon
		// resuming the app and ANativeWindow_setBuffersGeometry() has no effect.
		// Explicitly setting the format here seems to fix the problem. Android bug?
		getWindow().setFormat(PixelFormat.RGB_565);
	}
}