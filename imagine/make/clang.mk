include $(currPath)/common.mk
include $(currPath)/gcc-link.mk
include $(currPath)/gcc-common.mk

BASE_CXXFLAGS += -std=gnu++0x
WARNINGS_CFLAGS += -Wno-unused-parameter -Wno-attributes -Wno-delete-non-virtual-dtor
