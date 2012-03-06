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

#pragma once

#include <io/Io.hh>
#include <fs/sys.hh>
#include <util/time/sys.hh>
#include <util/audio/PcmFormat.hh>
#include <config/env.hh>
#include <gui/FSPicker/FSPicker.hh>
#include <ViewStack.hh>

extern BasicNavView viewNav;

class EmuSystem
{
	public:
	static bool active;
	static FsSys::cPath gamePath, fullGamePath;
	static char gameName[256];
	static int autoSaveStateFrameCount, autoSaveStateFrames;
	static int saveStateSlot;
	static TimeSys startTime;
	static uint emuFrameNow;
	static Audio::PcmFormat pcmFormat;
	static const uint optionFrameSkipAuto;

	static void resetAutoSaveStateTime()
	{
		autoSaveStateFrameCount = autoSaveStateFrames;
		logMsg("reset auto-save state counter to %d", autoSaveStateFrameCount);
	}
	static void setupAutoSaveStateTime(int option)
	{
		switch(option)
		{
			bcase 0 ... 1: autoSaveStateFrames = 0;
			bdefault:
				autoSaveStateFrames = 60*60*option; // minutes to frames
		}
		resetAutoSaveStateTime();
	}

	static int loadState();
	static int saveState();
	static bool stateExists(int slot);
	static void saveAutoState();
	static void saveBackupMem();
	static void resetGame();
	static void initOptions();
	static void writeConfig(Io *io);
	static bool readConfig(Io *io, uint key, uint readSize);
	static int loadGame(const char *path, bool allowAutosaveState = 1);
	static void runFrame(bool renderGfx, bool processGfx, bool renderAudio) ATTRS(hot);
	static bool vidSysIsPAL();
	static void configAudioRate();
	static void clearInputBuffers();
	static void handleInputAction(uint player, uint state, uint emuKey);
	static uint translateInputAction(uint input, bool &turbo);
	static void handleOnScreenInputAction(uint state, uint emuKey);
	static void stopSound();
	static void startSound();
	static int setupFrameSkip(uint optionVal);

	static TimeSys benchmark()
	{
		TimeSys now, after;
		now.setTimeNow();
		iterateTimes(180, i)
		{
			runFrame(0, 1, 0);
		}
		after.setTimeNow();
		return after-now;
	}

	static bool gameIsRunning()
	{
		return !string_equal(gameName, "");
	}

	static void pause()
	{
		active = 0;
		stopSound();
	}

	static void start()
	{
		active = 1;
		clearInputBuffers();
		emuFrameNow = 0;
		startSound();
		startTime.setTimeNow();
	}

	static void closeSystem();
	static void closeGame(bool allowAutosaveState = 1)
	{
		if(gameIsRunning())
		{
			if(allowAutosaveState)
				saveAutoState();
			logMsg("closing game %s", gameName);
			closeSystem();
			strcpy(gameName, "");
			resetAutoSaveStateTime();
			viewNav.setRightBtnActive(0);
		}
	}
};
