#pragma once

#include <gui/View.hh>
#include <gui/FSPicker/FSPicker.hh>

class ViewStack
{
	View *view[5];
	NavView *nav;
	Rect2<int> viewRect, customViewRect;
public:
	uint size;
	bool useNavView;
	constexpr ViewStack() :
	#ifdef CONFIG_CXX11
			view({0}),
	#endif
			nav(0), size(0), useNavView(0) { }

	void init()
	{
		viewRect = Gfx::viewportRect();
	}

	void setNavView(NavView *nav)
	{
		var_selfs(nav);
		if(nav)
		{
			nav->setLeftBtnActive(size > 1);
			useNavView = 1;
		}
	}

	void place(const Rect2<int> &rect)
	{
		viewRect = rect;
		place();
	}

	void place()
	{
		customViewRect = viewRect;
		if(useNavView && nav)
		{
			nav->setTitle(top()->name());
			nav->viewRect.setPosRel(viewRect.x, viewRect.y, viewRect.xSize(), nav->text.face->nominalHeight()*1.75, LT2DO);
			nav->place();
			customViewRect.y += nav->viewRect.ySize();
		}
		top()->place(customViewRect);
	}

	void inputEvent(const InputEvent &e)
	{
		if(useNavView && nav && e.isPointer() && nav->viewRect.overlaps(e.x, e.y))
		{
			nav->inputEvent(e);
		}
		top()->inputEvent(e);
	}

	void draw()
	{
		top()->draw();
		if(useNavView && nav) nav->draw();
	}

	void push(View *v)
	{
		assert(size != sizeofArray(view));
		view[size] = v;
		size++;
		logMsg("push view, %d in stack", size);

		if(nav)
		{
			nav->setLeftBtnActive(size > 1);
		}
	}

	void pushAndShow(View *v)
	{
		push(v);
		place();
		v->show();
		Base::displayNeedsUpdate();
	}

	void pop()
	{
		top()->deinit();
		size--;
		logMsg("pop view, %d in stack", size);
		assert(size != 0);

		if(nav)
		{
			nav->setLeftBtnActive(size > 1);
			nav->setTitle(top()->name());
			useNavView = 1;
		}
	}

	void popAndShow()
	{
		pop();
		place();
		top()->show();
		Base::displayNeedsUpdate();
	}

	void popToRoot()
	{
		while(size > 1)
			pop();
		place();
		top()->show();
		Base::displayNeedsUpdate();
	}

	void show()
	{
		top()->show();
	}

	View *top() const
	{
		assert(size != 0);
		return view[size-1];
	}
};
