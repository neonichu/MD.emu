#pragma once

#include <engine-globals.h>
#include <util/pixel.h>

class PixmapDesc
{
public:
	uint x, y;
	uint pitch;
	const PixelFormatDesc *format;

	uint sizeOfNumPixels(uint num) const
	{
		return(num * format->bytesPerPixel);
	}

	uint sizeOfImage() const
	{
		return(x * y * format->bytesPerPixel);
	}

	uint pitchPixels() const
	{
		return pitch / format->bytesPerPixel;
	}

	bool isPadded() const
	{
		return x != pitchPixels();
	}
};

class Pixmap : public PixmapDesc
{
private:
	//uchar byteOrder;
	uchar *nextPixelOnLine(uchar *pixel) const;

public:
	uchar *data;

	/*Pixmap() { }
	Pixmap(uchar *data, const PixelFormatDesc *format, uint x, uint y, uint extraPitch = 0)
	{
		init(data, format, x, y, extraPitch);
	}*/

	uchar *getPixel(uint x, uint y) const;

	void init(uchar *data, const PixelFormatDesc *format, uint x, uint y, uint extraPitch = 0)
	{
		this->data = data;
		this->format = format;
		this->x = x;
		this->y = y;
		this->pitch = (x * format->bytesPerPixel) + extraPitch;
		//logMsg("init %dx%d pixmap, pitch %d", x, y, pitch);
	}

	void init(const PixelFormatDesc *format, uint x, uint y, uint extraPitch = 0)
	{
		uchar *data = (uchar*)mem_alloc(x * y * format->bytesPerPixel + extraPitch * y);
		init(data, format, x, y, extraPitch);
	}

	void copy(int srcX, int srcY, int width, int height, Pixmap *dest, int destX, int destY) const;
	void initSubPixmap(const Pixmap &orig, uint x, uint y, uint xlen, uint ylen);
	void copyHLineToRectFromSelf(uint xStart, uint yStart, uint xlen, uint xDest, uint yDest, uint yDestLen);
	void copyVLineToRectFromSelf(uint xStart, uint yStart, uint ylen, uint xDest, uint yDest, uint xDestLen);
	void copyPixelToRectFromSelf(uint xStart, uint yStart, uint xDest, uint yDest, uint xDestLen, uint yDestLen);
	void clearRect(uint xStart, uint yStart, uint xlen, uint ylen);
	void subPixmapOffsets(Pixmap *sub, uint *x, uint *y) const;
};
