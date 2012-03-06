ifndef inc_bluetooth_btstack
inc_bluetooth_btstack := 1

ifndef iOSAppStore

include $(IMAGINE_PATH)/make/package/btstack.mk

configDefs += CONFIG_BLUETOOTH

SRC += bluetooth/btstack/btstack.cc

endif

endif
