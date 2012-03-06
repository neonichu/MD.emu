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

#include <gfx/GfxText.hh>
#include <gfx/GeomRect.hh>

class MsgPopup
{
public:
	GfxText text;
	bool error;

	void init();
	void clear();
	void place();
	static void unpost(void *ctx);
	void post(const char *msg, int secs = 3, bool error = 0);
	void postError(const char *msg, int secs = 3);
	void draw();
};
