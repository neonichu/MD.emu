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

#define thisModuleName "filePicker"
#include <FilePicker.hh>
#include <EmuSystem.hh>
#include <Recent.hh>
#include <resource2/image/png/ResourceImagePng.h>
#include "ViewStack.hh"
#include <gui/FSPicker/FSPicker.hh>

extern ViewStack viewStack;
void startGameFromMenu();
void removeModalView();
bool isMenuDismissKey(const InputEvent &e);
void popupPrint(uint secs, bool error, const char *format, ...);
extern DLList<RecentGameInfo> recentGameList;

FsDirFilterFunc BaseFilePicker::defaultFsFilter = 0;
FsDirFilterFunc BaseFilePicker::defaultBenchmarkFsFilter = 0;

ResourceImage *getArrowAsset();
ResourceImage *getXAsset();

void BaseFilePicker::init(bool highlightFirst, FsDirFilterFunc filter, bool singleDir)
{
	FSPicker::init(".", needsUpDirControl ? getArrowAsset() : 0,
			View::needsBackControl ? getXAsset() : 0, filter, singleDir);
	if(highlightFirst && tbl.cells)
	{
		tbl.selected = 0;
	}
}

void GameFilePicker::onSelectFile(const char* name)
{
	if(EmuSystem::loadGame(name))
	{
		recent_addGame(EmuSystem::fullGamePath, EmuSystem::gameName);
		startGameFromMenu();
	}
}

void GameFilePicker::onClose()
{
	//dismiss(initMainMenu);
	viewStack.popAndShow();
}

void BenchmarkFilePicker::onSelectFile(const char* name)
{
	if(!EmuSystem::loadGame(name, 0))
		return;
	logMsg("starting benchmark");
	TimeSys time = EmuSystem::benchmark();
	EmuSystem::closeGame(0);
	logMsg("done in: %f", double(time));
	popupPrint(2, 0, "%.2f fps", double(180.)/double(time));
}

void BenchmarkFilePicker::onClose()
{
	removeModalView();
}

void BenchmarkFilePicker::inputEvent(const InputEvent &e)
{
	if(e.state == INPUT_PUSHED)
	{
		if(e.isDefaultCancelButton())
		{
			onClose();
			return;
		}

		if(isMenuDismissKey(e))
		{
			if(EmuSystem::gameIsRunning())
			{
				removeModalView();
				startGameFromMenu();
				return;
			}
		}
	}

	FSPicker::inputEvent(e);
}

void BenchmarkFilePicker::init(bool highlightFirst)
{
	BaseFilePicker::init(highlightFirst, defaultBenchmarkFsFilter);
}

#undef thisModuleName
