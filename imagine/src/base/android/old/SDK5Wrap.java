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

import android.os.*;
import android.view.*;
import android.content.res.Configuration;
import android.util.Log;

final class SDK5Wrap
{
	static
	{
		if(android.os.Build.VERSION.SDK_INT < 5)
			throw new RuntimeException();
	}
	public static void checkAvailable() {}
	
	private static String logTag = "SDK5Wrap";
	
	private static void dumpEvent(MotionEvent event)
	{
		String names[] = { "DOWN" , "UP" , "MOVE" , "CANCEL" , "OUTSIDE" ,
		  "POINTER_DOWN" , "POINTER_UP" , "7?" , "8?" , "9?" };
		StringBuilder sb = new StringBuilder();
		int action = event.getAction();
		int actionCode = action & MotionEvent.ACTION_MASK;
		sb.append("event ACTION_" ).append(names[actionCode]);
		if (actionCode == MotionEvent.ACTION_POINTER_DOWN
			 || actionCode == MotionEvent.ACTION_POINTER_UP) {
		  sb.append("(pid " ).append(
		  action >> MotionEvent.ACTION_POINTER_ID_SHIFT);
		  sb.append(")" );
		}
		sb.append("[" );
		for (int i = 0; i < event.getPointerCount(); i++) {
		  sb.append("#" ).append(i);
		  sb.append("(pid " ).append(event.getPointerId(i));
		  sb.append(")=" ).append((int) event.getX(i));
		  sb.append("," ).append((int) event.getY(i));
		  if (i + 1 < event.getPointerCount())
			 sb.append(";" );
		}
		sb.append("]" );
		Log.i("", sb.toString());
	}

	public static boolean onTouchEvent(final MotionEvent event)
	{
		//dumpEvent(event);
		//Log.i(logTag, "pointers: " + event.getPointerCount());
		
		int eventAction = event.getAction();
		int action = eventAction & MotionEvent.ACTION_MASK;
		int pid = eventAction >> MotionEvent.ACTION_POINTER_ID_SHIFT;
		// 2.2 API
		//int action = event.getActionMasked();
		//int pid = event.getPointerId(event.getActionIndex());
		
		for (int i = 0; i < event.getPointerCount(); i++)
		{
			int sendAction = action;
			int pointerId = event.getPointerId(i);
			if(action == MotionEvent.ACTION_POINTER_DOWN || action == MotionEvent.ACTION_POINTER_UP)
			{
				// send ACTION_POINTER_* for only the pointer it belongs to
				if(pid != pointerId)
					sendAction = MotionEvent.ACTION_MOVE;
			}
			if(GLView.touchEvent(sendAction, (int)event.getX(i), (int)event.getY(i), pointerId))
			{
				GLView.postUpdate();
			}
		}
		return true;
	}
	
	public static int getNavigationHidden(Configuration config)
	{
		//Log.i(logTag, "called getNavigationHidden");
		return config.navigationHidden;
	}
}
