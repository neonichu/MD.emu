ifndef inc_data_type_font_freetype_2
inc_data_type_font_freetype_2 := 1

include $(imagineSrcDir)/io/system.mk

ifdef package_freetype_externalPath
	CPPFLAGS +=  -I$(package_freetype_externalPath)/include
	LDLIBS += -L$(package_freetype_externalPath)/lib
else
	ifeq ($(ENV), macOSX)
		CPPFLAGS +=  -I/opt/local/include/freetype2
	else
		CPPFLAGS +=  -I/usr/include/freetype2
	endif
endif

LDLIBS += -lfreetype

configDefs += CONFIG_DATA_TYPE_FONT_FREETYPE_2

SRC += data-type/font/freetype-2/reader.cc

endif