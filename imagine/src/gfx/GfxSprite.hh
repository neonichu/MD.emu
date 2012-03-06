#pragma once

#include <gfx/interface.h>
#include <gfx/GfxBufferImage.hh>
#include <gfx/GeomRect.hh>
#include <gfx/GeomQuad.hh>
#include <gfx/VertexArray.hh>

template<class BaseRect>
class GfxSpriteBase : public BaseRect
{
public:
	constexpr GfxSpriteBase(): img(0) { }
	CallResult init(Coordinate x = 0, Coordinate y = 0, Coordinate x2 = 0, Coordinate y2 = 0, GfxUsableImage *img = 0);
	CallResult init(GfxUsableImage *img)
	{
		return init(0, 0, 0, 0, img);
	}
	void deinit();
	void setImg(GfxUsableImage *img);
	void setImg(GfxUsableImage *img, GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV);

	void draw(uint manageBlend = 1) const;

	void deinitAndFreeImg()
	{
		deinit();
		if(img)
			img->deinit();
	}

	GfxUsableImage *img;
};

typedef GfxSpriteBase<GfxTexRect> GfxSprite;
typedef GfxSpriteBase<GfxColTexQuad> GfxShadedSprite;
