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

#define thisModuleName "gfx:opengl"
#include <engine-globals.h>
#include <gfx/interface.h>
#include <logger/interface.h>
#include <mem/interface.h>
#include <base/interface.h>
#include <util/number.h>

#if defined(CONFIG_RESOURCE_IMAGE)
#include <resource2/image/ResourceImage.h>
#endif

#ifdef CONFIG_BASE_WIN32
	#include <env/win.mingw/windows.h>
#endif

#if defined(CONFIG_BASE_IOS)
	#define CONFIG_GFX_OPENGL_ES 1
	#import <OpenGLES/ES1/gl.h>
	#import <OpenGLES/ES1/glext.h>
#elif defined(CONFIG_BASE_ANDROID)
	#define CONFIG_GFX_OPENGL_ES 1
	#define GL_GLEXT_PROTOTYPES
	#include <GLES/gl.h>
	#include <GLES/glext.h>
#elif defined(CONFIG_ENV_WEBOS) // standard GL headers seem to be missing some extensions
	#define CONFIG_GFX_OPENGL_ES 1
	#define GL_GLEXT_PROTOTYPES
	#include <GLES/gl.h>
	#include <SDL/SDL_opengles_ext.h>
#elif defined(CONFIG_BASE_PS3)
	#define CONFIG_GFX_OPENGL_ES 1
	#include <PSGL/psgl.h>
#endif

#if defined(CONFIG_BASE_X11)
	#if defined(CONFIG_GFX_OPENGL_ES)
		#define GL_GLEXT_PROTOTYPES
		#include <GLES/gl.h>
		#include <GLES/glext.h>
	#else
		#define CONFIG_GFX_OPENGL_GLX
	#endif
#endif

#ifdef CONFIG_GFX_OPENGL_ES
	#if !defined(CONFIG_BASE_PS3)
		#define glBlendEquation glBlendEquationOES
		#define GL_FUNC_ADD GL_FUNC_ADD_OES
		#define GL_FUNC_SUBTRACT GL_FUNC_SUBTRACT_OES
		#define GL_FUNC_REVERSE_SUBTRACT GL_FUNC_REVERSE_SUBTRACT_OES
	#endif

	#ifdef GL_USE_OES_FIXED
		#define glTranslatef glTranslatex
		static void glTranslatex(TransformCoordinate x, TransformCoordinate y, TransformCoordinate z)
		{
			glTranslatexOES(TransformCoordinatePOD(x), TransformCoordinatePOD(y), TransformCoordinatePOD(z));
		}
		#define glScalef glScalex
		static void glScalex(TransformCoordinate sx, TransformCoordinate sy, TransformCoordinate sz)
		{
			glScalexOES(TransformCoordinatePOD(sx), TransformCoordinatePOD(sy), TransformCoordinatePOD(sz));
		}
		#define glRotatef glRotatex
		static void glRotatex(Angle angle, Angle x, Angle y, Angle z)
		{
			glRotatexOES(AnglePOD(angle), AnglePOD(x), AnglePOD(y), AnglePOD(z));
		}
	#endif
#endif

#ifdef CONFIG_GFX_OPENGL_GLEW_STATIC
	#if 0
	// configure GetProcAddress depending on platform
	#if defined(_WIN32)
		// wglGetProcAddress included by default
	#else
		#if defined(__APPLE__)
			// NSGLGetProcAddress included in glew source
		#else
			#if defined(__sgi) || defined(__sun)
				// dlGetProcAddress has not been tested yet
			#else /* __linux */
				#define USE_GLX_ARB_get_proc_address
			#endif
		#endif
	#endif
	#endif

	//#ifdef CONFIG_BASE_X11
	//#define USE_GLX_ARB_get_proc_address
	//#endif

	// extensions we want to compile in
	/*#define USE_GL_VERSION_1_2
	#define USE_GL_VERSION_1_3
	#define USE_GL_VERSION_1_4
	#define USE_GL_EXT_texture_filter_anisotropic
	#define USE_GL_SGIS_generate_mipmap
	#define USE_GL_ARB_multisample
	#define USE_GL_NV_multisample_filter_hint
	#define USE_GL_ARB_texture_non_power_of_two
	#define USE_GL_EXT_framebuffer_object
	#define USE_GL_ARB_imaging*/

	#define GLEW_STATIC
	#include <util/glew/glew.h>

	#ifdef CONFIG_BASE_WIN32
		#define USE_WGL_ARB_extensions_string
		#define USE_WGL_EXT_extensions_string
		#define USE_WGL_ARB_pixel_format
		#define USE_WGL_EXT_swap_control
		#define USE_WGL_ARB_multisample
		#include <util/glew/wglew.h>
	#endif
#elif !defined(CONFIG_GFX_OPENGL_ES)
	// for using the standard GLEW lib
	#include <GL/glew.h>
	#ifdef CONFIG_BASE_WIN32
		#include <GL/wglew.h>
	#endif
	#ifdef CONFIG_BASE_X11
		#define Time X11Time_
		#define Pixmap X11Pixmap_
		#define GC X11GC_
		#define Window X11Window
		#define BOOL X11BOOL
		#include <GL/glxew.h>
		#undef Time
		#undef Pixmap
		#undef GC
		#undef Window
		#undef BOOL
	#endif
#endif

static const char *extensions, *version, *rendererName;

uint gfx_frameTime = 0, gfx_frameTimeRel = 0;

#ifdef CONFIG_GFX_OPENGL_GLEW_STATIC
	#define Pixmap _X11Pixmap
	#include <util/glew/glew.c>
	#undef Pixmap
#endif

#if defined(__APPLE__) && ! defined (CONFIG_BASE_IOS)
	#include <OpenGL/OpenGL.h>
#endif

DefCallbackWithCtx(ViewChangeFunc, gfx_viewChangeHandler, viewChangeHandler);
DefCallbackWithCtx(DrawHandlerFunc, gfx_drawHandler, drawHandler);

static const char *glErrorToString(GLenum err)
{
	switch(err)
	{
		case GL_NO_ERROR: return "No Error";
		case GL_INVALID_ENUM: return "Invalid Enum";
		case GL_INVALID_VALUE: return "Invalid Value";
		case GL_INVALID_OPERATION: return "Invalid Operation";
		case GL_STACK_OVERFLOW: return "Stack Overflow";
		case GL_STACK_UNDERFLOW: return "Stack Underflow";
		case GL_OUT_OF_MEMORY: return "Out of Memory";
		default: return "Unknown Error";
	}
}

#ifndef NDEBUG
	#define CHECK_GL_ERRORS
#endif

#ifndef CHECK_GL_ERRORS
	static void clearGLError() { }
	#define glErrorCase(errorLabel) for(int errorLabel = 0; errorLabel == 1;)
#else
	static void clearGLError() { while (glGetError() != GL_NO_ERROR) { } }
	#define glErrorCase(errorLabel) for(GLenum errorLabel = glGetError(); errorLabel != GL_NO_ERROR; errorLabel = GL_NO_ERROR)
#endif

#include "glStateCache.h"
#include <util/Matrix4x4.hh>
#include <util/Motion.hh>
#include <gfx/common/space.h>

static int animateOrientationChange = 1;
TimedMotion<GC> projAngleM;

#include "settings.h"
#include "transforms.hh"

/*#if !defined(CONFIG_BASE_IOS) && !defined(CONFIG_BASE_ANDROID)
	#define USE_TEX_CACHE
	typedef uint TexRef;
	void texReuseCache_getTexRef(TexRef *ref) { glGenTextures(1, ref); }
	void texReuseCache_deleteTexRef(TexRef *ref) { glcDeleteTextures(1, ref); }
	#include <testing/gfx/textureReuseCache.h>
	#define TEX_CACHE_ENTRIES 128
	static TexCacheNode texCacheStore[TEX_CACHE_ENTRIES];
	static TextureReuseCache texCache;
#else*/
	static uint texReuseCache_getExistingUsable(uint *hnd, uint x, uint y, uint format) { return 0; }
	static uint texReuseCache_getNew(uint *hnd, uint x, uint y, uint format)
	{
		GLuint ref;
		glGenTextures(1, &ref);
		return ref;
	}
	static void texReuseCache_doneUsing(uint *hnd, uint texRef) { glcDeleteTextures(1, &texRef); }
	static uint texCache;
//#endif

#include "geometry.hh"
#include "texture.hh"

#include "startup-shutdown.h"
#include "commit.hh"

namespace Gfx
{

void setActiveTexture(GfxTextureHandle texture)
{
	if(texture != 0)
	{
		glcBindTexture(GL_TEXTURE_2D, texture);
		glcEnable(GL_TEXTURE_2D);
	}
	else
	{
		glcDisable(GL_TEXTURE_2D);
		// TODO: binding texture 0 causes implicit glDisable(GL_TEXTURE_2D) ?
	}
}

void setZTest(bool on)
{
	if(on)
	{
		glcEnable(GL_DEPTH_TEST);
		clearZBufferBit = GL_DEPTH_BUFFER_BIT;
	}
	else
	{
		glcDisable(GL_DEPTH_TEST);
		clearZBufferBit = 0;
	}
}

void setBlendMode(uint mode)
{
	switch(mode)
	{
		bcase BLEND_MODE_OFF:
			glcDisable(GL_BLEND);
		bcase BLEND_MODE_ALPHA:
			glcBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); // general blending
			//glcBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // for premultiplied alpha
			glcEnable(GL_BLEND);
		bcase BLEND_MODE_INTENSITY:
			glcBlendFunc(GL_SRC_ALPHA,GL_ONE); // for text
			glcEnable(GL_BLEND);
	}
}

void setImgMode(uint mode)
{
	switch(mode)
	{
		bcase IMG_MODE_REPLACE: glcTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		bcase IMG_MODE_MODULATE: glcTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		bcase IMG_MODE_BLEND: glcTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
			//GLfloat col[4] = { 1, 1, 0, 0 } ;
			//glcTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, col);
	}
}

void setBlendEquation(uint mode)
{
#ifndef CONFIG_ENV_WEBOS
	glcBlendEquation(mode == BLEND_EQ_ADD ? GL_FUNC_ADD :
			mode == BLEND_EQ_SUB ? GL_FUNC_SUBTRACT :
			mode == BLEND_EQ_RSUB ? GL_FUNC_REVERSE_SUBTRACT :
			GL_FUNC_ADD);
#endif
}

void setImgBlendColor(GColor r, GColor g, GColor b, GColor a)
{
	GLfloat col[4] = { r, g, b, a } ;
	glcTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, col);
}

void setZBlend(uchar on)
{
	if(on)
	{
		#ifndef CONFIG_GFX_OPENGL_ES
			glFogi(GL_FOG_MODE, GL_LINEAR);
		#else
			glFogf(GL_FOG_MODE, GL_LINEAR);
		#endif
		glFogf(GL_FOG_DENSITY, 0.1f);
		glHint(GL_FOG_HINT, GL_DONT_CARE);
		glFogf(GL_FOG_START, zRange/2.0);
		glFogf(GL_FOG_END, zRange);
		glcEnable(GL_FOG);
	}
	else
	{
		glcDisable(GL_FOG);
	}
}

void setZBlendColor(GColor r, GColor g, GColor b)
{
	GLfloat c[4] = {r, g, b, 1.0f};
	glFogfv(GL_FOG_COLOR, c);
}

void setColor(GColor r, GColor g, GColor b, GColor a)
{
	glcColor4f(r, g, b, a);
}

uint color()
{
	return ColorFormat.build(glState.colorState[0], glState.colorState[1], glState.colorState[2], glState.colorState[3]);
}

void setVisibleGeomFace(uint faces)
{
	if(faces == BOTH_FACES)
	{
		glcDisable(GL_CULL_FACE);
	}
	else if(faces == FRONT_FACES)
	{
		glcEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT); // our order is reversed from OpenGL
	}
	else
	{
		glcEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}
}

}

void pointerPos(int x, int y, int *xOut, int *yOut);

namespace Gfx
{

void setClipRect(bool on)
{
	if(on)
		glcEnable(GL_SCISSOR_TEST);
	else
		glcDisable(GL_SCISSOR_TEST);
}

void setClipRectBounds(int x, int y, int w, int h)
{
	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	switch(rotateView)
	{
		bcase VIEW_ROTATE_0:
			y = (viewPixelHeight() - y) - h;
		bcase VIEW_ROTATE_90:
			y = (viewPixelHeight() - y) - h;
			IG::swap(x, y);
			IG::swap(w, h);
		bcase VIEW_ROTATE_270:
			IG::swap(x, y);
			IG::swap(w, h);
			//TODO: VIEW_ROTATE_180
	}
	#else
	y = (viewPixelHeight() - y) - h;
	#endif
	//logMsg("setting Scissor %d,%d size %d,%d", x, y, w, h);
	glScissor(x, y, w, h);
}

void setClear(bool on)
{
	// always clear screen on android since not doing so seems to have a performance hit
	#if !defined(CONFIG_GFX_OPENGL_ES)
		clearColorBufferBit = on ? GL_COLOR_BUFFER_BIT : 0;
	#endif
}

void setClearColor(GColor r, GColor g, GColor b, GColor a)
{
	//GLfloat c[4] = {r, g, b, a};
	//logMsg("setting clear color %f %f %f %f", (float)r, (float)g, (float)b, (float)a);
	// TODO: add glClearColor to the state cache
	glClearColor((float)r, (float)g, (float)b, (float)a);
}

}

#undef thisModuleName
