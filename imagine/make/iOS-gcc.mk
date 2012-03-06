ENV := iOS

ifndef targetDir
 ifdef O_RELEASE
  targetDir := target/iOS/bin-release
 else
  targetDir := target/iOS/bin-debug
 endif
endif

ifeq ($(config_compiler),gcc)
 ifeq ($(origin CC), default)
  CC := llvm-gcc
 endif
 include $(currPath)/gcc.mk
else
 # default to clang
 config_compiler := clang
 ifeq ($(origin CC), default)
  CC := clang
 endif
 include $(currPath)/clang.mk
endif

ifdef RELEASE
 COMPILE_FLAGS += -DNS_BLOCK_ASSERTIONS
endif

 # base engine code needs at least iOS 3.1
minIOSVer := 3.1
IOS_SYSROOT := /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.0.sdk
IOS_FLAGS = -isysroot $(IOS_SYSROOT) -miphoneos-version-min=$(minIOSVer)
CPPFLAGS += $(IOS_FLAGS)
LDFLAGS += $(IOS_FLAGS)
WARNINGS_CFLAGS += -Wno-attributes # for attributes not understood by llvm-gcc

ifneq ($(config_compiler),clang)
 # TODO: find clang equivalent of -fsingle-precision-constant?
 COMPILE_FLAGS += -fsingle-precision-constant -ftree-vectorize
endif
#COMPILE_FLAGS += -ftemplate-depth-100
LDFLAGS += -dead_strip -Wl,-S,-x
WHOLE_PROGRAM_CFLAGS := -fipa-pta -fwhole-program

CPPFLAGS += -I$(IMAGINE_PATH)/bundle/darwin-iOS/include
LDLIBS += -L$(IMAGINE_PATH)/bundle/darwin-iOS/lib
noDoubleFloat=1

allSrc := src/concat/ios.mm
