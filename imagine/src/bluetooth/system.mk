ifeq ($(ENV), linux)
 include $(imagineSrcDir)/bluetooth/bluez/build.mk
else ifeq ($(ENV), android)
 ifneq ($(ARCH), x86)
  include $(imagineSrcDir)/bluetooth/bluez/build.mk
 endif
else ifeq ($(ENV), iOS)
 include $(imagineSrcDir)/bluetooth/btstack/build.mk
endif
