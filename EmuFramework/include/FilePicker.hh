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

#include <gui/FSPicker/FSPicker.hh>

class BaseFilePicker : public FSPicker
{
public:
	constexpr BaseFilePicker() { }
	static FsDirFilterFunc defaultFsFilter;
	static FsDirFilterFunc defaultBenchmarkFsFilter;

	void init(bool highlightFirst, FsDirFilterFunc filter = defaultFsFilter, bool singleDir = 0);
};

class GameFilePicker : public BaseFilePicker
{
public:
	void onSelectFile(const char* name);

	void onClose();
};

class BenchmarkFilePicker : public BaseFilePicker
{
public:
	constexpr BenchmarkFilePicker() { }
	void onSelectFile(const char* name);

	void onClose();

	void inputEvent(const InputEvent &e);

	void init(bool highlightFirst);
};
