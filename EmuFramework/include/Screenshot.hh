#pragma once

#include <stdlib.h>
#include <pixmap/Pixmap.hh>
#include <EmuSystem.hh>

bool writeScreenshot(const Pixmap &vidPix, const char *fname);

template <size_t S>
static int sprintScreenshotFilename(char (&str)[S])
{
	const uint maxNum = 999;
	int num = -1;
	iterateTimes(maxNum, i)
	{
		snprintf(str, S, "%s/%s.%.3d.png", EmuSystem::gamePath, EmuSystem::gameName, i);
		if(!Fs::fileExists(str))
		{
			num = i;
			break;
		}
	}
	if(num == -1)
	{
		logMsg("no screenshot filenames left");
		return -1;
	}
	logMsg("screenshot %d", num);
	return num;
}
