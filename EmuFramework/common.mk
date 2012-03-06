ifeq ($(ENV), iOS)
configDefs += CONFIG_BASE_IOS_ICADE
endif

include $(imagineSrcDir)/audio/system.mk
include $(imagineSrcDir)/input/system.mk
include $(imagineSrcDir)/gfx/system.mk
include $(imagineSrcDir)/fs/system.mk
include $(imagineSrcDir)/io/system.mk
include $(imagineSrcDir)/bluetooth/system.mk
ifeq ($(ENV), android)
include $(imagineSrcDir)/io/zip/build.mk
endif
include $(imagineSrcDir)/gui/GuiTable1D/build.mk
include $(imagineSrcDir)/gui/FSPicker/build.mk
include $(imagineSrcDir)/resource2/font/freetype/build.mk
include $(imagineSrcDir)/resource2/image/png/build.mk

ifeq ($(ENV), android)
 ifneq ($(ARCH), x86)
  configDefs += SUPPORT_ANDROID_DIRECT_TEXTURE
 endif
endif

CPPFLAGS += -I../EmuFramework/include
VPATH += ../EmuFramework/src
SRC += CreditsView.cc MsgPopup.cc FilePicker.cc EmuSystem.cc Recent.cc \
AlertView.cc Screenshot.cc ButtonConfigView.cc

ifneq ($(ENV), ps3)
SRC += VController.cc
endif