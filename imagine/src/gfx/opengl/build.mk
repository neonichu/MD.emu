ifndef inc_gfx
inc_gfx := 1

include $(imagineSrcDir)/base/system.mk
include $(imagineSrcDir)/pixmap/build.mk

configDefs += CONFIG_GFX CONFIG_GFX_OPENGL

ifeq ($(ENV), linux)
 ifdef config_gfx_openGLES
 LDLIBS += -lGLESv1_CM -lEGL -lm
 configDefs += CONFIG_GFX_OPENGL_ES
 else
 #configDefs += CONFIG_GFX_OPENGL_GLEW_STATIC
 LDLIBS += -lGL -lGLEW
 endif
else ifeq ($(ENV), android)
 LDLIBS += -lGLESv1_CM
 ifneq ($(ARCH), x86)
  LDLIBS += -ldl
 endif
else ifeq ($(ENV), iOS)
 LDLIBS += -framework OpenGLES
else ifeq ($(ENV), macOSX)
 configDefs += CONFIG_GFX_OPENGL_GLEW_STATIC
 LDLIBS += -framework OpenGL -framework CoreVideo
else ifeq ($(ENV), webos)
 LDLIBS += -lGLES_CM
else ifeq ($(ENV), ps3)
 CPPFLAGS += -DPSGL
 LDLIBS += -L/usr/local/cell/target/ppu/lib/PSGL/RSX/ultra-opt -lPSGL  #\
 $(ps3CellPPULibPath)/libstdc++.a $(ps3CellPPULibPath)/libcgc.a $(ps3CellPPULibPath)/libsnc.a \
 $(ps3CellPPULibPath)/libc.a $(ps3CellPPULibPath)/liblv2_stub.a
 # -lPSGLFX -lperf
endif

#ifeq ($(ENV), windows)
#	LDFLAGS += -lopengl32
#endif

SRC += gfx/opengl/opengl.cc

endif
