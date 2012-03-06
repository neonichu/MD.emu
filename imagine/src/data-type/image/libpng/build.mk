ifndef inc_data_type_image_libpng
inc_data_type_image_libpng := 1

ifdef package_libpng_externalPath
	CPPFLAGS +=  -I$(package_libpng_externalPath)/include
	LDLIBS += -L$(package_libpng_externalPath)/lib
	
	ifdef package_zlib_externalPath
		CPPFLAGS +=  -I$(package_zlib_externalPath)/include
		LDLIBS += -L$(package_zlib_externalPath)/lib
	endif
	
	LDLIBS +=  -lpng14 -lz
else
	ifneq ($(ENV), linux)
		CPPFLAGS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_INCLUDE_PATH=$(system_externalSysroot)/include pkg-config libpng --cflags --static --define-variable=prefix=$(system_externalSysroot))
		LDLIBS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_LIBRARY_PATH=$(system_externalSysroot)/lib pkg-config libpng --libs --static --define-variable=prefix=$(system_externalSysroot))
	else
		CPPFLAGS += $(shell pkg-config libpng --cflags)
		LDLIBS += $(shell pkg-config libpng --libs)
	endif
	#ifneq ($(ENV), linux)
	#	LDLIBS +=  -lpng14 -lz
	#else
	#	CPPFLAGS +=  -I/usr/include/libpng14
	#	LDLIBS +=  -lpng14
	#endif
endif

configDefs += CONFIG_DATA_TYPE_IMAGE_LIBPNG

SRC += data-type/image/libpng/reader.cc

endif
