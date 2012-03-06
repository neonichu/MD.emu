/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#define thisModuleName "btstack"

#include <ifaddrs.h>
#include <arpa/inet.h>
#import <btstack/btstack.h>
#include <logger/interface.h>
#include <util/cLang.h>
#include <bluetooth/Bluetooth.hh>
#include <util/collection/DLList.hh>
#include <util/cLang.h>
#include "../Device.hh"

class BTDevice
{
public:
	bd_addr_t  address;
	//uint32_t   classOfDevice;
	uint16_t   clockOffset;
	uint8_t	   pageScanRepetitionMode;
	//uint8_t    rssi;
	uint8_t    nameState; // 0 empty, 1 found, 2 remote name tried, 3 remote name found

	void requestName()
	{
		if(nameState == 1)
		{
			logMsg("requesting name");
			nameState = 2;
			bt_send_cmd(&hci_remote_name_request, address,
					pageScanRepetitionMode, 0, clockOffset | 0x8000);
		}
	}
};

namespace Base
{
	extern InputDevChangeFunc onInputDevChangeHandler;
	extern void *onInputDevChangeHandlerCtx;
}

namespace Bluetooth
{
	using namespace Base;

	class BtStackInputDevice
	{
	public:
		uint devId, devType;
		virtual bool handleInputData(uchar *data, uint size) = 0; // process data
		virtual void close(bool justRemoveFromList = 0) = 0; // close dev and remove from dev-specific list
	};

	static const uint maxBTInputs = maxGamepadsPerTypeStorage
	+ maxGamepadsPerTypeStorage // for iCP
	+ maxGamepadsPerTypeStorage // for Zeemote
	;

	static BTDevice device[maxBTInputs];
	static uint devices = 0;
	StaticDLList<BtStackInputDevice*, maxGamepadsPerTypeStorage> l2capInputDevList;
	StaticDLList<BtStackInputDevice*, maxGamepadsPerTypeStorage*2> rfcommInputDevList;

	uint maxGamepadsPerType = maxGamepadsPerTypeStorage;
	static bool lookForDeviceNames();

	#ifdef CONFIG_BLUETOOTH_ICP
	struct BtStackIControlPad : public BtStackInputDevice, public IControlPad
	{
		static StaticDLList<BtStackIControlPad*, maxGamepadsPerTypeStorage> list;
		uint16_t ch, handle;
		uint state;
		uchar inputBuffer[6];
		uint inputBufferPos;
		enum { CONNECTED = 0, /*CONTROLS_LED_WAIT, CONTROLS_LED, TURNED_ON_LED_WAIT, TURNED_ON_LED,*/
			CHANGED_LED_MODE_WAIT, CHANGED_LED_MODE, TURNED_ON_REPORTS_WAIT, TURNED_ON_REPORTS };
		static const uint lastSetupStep = CHANGED_LED_MODE;

		void init(uchar *packet, uint player)
		{
			ch = READ_BT_16(packet, 12);
			handle = READ_BT_16(packet, 9);
			devId = player;
			devType = InputEvent::DEV_ICONTROLPAD;
			state = CONNECTED;
			mem_zero(prevBtnData);
			inputBufferPos = 0;
			logMsg("iCP RFCOMM channel opened cid 0x%02X", ch);
		}

		void close(bool justRemoveFromList)
		{
			if(!justRemoveFromList)
				bt_send_cmd(&hci_disconnect, handle, 0x13);
			list.remove(this);
		}

		bool isSetup() { return state == TURNED_ON_REPORTS; }

		bool isWaitingForOk()
		{
			switch(state)
			{
				//case CONTROLS_LED_WAIT:
				//case TURNED_ON_LED_WAIT:
				case CHANGED_LED_MODE_WAIT:
				case TURNED_ON_REPORTS_WAIT:
					return 1;
				default: return 0;
			}
		}

		void notifySetupStepOk()
		{
			switch(state)
			{
				//case CONTROLS_LED_WAIT: state = CONTROLS_LED; break;
				//case TURNED_ON_LED_WAIT: state = TURNED_ON_LED; break;
				case CHANGED_LED_MODE_WAIT: state = CHANGED_LED_MODE; break;
				case TURNED_ON_REPORTS_WAIT: state = TURNED_ON_REPORTS; break;
				default: assert(0);
			}
		}

		void doSetupStep()
		{
			switch(state)
			{
				case CONNECTED:	changeLEDMode(); break;
				//case CONTROLS_LED: turnONLED(); break;
				//case TURNED_ON_LED: changeLEDMode(); break;
				case lastSetupStep: turnONReports(); break;
			}
		}

		/*void controlLED()
		{
			logMsg("turning on LED control");
			assert(state == CONNECTED);
			_bt_rfcomm_send_uih_data(ch, 1, 1, IControlPad::turnOnLEDControl, sizeof IControlPad::turnOnLEDControl);
			state = CONTROLS_LED_WAIT;
		}

		void turnONLED()
		{
			logMsg("turning on LED");
			assert(state == CONTROLS_LED);
			_bt_rfcomm_send_uih_data(ch, 1, 1, IControlPad::turnOnLED, sizeof IControlPad::turnOnLED);
			state = TURNED_ON_LED_WAIT;
		}*/

		void changeLEDMode()
		{
			logMsg("setting LED mode");
			assert(state == CONNECTED);
			bt_send_rfcomm(ch, (uint8_t*)IControlPadDefs::setLEDPulseInverse, sizeof IControlPadDefs::setLEDPulseInverse);
			state = CHANGED_LED_MODE_WAIT;
		}

		void turnONReports()
		{
			logMsg("turning on Reports");
			assert(state == lastSetupStep);
			bt_send_rfcomm(ch, (uint8_t*)IControlPadDefs::turnOnReports, sizeof IControlPadDefs::turnOnReports);
			state = TURNED_ON_REPORTS_WAIT;
		}

		bool handleInputData(uchar *packet, uint size)
		{
			if(isWaitingForOk())
			{
				if(packet[0] == IControlPadDefs::RESP_OKAY)
				{
					logMsg("got OK");
					notifySetupStepOk();
					if(isSetup()) // done our iCP init, look for more devices
					{
						logMsg("done iCP init");
						callSafe(onInputDevChangeHandler, onInputDevChangeHandlerCtx, (InputDevChange){ devId, InputEvent::DEV_ICONTROLPAD, Base::InputDevChange::ADDED });
						lookForDeviceNames();
						return 1;
					}
				}
				else
				{
					logMsg("no OK reply, got size %d, first byte 0x%X", size, packet[0]);
				}
			}

			if(!isSetup() && !isWaitingForOk())
			{
				doSetupStep();
			}
			else
			{
				//logMsg("buffer pos %d", iCP.inputBufferPos);
				iterateTimes(size, i)
				{
					inputBuffer[inputBufferPos++] = packet[i];
					if(inputBufferPos == sizeof inputBuffer)
					{
						inputBufferPos = 0;
						processNubDataForButtonEmulation((schar*)inputBuffer, devId);
						processBtnReport(&inputBuffer[4], devId);
					}
				}
			}
			return 1;
		}
	};
	StaticDLList<BtStackIControlPad*, maxGamepadsPerTypeStorage> BtStackIControlPad::list;

	static BtStackIControlPad *iCPForChannel(uint16_t channel)
	{
		logMsg("looking for iCP with channel %d", channel);
		forEachInDLList(&BtStackIControlPad::list, e)
		{
			if(e->ch == channel )
			{
				logMsg("found %p", e);
				return e;
			}
		}
		logMsg("not found");
		return 0;
	}
	#endif

	struct BtStackZeemote : public BtStackInputDevice, public Zeemote
	{
		static StaticDLList<BtStackZeemote*, maxGamepadsPerTypeStorage> list;
		uint16_t ch, handle;
		uchar inputBuffer[46];
		uint inputBufferPos;
		uint packetSize;

		void init(uchar *packet, uint player)
		{
			ch = READ_BT_16(packet, 12);
			handle = READ_BT_16(packet, 9);
			devId = player;
			devType = InputEvent::DEV_ZEEMOTE;
			mem_zero(inputBuffer);
			inputBufferPos = 0;
			packetSize = 0;
			logMsg("Zeemote RFCOMM channel opened cid 0x%02X", ch);
			callSafe(onInputDevChangeHandler, onInputDevChangeHandlerCtx, (InputDevChange){ player, InputEvent::DEV_ZEEMOTE, Base::InputDevChange::ADDED });
		}

		void close(bool justRemoveFromList)
		{
			if(!justRemoveFromList)
				bt_send_cmd(&hci_disconnect, handle, 0x13);
			list.remove(this);
		}

		bool handleInputData(uchar *packet, uint size)
		{
			iterateTimes(size, i)
			{
				if(inputBufferPos == 0) // get data size
				{
					packetSize = packet[i] + 1;
					logMsg("got packet size %d", packetSize);
					if(packetSize > sizeof(inputBuffer) || packetSize < minPacketSize)
					{
						logErr("can't handle packet, closing Zeemote");
						close(0);
						return 0;
					}
				}

				inputBuffer[inputBufferPos++] = packet[i];
				assert(inputBufferPos <= sizeof(inputBuffer));

				// check if inputBuffer is complete
				if(inputBufferPos == packetSize)
				{
					uint rID = inputBuffer[2];
					logMsg("report id 0x%X, %s", rID, reportIDToStr(rID));
					switch(rID)
					{
						bcase RID_BTN_REPORT:
						{
							const uchar *key = &inputBuffer[3];
							logMsg("got button report %X %X %X %X %X %X", key[0], key[1], key[2], key[3], key[4], key[5]);
							processBtnReport(key, devId);
						}
						bcase RID_8BA_2A_JS_REPORT:
							logMsg("got analog report %d %d", (schar)inputBuffer[4], (schar)inputBuffer[5]);
							processStickDataForButtonEmulation((schar*)&inputBuffer[4], devId);
					}
					inputBufferPos = 0;
				}
			}
			return 1;
		}
	};
	StaticDLList<BtStackZeemote*, maxGamepadsPerTypeStorage> BtStackZeemote::list;

	static BtStackZeemote *zeemoteForChannel(uint16_t channel)
	{
		logMsg("looking for Zeemote with channel %d", channel);
		forEachInDLList(&BtStackZeemote::list, e)
		{
			if(e->ch == channel )
			{
				logMsg("found %p", e);
				return e;
			}
		}
		logMsg("not found");
		return 0;
	}

	static BtStackInputDevice *rfcommDevForChannel(uint16_t ch)
	{
		BtStackInputDevice *dev = iCPForChannel(ch);
		if(dev)
			return dev;
		return zeemoteForChannel(ch);
	}

	static BTDevice *getDeviceForAddress(bd_addr_t addr)
	{
		iterateTimes(devices, i)
		{
			if (BD_ADDR_CMP(addr, device[i].address) == 0)
			{
				return &device[i];
			}
		}
		return 0;
	}

	struct BtStackWiimote : public BtStackInputDevice, public Wiimote
	{
		bd_addr_t addr;
		uint16_t handle;
		uint16_t intCh, ctlCh;
		int extension;
		static StaticDLList<BtStackWiimote*, maxGamepadsPerTypeStorage> list;

		void init(bd_addr_t addr, uint16_t intCh, uint player)
		{
			var_selfs(intCh);
			BD_ADDR_COPY(this->addr, addr);
			devId = player;
			devType = InputEvent::DEV_WIIMOTE;
			extension = 0;
			handle = ctlCh = 0;
		}

		void initCtl(uint16_t ctlCh, uint16_t handle)
		{
			var_selfs(handle);
			var_selfs(ctlCh);
		}

		void close(bool justRemoveFromList)
		{
			if(!justRemoveFromList)
			{
				logMsg("closing Wiimote %d", devId);
				bt_send_cmd(&hci_disconnect, handle, 0x13);
			}
			list.remove(this);
		}

		void sendDataMode30()
		{
			uchar setMode30[] = { 0x52, 0x12, 0x00, 0x30 };
			bt_send_l2cap(ctlCh, setMode30, sizeof(setMode30));
		}

		void sendDataMode32()
		{
			uchar setMode32[] = { 0x52, 0x12, 0x00, 0x32 };
			bt_send_l2cap(ctlCh, setMode32, sizeof(setMode32));
		}

		void requestStatus()
		{
			uchar reqStatus[] = { 0x52, 0x15, 0x00 };
			logMsg("requesting status on ch 0x%02X", ctlCh);
			bt_send_l2cap(ctlCh, reqStatus, sizeof(reqStatus));
		}

		void writeReg(uchar offset, uchar val)
		{
			uchar toWrite[23] = { 0x52, 0x16, 0x04, 0xA4, 0x00, offset, 0x01, val }; // extra 15 bytes padding
			bt_send_l2cap(ctlCh, toWrite, sizeof(toWrite));
		}

		bool handleInputData(uchar *packet, uint size)
		{
			switch(packet[1])
			{
				bcase 0x30:
				{
					logMsg("got core report");
					processCoreButtons(packet, devId);
				}

				bcase 0x32:
				{
					//logMsg("got core + extension report");
					processCoreButtons(packet, devId);
					processClassicButtons(packet, devId);
				}

				bcase 0x20:
				{
					logMsg("got status report, bits 0x%X", packet[4]);
					if(!ctlCh)
					{
						logMsg("skipping report since control channel not yet open");
						break;
					}
					if(extension && !(packet[4] & BIT(1)))
					{
						logMsg("extension disconnected");
						extension = 0;
						sendDataMode30();
					}
					else if(!extension && (packet[4] & BIT(1)))
					{
						logMsg("extension connected, sending init");
						writeReg(0xF0, 0x55); // start init process
						extension = 1;
					}
					else
					{
						logMsg("no extension change, %d", extension);
					}
				}

				bcase 0x22:
				{
					logMsg("got acknowledge output report");
					if(extension == 1)
					{
						logMsg("sending extension init part 2");
						writeReg(0xFB, 0x00); // finish init process
						extension = 2;
					}
					else if(extension == 2)
					{
						logMsg("extension init done, setting report mode");
						memset(prevCCData, 0xFF, sizeof(prevCCData));
						sendDataMode32(); // init done, set data mode
						extension = 3;
					}
				}
			}
			return 1;
		}
	};
	StaticDLList<BtStackWiimote*, maxGamepadsPerTypeStorage> BtStackWiimote::list;

	static BtStackWiimote *wiimoteForIChannel(uint16_t channel)
	{
		forEachInDLList(&BtStackWiimote::list, e)
		{
			//logMsg("testing wiimote %d channel %d to %d", i, wiimote[i].iChannel, channel);
			if(e->intCh == channel || e->ctlCh == channel)
			{
				return e;
			}
		}
		return 0;
	}

	static BtStackWiimote *wiimoteForAddress(bd_addr_t addr)
	{
		forEachInDLList(&BtStackWiimote::list, e)
		{
			if(BD_ADDR_CMP(addr, e->addr) == 0)
			{
				return e;
			}
		}
		return 0;
	}

	uint wiimotes() { return BtStackWiimote::list.size; }
	uint iCPs() { return BtStackIControlPad::list.size; }
	uint zeemotes() { return BtStackZeemote::list.size; }

	bool devsAreConnected()
	{
		return l2capInputDevList.size + rfcommInputDevList.size;
	}

	static BTConnectionEventFunc btConnectedEvent;

	static HCI_STATE bluetoothState;
	uint scanSecs = 4;
	static bool isInScan = 0;
	static uchar pinReply[6] = { 0 };
	static uint pinReplySize = 0;

	static bool lookForDeviceNames()
	{
		//logMsg("looking for dev names");
		iterateTimes(devices, i)
		{
			// retry remote name request
			if (device[i].nameState == 2)
				device[i].nameState = 1;
			if(device[i].nameState == 1)
			{
				//logMsg("device %d", i);
				device[i].requestName();
				return 1;
			}
		}
		//logMsg("none left");

		// check control channels of Wiimotes are valid
		forEachInDLList(&BtStackWiimote::list, e)
		{
			if(!e->ctlCh)
			{
				logWarn("wiimote %d has invalid ctl channel, removing", e->devId);
				l2capInputDevList.remove(e);
				e_it.removeElem();
			}
		}
		isInScan = 0;
		return 0; // no devices left to get names
	}

	static const char *btstackPacketTypeToString(uint8_t type)
	{
		switch(type)
		{
			case L2CAP_DATA_PACKET: return "L2CAP_DATA_PACKET";
			case HCI_EVENT_PACKET: return "HCI_EVENT_PACKET";
		}
		return "Other";
	}

	static void debugPrintL2CAPPacket(uint16_t channel, uint8_t *packet, uint16_t size)
	{
		char dataStr[128];
		snprintf(dataStr, sizeof(dataStr), "L2CAP_DATA_PACKET ch 0x%02X, size %d,", (int)channel, size);
		iterateTimes((uint)IG::min(size, (uint16_t)5), i)
		{
			char byteHex[5];
			snprintf(byteHex, sizeof(byteHex), " %02X", packet[i]);
			strcat(dataStr, byteHex);
		}
		logMsg("%s", dataStr);
	}

	static void sprintBTAddr(char *addrStr, bd_addr_t &addr)
	{
		strcpy(addrStr, "");
		iterateTimes(6, i)
		{
			if(i != 0)
				strcat(addrStr, ":");
			char byteHex[5];
			snprintf(byteHex, sizeof(byteHex), "%02X", addr[i]);
			strcat(addrStr, byteHex);
		}
	}

	static void printAddrs()
	{
		struct ifaddrs *interfaces = NULL;
		if(getifaddrs(&interfaces) == 0)
		{
			struct ifaddrs *iface = interfaces;
			while(iface)
			{
				logMsg("if %s", iface->ifa_name);
				iface = iface->ifa_next;
			}
		}
		freeifaddrs(interfaces);
	}

	struct OpenChCommand
	{
		const hci_cmd_t *cmd;
		bd_addr_t addr;
		uint param;
	};

	static void BTHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
	{
		static uint rfcommDevTypeConnecting = 0;
		static OpenChCommand openAfterAuthenticationChange = { 0 };
		//logMsg("got packet type: %s", btstackPacketTypeToString(packet_type));
		switch (packet_type)
		{
			bcase L2CAP_DATA_PACKET:
			{
				//logMsg("L2CAP_DATA_PACKET ch 0x%02X, size %d", (int)channel, size);
				//debugPrintL2CAPPacket(channel, packet, size);
				if(BtStackWiimote::list.size && packet[0] == 0xa1)
				{
					BtStackWiimote *w = wiimoteForIChannel(channel);
					if(!w)
						break;
					w->handleInputData(packet, size);
					break;
				}
			}

			bcase RFCOMM_DATA_PACKET:
			{
				#ifdef CONFIG_BLUETOOTH_ICP
				//logMsg("RFCOMM_DATA_PACKET ch 0x%02X", (int)channel);
				if(size)
				{
					BtStackInputDevice *i = rfcommDevForChannel(channel);
					if(!i)
						break;
					if(!i->handleInputData(packet, size))
					{
						rfcommInputDevList.remove(i);
						callSafe(onInputDevChangeHandler, onInputDevChangeHandlerCtx, (InputDevChange){ i->devId, i->devType, Base::InputDevChange::REMOVED });
						delete i;
					}
				}
				#endif
			}

			bcase HCI_EVENT_PACKET:
			{
				switch (packet[0])
				{
					bcase BTSTACK_EVENT_STATE:
					{
						logMsg("got BTSTACK_EVENT_STATE");
						// bt stack activated
						bluetoothState = (HCI_STATE)packet[2];

						// set BT state
						if (bluetoothState == HCI_STATE_WORKING)
						{
							//printAddrs();

							logMsg("Starting inquiry");
							devices = 0;
							bt_send_cmd(&hci_inquiry, HCI_INQUIRY_LAP, scanSecs, 0);
							isInScan = 1;
						}
					}

					bcase BTSTACK_EVENT_NR_CONNECTIONS_CHANGED:
					{
						logMsg("got BTSTACK_EVENT_NR_CONNECTIONS_CHANGED");
					}

					bcase BTSTACK_EVENT_REMOTE_NAME_CACHED:
					{
						logMsg("got BTSTACK_EVENT_REMOTE_NAME_CACHED");
					}

					bcase BTSTACK_EVENT_DISCOVERABLE_ENABLED:
					{
						logMsg("got BTSTACK_EVENT_DISCOVERABLE_ENABLED");
					}

					bcase HCI_EVENT_COMMAND_STATUS:
					{
						//logMsg("got HCI_EVENT_COMMAND_STATUS");
					}

					bcase HCI_EVENT_CONNECTION_COMPLETE:
					{
						//logMsg("got HCI_EVENT_CONNECTION_COMPLETE");
					}

					bcase HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS:
					{
						//logMsg("got HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS");
					}

					bcase L2CAP_EVENT_CREDITS:
					{
						//logMsg("got L2CAP_EVENT_CREDITS");
					}

					bcase HCI_EVENT_QOS_SETUP_COMPLETE:
					{
						//logMsg("got HCI_EVENT_QOS_SETUP_COMPLETE");
					}

					bcase HCI_EVENT_PIN_CODE_REQUEST:
					{

						bd_addr_t addr;
						bt_flip_addr(addr, &packet[2]);
						/*char addrStr[64];
						sprintBTAddr(addrStr, addr);*/
						if(pinReplySize)
						{
							logMsg("replying to PIN request");
							bt_send_cmd(&hci_pin_code_request_reply, &addr, pinReplySize, pinReply);
						}
						else
							logMsg("got HCI_EVENT_PIN_CODE_REQUEST");
					}

					bcase BTSTACK_EVENT_POWERON_FAILED:
					{
						bluetoothState = HCI_STATE_OFF;
						logMsg("Bluetooth not accessible! Make sure you have turned off Bluetooth in the System Settings.");
						callSafe(btConnectedEvent, MSG_NO_PERMISSION);
					}

					bcase HCI_EVENT_INQUIRY_RESULT:
					case HCI_EVENT_INQUIRY_RESULT_WITH_RSSI:
					{
						uint numResponses = packet[2];
						logMsg("got HCI_EVENT_INQUIRY_RESULT, %d responses", numResponses);
						iterateTimes(numResponses, i)
						{
							bd_addr_t addr;
							bt_flip_addr(addr, &packet[3+i*6]);

							BTDevice *d = getDeviceForAddress(addr);
							if(d) continue; // device already in list
							uchar *devClass = &packet[3 + numResponses*(6+1+1+1) + i*3];
							if(!testSupportedBTDevClasses(devClass))
							{
								logMsg("skipping device due to class %X:%X:%X", devClass[0], devClass[1], devClass[2]);
								continue;
							}
							if(devices == sizeofArray(device))
							{
								logMsg("max devices reached");
								break;
							}
							logMsg("new device @ idx %d, COD: %X %X %X", devices, devClass[0], devClass[1], devClass[2]);
							d = &device[devices];
							BD_ADDR_COPY(d->address, addr);
							d->pageScanRepetitionMode = packet[3 + numResponses*(6) + i*1];
							//d->classOfDevice = READ_BT_24(packet, 3 + numResponses*(6+1+1+1) + i*3);
							d->clockOffset = READ_BT_16(packet, 3 + numResponses*(6+1+1+1+3) + i*2) & 0x7fff;
							//d->rssi = 0;
							d->nameState = 1;
							logMsg("Device found with pageScan %u, clock offset 0x%04x", d->pageScanRepetitionMode,  d->clockOffset);
							devices++;
						}
					}

					bcase HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE:
					{
						//logMsg("got HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE ch %d", (int)channel);
						bd_addr_t addr;
						bt_flip_addr(addr, &packet[3]);
						BTDevice *d = getDeviceForAddress(addr);
						if (!d) break;

						char addrStr[64];
						sprintBTAddr(addrStr, addr);

						if (packet[2] == 0)
						{
							// assert max length
							packet[9+255] = 0;

							char* name = (char*)&packet[9];
							logMsg("Name: '%s', Addr: %s", name, addrStr);
							d->nameState = 3;
							if(string_equal(name, WiimoteDefs::btNamePrefix))
							{
								logMsg("found Wiimote");
								if(BtStackWiimote::list.size == (int)maxGamepadsPerType)
								{
									logMsg("max wiimotes reached");
									break;
								}
								pinReplySize = 0;
								bt_send_cmd(&hci_write_authentication_enable, 0);
								/*iterateTimes(6, i)
								{
									pinReply[i] = addr[5-i];
								}
								pinReplySize = 6;
								logMsg("using pin %X %X %X %X %X %X", (uint)pinReply[0], (uint)pinReply[1], (uint)pinReply[2],
										(uint)pinReply[3], (uint)pinReply[4], (uint)pinReply[5]);
								bt_send_cmd(&hci_write_authentication_enable, 1);*/
								openAfterAuthenticationChange = (OpenChCommand){ &l2cap_create_channel, { 0 }, 0x13 };
								BD_ADDR_COPY(openAfterAuthenticationChange.addr, d->address);
								//bt_send_cmd(&l2cap_create_channel, d->address, 0x13);
								break; // open the wiimote channels, will continue name requests after
							}
							#ifdef CONFIG_BLUETOOTH_ICP
							else if(strstr(name, IControlPadDefs::btNamePrefix))
							{
								logMsg("found iCP");
								if(BtStackIControlPad::list.size == (int)maxGamepadsPerType)
								{
									logMsg("max iCPs reached");
									break;
								}
								strcpy((char*)pinReply, "1234");
								pinReplySize = 4;
								rfcommDevTypeConnecting = InputEvent::DEV_ICONTROLPAD;
								bt_send_cmd(&hci_write_authentication_enable, 1);
								openAfterAuthenticationChange = (OpenChCommand){ &rfcomm_create_channel, { 0 }, 1 };
								BD_ADDR_COPY(openAfterAuthenticationChange.addr, d->address);
								//bt_send_cmd(&rfcomm_create_channel, d->address, 1);
								break;
							}
							#endif
							else if(strstr(name, ZeemoteDefs::btName))
							{
								logMsg("found Zeemote");
								if(BtStackZeemote::list.size == (int)maxGamepadsPerType)
								{
									logMsg("max Zeemotes reached");
									break;
								}
								strcpy((char*)pinReply, "0000");
								pinReplySize = 4;
								rfcommDevTypeConnecting = InputEvent::DEV_ZEEMOTE;
								bt_send_cmd(&hci_write_authentication_enable, 1);
								openAfterAuthenticationChange = (OpenChCommand){ &rfcomm_create_channel, { 0 }, 1 };
								BD_ADDR_COPY(openAfterAuthenticationChange.addr, d->address);
								//bt_send_cmd(&rfcomm_create_channel, d->address, 1);
								break;
							}
							else
							{
								callSafe(btConnectedEvent, MSG_UNKNOWN_DEV);
							}
						}
						else
						{
							callSafe(btConnectedEvent, MSG_ERR_NAME);
							d->nameState = 3;
							logMsg("Failed to get name: page timeout");
						}
						lookForDeviceNames();
					}

					bcase HCI_EVENT_LINK_KEY_NOTIFICATION:
					{
						logMsg("got HCI_EVENT_LINK_KEY_NOTIFICATION");
					}

					bcase HCI_EVENT_LINK_KEY_REQUEST:
					{
						logMsg("got HCI_EVENT_LINK_KEY_REQUEST");
						bd_addr_t addr;
						bt_flip_addr(addr, &packet[2]);
						bt_send_cmd(&hci_link_key_request_negative_reply, &addr);
					}

					bcase L2CAP_EVENT_TIMEOUT_CHECK:
					{
						//logMsg("got L2CAP_EVENT_TIMEOUT_CHECK");
					}

					bcase HCI_EVENT_ENCRYPTION_CHANGE:
					{
						logMsg("got HCI_EVENT_ENCRYPTION_CHANGE");
					}

					bcase HCI_EVENT_MAX_SLOTS_CHANGED:
					{
						logMsg("got HCI_EVENT_MAX_SLOTS_CHANGED");
					}

					bcase HCI_EVENT_COMMAND_COMPLETE:
					{
						if (COMMAND_COMPLETE_EVENT(packet, hci_inquiry_cancel))
						{
							logMsg("inquiry canceled");
						}
						else if (COMMAND_COMPLETE_EVENT(packet, hci_remote_name_request_cancel))
						{
							logMsg("remote name request canceled");
						}
						else if(COMMAND_COMPLETE_EVENT(packet, hci_write_authentication_enable))
						{
							logMsg("write authentication changed");
							if(openAfterAuthenticationChange.cmd)
							{
								logMsg("opening channel with parameter %d", openAfterAuthenticationChange.param);
								bt_send_cmd(openAfterAuthenticationChange.cmd, openAfterAuthenticationChange.addr, openAfterAuthenticationChange.param);
								openAfterAuthenticationChange.cmd = 0;
							}
						}
						else
							logMsg("got HCI_EVENT_COMMAND_COMPLETE");
					}

					bcase HCI_EVENT_INQUIRY_COMPLETE:
					{
						logMsg("got HCI_EVENT_INQUIRY_COMPLETE");
						if(devices)
							lookForDeviceNames();
						else
						{
							isInScan = 0;
							callSafe(btConnectedEvent, MSG_NO_DEVS);
						}
					}

					bcase RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE:
					{
						{
							if(rfcommDevTypeConnecting == InputEvent::DEV_ICONTROLPAD)
							{
								BtStackIControlPad *dev = new BtStackIControlPad;
								dev->init(packet, BtStackIControlPad::list.size);
								dev->doSetupStep();
								BtStackIControlPad::list.add(dev);
								rfcommInputDevList.add(dev);
							}
							else
							{
								BtStackZeemote *dev = new BtStackZeemote;
								dev->init(packet, BtStackZeemote::list.size);
								BtStackZeemote::list.add(dev);
								rfcommInputDevList.add(dev);
							}
							lookForDeviceNames();
						}
					}

					bcase L2CAP_EVENT_CHANNEL_OPENED:
					{
						logMsg("got L2CAP_EVENT_CHANNEL_OPENED ch 0x%02X", (int)channel);
						if (packet[2] == 0)
						{
							bd_addr_t addr;
							bt_flip_addr(addr, &packet[3]);
							uint16_t psm = READ_BT_16(packet, 11);
							uint16_t sourceCid = READ_BT_16(packet, 13);
							uint16_t destCid = READ_BT_16(packet, 15);
							uint16_t handle = READ_BT_16(packet, 9);
							if (psm == 0x13)
							{
								logMsg("Interrupt channel 0x%02X opened, dest cid 0x%02X, opening control channel",
										sourceCid, destCid);
								BtStackWiimote *dev = new BtStackWiimote;
								dev->init(addr, sourceCid, BtStackWiimote::list.size);
								BtStackWiimote::list.add(dev);
								l2capInputDevList.add(dev);
								bt_send_cmd(&l2cap_create_channel, addr, 0x11);
							}
							else if(psm == 0x11)
							{
								BtStackWiimote *dev = wiimoteForAddress(addr);
								if(!dev)
									break;
								dev->initCtl(sourceCid, handle);

								logMsg("Control channel 0x%02X opened, dest cid 0x%02X",
										sourceCid, destCid);

								uint8_t setLEDs[] = { 0x52, 0x11, Wiimote::playerLeds(dev->devId) };
								bt_send_l2cap( sourceCid, setLEDs, sizeof(setLEDs));
								dev->requestStatus();
								lookForDeviceNames();
								logMsg("wiimote %d finished setup", dev->devId);
								callSafe(onInputDevChangeHandler, onInputDevChangeHandlerCtx, (InputDevChange){ dev->devId, InputEvent::DEV_WIIMOTE, Base::InputDevChange::ADDED });
							}
						}
						else
						{
							callSafe(btConnectedEvent, MSG_ERR_CHANNEL);
							logMsg("failed. status code %u\n", packet[2]);
							lookForDeviceNames();
						}
					}

					bcase RFCOMM_EVENT_CHANNEL_CLOSED:
					{
						int rfcommCh = READ_BT_16(packet, 2);
						logMsg("got RFCOMM_EVENT_CHANNEL_CLOSED for 0x%02X", rfcommCh);
						BtStackInputDevice *dev = rfcommDevForChannel(rfcommCh);
						if(!dev)
							break;
						rfcommInputDevList.remove(dev);
						dev->close(1);
						callSafe(onInputDevChangeHandler, onInputDevChangeHandlerCtx, (InputDevChange){ dev->devId, dev->devType, Base::InputDevChange::REMOVED });
						delete dev;
					}

					bcase L2CAP_EVENT_CHANNEL_CLOSED:
					{
						logMsg("got L2CAP_EVENT_CHANNEL_CLOSED for 0x%02X", channel);

						BtStackWiimote *dev = wiimoteForIChannel(channel);
						if(!dev)
							break;
						l2capInputDevList.remove(dev);
						BtStackWiimote::list.remove(dev);
						callSafe(onInputDevChangeHandler, onInputDevChangeHandlerCtx, (InputDevChange){ dev->devId, InputEvent::DEV_WIIMOTE, Base::InputDevChange::REMOVED });
						delete dev;
					}

					bdefault:
						logMsg("unhandled HCI event type 0x%X", packet[0]);
					break;
				}
			}

			bdefault:
				logMsg("unhandled packet type 0x%X", packet_type);
				break;
		}
		//logMsg("end packet");
	}

	static bool btOpened = 0;

	static void disconnectDevs()
	{
		forEachInDLList(&l2capInputDevList, e)
		{
			e->close();
			e_it.removeElem();
			callSafe(onInputDevChangeHandler, onInputDevChangeHandlerCtx, (InputDevChange){ e->devId, e->devType, Base::InputDevChange::REMOVED });
			delete e;
		}
		forEachInDLList(&rfcommInputDevList, e)
		{
			e->close();
			e_it.removeElem();
			callSafe(onInputDevChangeHandler, onInputDevChangeHandlerCtx, (InputDevChange){ e->devId, e->devType, Base::InputDevChange::REMOVED });
			delete e;
		}
		assert(BtStackWiimote::list.size == 0);
		assert(BtStackIControlPad::list.size == 0);
		assert(BtStackZeemote::list.size == 0);
		assert(l2capInputDevList.size == 0);
		assert(rfcommInputDevList.size == 0);
	}

	void closeBT()
	{
		if(!btOpened /*bluetoothState == HCI_STATE_OFF*/)
		{
			logMsg("BTStack already off");
			return;
		}
		logMsg("BTStack shutdown");
		disconnectDevs();
		//bt_send_cmd(&btstack_set_power_mode, HCI_POWER_OFF/*HCI_POWER_SLEEP*/);
		bt_close();
		isInScan = 0;
		mem_zero(pinReply);
		pinReplySize = 0;
		btOpened = 0;
	}

	CallResult initBT()
	{
		if(btOpened)
			return OK;

		static bool runLoopInit = 0;
		if(!runLoopInit)
		{
			run_loop_init(RUN_LOOP_COCOA);
			runLoopInit = 1;
		}

		BtStackIControlPad::list.init();
		BtStackZeemote::list.init();
		BtStackWiimote::list.init();
		rfcommInputDevList.init();
		l2capInputDevList.init();
		bluetoothState = HCI_STATE_OFF;

		if(bt_open())
		{
			logWarn("Failed to open connection to BTdaemon");
			return INVALID_PARAMETER;
		}
		bt_register_packet_handler(BTHandler);
		btOpened = 1;
		logMsg("BTStack init");
		return OK;
	}

	bool startBT(BTConnectionEventFunc connectFunc)
	{
		/*if(btOpened /*bluetoothState != HCI_STATE_OFF*//*)// && bluetoothState != HCI_STATE_SLEEPING)
		{
			closeBT();
		}*/
		assert(btOpened);
		btConnectedEvent = connectFunc;
		if(isInScan)
			return 0;
		disconnectDevs();
		if(bluetoothState == HCI_STATE_WORKING)
		{
			logMsg("HCI is on, Starting inquiry");
			devices = 0;
			bt_send_cmd(&hci_inquiry, HCI_INQUIRY_LAP, scanSecs, 0);
			isInScan = 1;
		}
		else
		{
			logMsg("BTStack power on");
			bt_send_cmd(&btstack_set_power_mode, HCI_POWER_ON);
		}
		return 1;
	}
};

#undef thisModuleName
