#pragma once

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

#include <gfx/GfxText.hh>
#include <gui/View.hh>
#include <gui/GuiTable1D/GuiTable1D.hh>

class MenuItem
{
public:
	constexpr MenuItem() { }
	virtual void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const = 0;
	virtual void compile() = 0;
	virtual int ySize() = 0;
	virtual GC xSize() = 0;
	virtual void deinit() = 0;
	virtual void select(View *parent, const InputEvent &e) { bug_exit("unimplemented select()"); };
};


class TextMenuItem : public MenuItem
{
public:
	constexpr TextMenuItem(): active(0) { }
	GfxText t;
	bool active;

	void init(const char *str, bool active = 1, ResourceFace *face = View::defaultFace)
	{
		t.init(str, face);
		this->active = active;
	}

	void deinit()
	{
		t.deinit();
	}

	void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
	{
		using namespace Gfx;
		if(!active)
		{
			uint col = color();
			setColor(ColorFormat.r(col)/2, ColorFormat.g(col)/2, ColorFormat.b(col)/2, ColorFormat.a(col));
		}

		if(align.isXCentered())
			xPos += xSize/2;
		else
			xPos += GuiTable1D::globalXIndent;
		t.draw(xPos, yPos, align);
	}

	void compile() { t.compile(); }
	int ySize() { return t.face->nominalHeight(); }
	GC xSize() { return t.xSize; }
};

class DualTextMenuItem : public TextMenuItem
{
public:
	constexpr DualTextMenuItem() { }
	GfxText t2;

	void init(const char *str, const char *str2, bool active = 1, ResourceFace *face = View::defaultFace)
	{
		TextMenuItem::init(str, active, face);
		t2.init(str2, face);
	}

	void deinit()
	{
		t2.deinit();
		TextMenuItem::deinit();
	}

	void compile()
	{
		TextMenuItem::compile();
		t2.compile();
	}

	void draw2ndText(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
	{
		t2.draw((xPos + xSize) - GuiTable1D::globalXIndent, yPos, RC2DO, LT2DO);
	}

	void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
	{
		TextMenuItem::draw(xPos, yPos, xSize, ySize, align);
		DualTextMenuItem::draw2ndText(xPos, yPos, xSize, ySize, align);
	}
};


class BoolMenuItem : public DualTextMenuItem
{
public:
	constexpr BoolMenuItem(): on(0) { }
	bool on;

	void init(const char *str, bool on, bool active = 1, ResourceFace *face = View::defaultFace)
	{
		DualTextMenuItem::init(str, on ? "On" : "Off", active, face);
		this->on = on;
	}

	void set(bool val)
	{
		if(val != on)
		{
			//logMsg("setting bool: %d", val);
			on = val;
			t2.setString(val ? "On" : "Off");
			t2.compile();
			Base::displayNeedsUpdate();
		}
	}

	void toggle()
	{
		if(on)
			set(0);
		else
			set(1);
	}

	void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
	{
		using namespace Gfx;
		TextMenuItem::draw(xPos, yPos, xSize, ySize, align);
		if(on)
			setColor(0., 1., 0., 1.);
		else
			setColor(1., 0., 0., 1.);
		draw2ndText(xPos, yPos, xSize, ySize, align);
	}
};

class BoolTextMenuItem : public BoolMenuItem
{
public:
	constexpr BoolTextMenuItem(): offStr(0), onStr(0) { }
	const char *offStr, *onStr;
	void init(const char *str, const char *offStr, const char *onStr, bool on, bool active = 1, ResourceFace *face = View::defaultFace)
	{
		var_selfs(offStr);
		var_selfs(onStr);
		DualTextMenuItem::init(str, on ? onStr : offStr, active, face);
		var_selfs(on);
	}

	void set(bool val)
	{
		if(val != on)
		{
			//logMsg("setting bool: %d", val);
			on = val;
			t2.setString(val ? onStr : offStr);
			t2.compile();
			Base::displayNeedsUpdate();
		}
	}

	void toggle()
	{
		if(on)
			set(0);
		else
			set(1);
	}

	void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
	{
		using namespace Gfx;
		TextMenuItem::draw(xPos, yPos, xSize, ySize, align);
		setColor(0., 1., 1.); // aqua
		draw2ndText(xPos, yPos, xSize, ySize, align);
	}
};

class MultiChoiceMenuItem : public DualTextMenuItem
{
public:
	constexpr MultiChoiceMenuItem() : choice(0), choices(0), baseVal(0), choiceStr(0) { }
	int choice, choices, baseVal;
	const char **choiceStr;

	void init(const char *str, const char **choiceStr, int val, int max, int baseVal = 0, bool active = 1, const char *initialDisplayStr = 0, ResourceFace *face = View::defaultFace)
	{
		val -= baseVal;
		if(!initialDisplayStr) assert(val >= 0);
		DualTextMenuItem::init(str, initialDisplayStr ? initialDisplayStr : choiceStr[val], active, face);
		assert(val < max);
		choice = val;
		choices = max;
		this->baseVal = baseVal;
		this->choiceStr = choiceStr;
	}

	void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
	{
		using namespace Gfx;
		TextMenuItem::draw(xPos, yPos, xSize, ySize, align);
		setColor(0., 1., 1.); // aqua
		DualTextMenuItem::draw2ndText(xPos, yPos, xSize, ySize, align);
	}

	bool updateVal(int val)
	{
		assert(val >= 0 && val < choices);
		if(val != choice)
		{
			choice = val;
			t2.setString(choiceStr[val]);
			t2.compile();
			Base::displayNeedsUpdate();
			return 1;
		}
		return 0;
	}

	void set(int val)
	{
		if(updateVal(val))
		{
			doSet(val + baseVal);
		}
	}

	virtual void doSet(int val) { }

	void cycle(int direction)
	{
		if(direction > 0)
			set(IG::incWrapped(choice, choices));
		else if(direction < 0)
			set(IG::decWrapped(choice, choices));
	}
};

