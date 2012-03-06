#pragma once

#include <util/gui/MenuItem.hh>
#include <util/gui/BaseMenuView.hh>

class BaseMultiChoiceView : public BaseMenuView
{
public:
	constexpr BaseMultiChoiceView() { }
	Rect2<int> viewFrame;

	void inputEvent(const InputEvent &e)
	{
		if(e.state == INPUT_PUSHED)
		{
			if(e.isDefaultCancelButton())
			{
				removeModalView();
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

		BaseMenuView::inputEvent(e);
	}

	void place(Rect2<int> rect)
	{
		View::place(rect);
	}

	void place()
	{
		GC maxWidth = 0;
		iterateTimes(items, i)
		{
			item[i]->compile();
			maxWidth = IG::max(maxWidth, item[i]->xSize());
		}

		tbl.setYCellSize(item[0]->ySize()*2);

		viewFrame.setPosRel(Gfx::viewPixelWidth()/2, Gfx::viewPixelHeight()/2,
				Gfx::viewPixelWidth(), Gfx::viewPixelHeight(), C2DO);
		tbl.place(&viewFrame);
	}

	void draw()
	{
		using namespace Gfx;
		resetTransforms();
		setBlendMode(0);
		setColor(.2, .2, .2, 1.);
		GeomRect::draw(viewFrame);
		BaseMenuView::draw();
	}
};

class MultiChoiceView : public BaseMultiChoiceView
{
public:
	constexpr MultiChoiceView(): srcEntry(0)
	#ifdef CONFIG_CXX11
	, choiceEntryItem({0})
	#endif
	{ }
	MultiChoiceMenuItem *srcEntry;
	TextMenuItem choiceEntry[13];
	MenuItem *choiceEntryItem[13];

	void init(MultiChoiceMenuItem *src, bool highlightCurrent)
	{
		assert((uint)src->choices <= sizeofArray(choiceEntry));
		iterateTimes(src->choices, i)
		{
			choiceEntry[i].init(src->choiceStr[i], src->t2.face);
			choiceEntryItem[i] = &choiceEntry[i];
		}
		BaseMenuView::init(choiceEntryItem, src->choices, 0, C2DO);
		srcEntry = src;
		if(highlightCurrent)
		{
			tbl.selected = src->choice;
		}
	}

	void onSelectElement(const GuiTable1D *table, const InputEvent &e, uint i)
	{
		assert((int)i < srcEntry->choices);
		logMsg("set choice %d", i);
		srcEntry->set(i);
		removeModalView();
	}
};

// TODO: merge with MultiChoiceView
class MultiPickView : public BaseMultiChoiceView
{
public:
	constexpr MultiPickView(): callback(0)
	#ifdef CONFIG_CXX11
	, choiceEntryItem({0})
	#endif
	{ }

	struct Callback
	{
		virtual bool doSet(int val, const InputEvent &e) = 0;
	};

	Callback *callback;
	TextMenuItem choiceEntry[13];
	MenuItem *choiceEntryItem[13];

	void init(const char **choice, uint choices, Callback *callback, bool highlightCurrent)
	{
		assert(choices <= sizeofArray(choiceEntry));
		iterateTimes(choices, i)
		{
			choiceEntry[i].init(choice[i]);
			choiceEntryItem[i] = &choiceEntry[i];
		}
		BaseMenuView::init(choiceEntryItem, choices, highlightCurrent, C2DO);
		var_selfs(callback);
	}

	void onSelectElement(const GuiTable1D *table, const InputEvent &e, uint i)
	{
		logMsg("set choice %d", i);
		if(callback->doSet(i, e))
			removeModalView();
	}
};
