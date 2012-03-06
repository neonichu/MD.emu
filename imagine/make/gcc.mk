include $(currPath)/common.mk
include $(currPath)/gcc-link.mk
include $(currPath)/gcc-common.mk

gccVersion := $(shell $(CC) -dumpversion)
gccFeatures4_6 := $(shell expr $(gccVersion) \>= 4.6)

ifeq ($(gccFeatures4_6), 1)
 WHOLE_PROGRAM_CFLAGS += #-fipa-pta
endif

ifeq ($(gccFeatures4_6), 1)
 BASE_CXXFLAGS += -std=gnu++0x
 #NORMAL_WARNINGS_CFLAGS += -Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn
endif

ifdef RELEASE
 COMPILE_FLAGS += -fno-ident
 BASE_CXXFLAGS += -fno-enforce-eh-specs
endif

HIGH_OPTIMIZE_CFLAGS += -funsafe-loop-optimizations -Wunsafe-loop-optimizations
