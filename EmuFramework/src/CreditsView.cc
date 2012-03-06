#define thisModuleName "creditsView"
#include <CreditsView.hh>
#include "ViewStack.hh"
extern ViewStack viewStack;

void CreditsView::draw ()
{
	using namespace Gfx;
	if(!updateAnimation())
		return;
	setColor(1., 1., 1., fade.m.now);
	//gfx_setColor(GCOLOR_WHITE);
	text.draw(/*(1.-fade.m.now)/8. * (displayState == DISMISS ? -1 : 1)*/
			gXPos(rect, C2DO), gYPos(rect, C2DO), C2DO, C2DO);
}

void CreditsView::place ()
{
	text.compile();
}

void CreditsView::inputEvent(const InputEvent &e)
{
	if((e.isPointer() && rect.overlaps(e.x, e.y) && e.state == INPUT_RELEASED)
			|| (!e.isPointer() && e.state == INPUT_PUSHED))
	{
		//dismiss();
		viewStack.popAndShow();
	}
}

void CreditsView::init ()
{
	text.init(str, View::defaultFace);
	place();
	View::init(&fade, 1);
}

void CreditsView::deinit ()
{
	text.deinit();
}

#undef thisModuleName
