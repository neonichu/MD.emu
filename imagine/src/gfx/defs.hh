#pragma once

#include <engine-globals.h>
#include <util/number.h>
#include <util/Matrix4x4.hh>
#include <gfx/common/TextureSizeSupport.hh>

#ifdef CONFIG_GFX_OPENGL
	#include <gfx/opengl/gfx-globals.hh>
#endif

typedef TransformCoordinate GC;
typedef TransformCoordinate Coordinate;
typedef TextureCoordinate GTexC;


typedef struct IG::Point2D<GC> GfxPoint;

namespace Gfx
{
	typedef GfxPoint GP;

	template <class T>
	static GTexC pixelToTexC(T pixel, T total) { return (GTexC)pixel / (GTexC)total; }

	extern TextureSizeSupport textureSizeSupport;
}

static GfxPoint gfxP_makeXWithAR(Coordinate x, Coordinate aR)
{
	return GfxPoint(x, x / aR);
}

static GfxPoint gfxP_makeYWithAR(Coordinate y, Coordinate aR)
{
	return GfxPoint(y * aR, y);
}

class GeomRectDesc
{
public:
	constexpr GeomRectDesc(GC x = 0, GC y = 0, GC x2 = 0, GC y2 = 0) : x(x), y(y), x2(x2), y2(y2) { }
	Coordinate x,y, x2,y2;
};

class GfxTextureDesc
{
public:
	constexpr GfxTextureDesc() : tid(0), xStart(0), xEnd(0), yStart(0), yEnd(0) { }
	GfxTextureHandle tid;
	TextureCoordinate xStart, xEnd;
	TextureCoordinate yStart, yEnd;
};

class GfxUsableImage
{
public:
	virtual const GfxTextureDesc &textureDesc() const = 0;
	virtual void deinit() = 0;
};
