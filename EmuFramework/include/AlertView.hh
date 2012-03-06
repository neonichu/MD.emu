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

#include <gui/View.hh>
#include <util/gui/MenuItem.hh>
#include <util/gui/BaseMenuView.hh>
#include <util/rectangle2.h>

void removeModalView();

class AlertView : public View
{
public:
	struct NoMenuItem : public TextMenuItem
	{
		void init() { TextMenuItem::init("No"); }

		void select(View *view, const InputEvent &e)
		{
			removeModalView();
		}
	};

	Area labelFrame;
	GfxText text;
	BaseMenuView menu;
	Rect2<int> rect;

	Rect2<int> &viewRect() { return rect; }

	void init(const char *label, MenuItem **menuItem, bool highlightFirst);
	void deinit();

	void place(Rect2<int> rect)
	{
		View::place(rect);
	}

	void place();
	void inputEvent(const InputEvent &e);
	void draw();
};

class YesNoAlertView : public AlertView
{
public:

	class Callback
	{
	public:
		constexpr Callback() { }
		virtual void onSelect() = 0;
	};

	struct YesMenuItem : public TextMenuItem
	{
		Callback *callback;
		void init(Callback *callback)
		{
			TextMenuItem::init("Yes");
			var_selfs(callback);
		}

		void select(View *view, const InputEvent &e)
		{
			removeModalView();
			assert(callback);
			callback->onSelect();
		}
	} yes;

	NoMenuItem no;

	MenuItem *menuItem[2];

	void init(const char *label, Callback *callback, bool highlightFirst)
	{
		yes.init(callback); menuItem[0] = &yes;
		no.init(); menuItem[1] = &no;
		AlertView::init(label, menuItem, highlightFirst);
	}
};
