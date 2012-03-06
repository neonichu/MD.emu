ifndef inc_bluetooth_bluez
inc_bluetooth_bluez := 1

include $(IMAGINE_PATH)/make/package/bluez.mk

configDefs += CONFIG_BLUETOOTH

SRC += bluetooth/bluez/bluez.cc

endif
