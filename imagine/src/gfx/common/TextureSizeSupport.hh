#pragma once

class TextureSizeSupport
{
public:
	uchar nonPow2;
	uchar nonSquare;
	uchar filtering;
	uint minXSize, minYSize;
	uint maxXSize, maxYSize;

	static const uint streamHint = BIT(0);

	void findBufferXYPixels(uint &x, uint &y, uint imageX, uint imageY, uint hints = 0)
	{
		using namespace IG;
		if(nonPow2
			/*#ifndef CONFIG_BASE_PS3
				&& (hints & streamHint) // don't use npot for static use textures
			#endif*/
			)
		{
			x = imageX;
			y = imageY;
		}
		else if(nonSquare)
		{
			x = nextHighestPowerOf2(imageX);
			y = nextHighestPowerOf2(imageY);
		}
		else
		{
			x = nextHighestPowerOf2(IG::max(imageX,imageY));
			y = nextHighestPowerOf2(IG::max(imageX,imageY));
		}

		if(minXSize && x < minXSize) x = minXSize;
		if(minYSize && y < minYSize) y = minYSize;
	}

};
