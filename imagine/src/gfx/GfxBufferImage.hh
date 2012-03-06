#pragma once

#include <gfx/defs.hh>
#include <pixmap/Pixmap.hh>

#if defined(CONFIG_RESOURCE_IMAGE)
class ResourceImage;
#endif

#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
	bool gfx_androidDirectTextureSupported();
	bool gfx_androidDirectTextureSupportWhitelisted();
	int gfx_androidDirectTextureError();
	void gfx_setAndroidDirectTexture(bool on);

	enum
	{
		ANDROID_DT_SUCCESS,
		ANDROID_DT_ERR_NO_EGL_IMG_EXT,
		ANDROID_DT_ERR_NO_LIBEGL, ANDROID_DT_ERR_NO_CREATE_IMG, ANDROID_DT_ERR_NO_DESTROY_IMG,
		ANDROID_DT_ERR_NO_GET_DISPLAY, ANDROID_DT_ERR_GET_DISPLAY,
		ANDROID_DT_ERR_NO_LIBHARDWARE, ANDROID_DT_ERR_NO_HW_GET_MODULE,
		ANDROID_DT_ERR_NO_GRALLOC_MODULE, ANDROID_DT_ERR_NO_ALLOC_DEVICE,

		ANDROID_DT_ERR_TEST_ALLOC, ANDROID_DT_ERR_TEST_LOCK,
		ANDROID_DT_ERR_TEST_UNLOCK, ANDROID_DT_ERR_TEST_IMG_CREATE,
		ANDROID_DT_ERR_TEST_TARGET_TEXTURE,

		ANDROID_DT_ERR_FORCE_DISABLE
	};

	static const char *gfx_androidDirectTextureErrorString(int err)
	{
		static bool verbose = 0;
		switch(err)
		{
			case ANDROID_DT_SUCCESS: return "OK";
			case ANDROID_DT_ERR_NO_EGL_IMG_EXT: return "no OES_EGL_image extension";
			case ANDROID_DT_ERR_NO_LIBEGL: if(verbose) return "can't load libEGL.so";
			case ANDROID_DT_ERR_NO_CREATE_IMG: if(verbose) return "can't find eglCreateImageKHR";
			case ANDROID_DT_ERR_NO_DESTROY_IMG: if(verbose) return "can't find eglDestroyImageKHR";
			case ANDROID_DT_ERR_NO_GET_DISPLAY: if(verbose) return "can't find eglGetDisplay";
			case ANDROID_DT_ERR_GET_DISPLAY: if(verbose) return "unable to get default EGL display";
				return "unsupported libEGL";
			case ANDROID_DT_ERR_NO_LIBHARDWARE: if(verbose) return "can't load libhardware.so";
			case ANDROID_DT_ERR_NO_HW_GET_MODULE: if(verbose) return "can't find hw_get_module";
			case ANDROID_DT_ERR_NO_GRALLOC_MODULE: if(verbose) return "can't load gralloc";
			case ANDROID_DT_ERR_NO_ALLOC_DEVICE: if(verbose) return "can't load allocator";
				return "unsupported libhardware";
			case ANDROID_DT_ERR_TEST_ALLOC: return "allocation failed";
			case ANDROID_DT_ERR_TEST_LOCK: return "lock failed";
			case ANDROID_DT_ERR_TEST_UNLOCK: return "unlock failed";
			case ANDROID_DT_ERR_TEST_IMG_CREATE: return "eglCreateImageKHR failed";
			case ANDROID_DT_ERR_TEST_TARGET_TEXTURE: return "glEGLImageTargetTexture2DOES failed";
			case ANDROID_DT_ERR_FORCE_DISABLE: return "unsupported GPU";
		}
		return "";
	}
#endif

class GfxBufferImage : public GfxTextureDesc, public GfxUsableImage
{
public:
	// TODO: getting an ICE in GCC 4.6.2 if constexpr is added here
	GfxBufferImage()
	#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
	: isDirect(0)
	#endif
	{ }

	static const uint nearest = 0, linear = 1;
	static bool isFilterValid(uint v) { return v <= 1; }

	CallResult init(Pixmap &pix, bool upload, uint filter = linear, uint hints = 1);
	#if defined(CONFIG_RESOURCE_IMAGE)
	CallResult init(ResourceImage &img, uint filter = linear);
	CallResult subInit(ResourceImage &img, int x, int y, int xSize, int ySize);
	#endif

	void setFilter(uint filter);
	void write(Pixmap &p);
	void replace(Pixmap &p);
	Pixmap *lock(uint x, uint y, uint xlen, uint ylen);
	void unlock();
	void deinit();
	const GfxTextureDesc &textureDesc() const { return *this; }

private:
	uint hints;
#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
public:
	bool isDirect;
#endif
};
