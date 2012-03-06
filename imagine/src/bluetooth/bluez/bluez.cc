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

#define thisModuleName "bluez"

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <util/thread/pthread.hh>
#include <util/fd-utils.h>
#include <util/number.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <bluetooth/Bluetooth.hh>
#include <base/interface.h>
#include <util/collection/DLList.hh>
#include "../Device.hh"

#if defined(CONFIG_BASE_ANDROID) && CONFIG_ENV_ANDROID_MINSDK < 9

namespace Base
{
CallResult android_attachInputThreadToJVM();
void android_detachInputThreadToJVM();
}

#endif

#ifdef CONFIG_BASE_ANDROID

extern bool bluez_loaded;
CallResult bluez_dl();

#endif

namespace Base
{
	extern InputDevChangeFunc onInputDevChangeHandler;
	extern void *onInputDevChangeHandlerCtx;
}

#define CONFIG_BLUEZ_ZEEMOTE

namespace Bluetooth
{

class BluezInputDevice : public Base::PollHandler
{
public:
	uint devId, devType;
	virtual void close() = 0; // close dev and remove from dev-specific list
};

static void closeDeviceCommon(BluezInputDevice *dev);

struct BluezZeemote : public BluezInputDevice, public Zeemote
{
public:
	int sock;
	bdaddr_t bdaddr;
	static StaticDLList<BluezZeemote*, maxGamepadsPerTypeStorage> list;
	uchar inputBuffer[46];
	uint inputBufferPos;
	uint packetSize;

	CallResult setup(int ePoll, uint i)
	{
		using namespace ZeemoteDefs;
		logMsg("connecting to Zeemote");
		struct sockaddr_rc addr;
		mem_zero(addr);
		addr.rc_family = AF_BLUETOOTH;
		addr.rc_channel = (uint8_t)1;
		addr.rc_bdaddr = bdaddr;
		sock = ::socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
		if(sock == -1)
		{
			logMsg("error creating socket");
			return INVALID_PARAMETER;
		}
		if(connect(sock, (struct sockaddr *)&addr, sizeof addr) == -1)
		{
			logMsg("error connecting socket");
			return INVALID_PARAMETER;
		}

		// try to read first packet

		int len = read(sock, inputBuffer, 1);
		if(len == -1)
		{
			logMsg("error reading first packet");
			::close(sock);
			return IO_ERROR;
		}
		packetSize = inputBuffer[0] + 1;
		inputBufferPos = 1;
		logMsg("first packet size %d", packetSize);

		Base::sendInputDevChangeMessageToMain(i, InputEvent::DEV_ZEEMOTE, Base::InputDevChange::ADDED);

		PollHandler::func = fdDataHandler;
		PollHandler::data = this;
		#ifdef CONFIG_BASE_HAS_FD_EVENTS
			Base::addPollEvent2(sock, *this);
		#else
			struct epoll_event ev = { 0 };
			ev.data.ptr = static_cast<Base::PollHandler*>(this);
			ev.events = EPOLLIN;
			assert(ePoll);
			epoll_ctl(ePoll, EPOLL_CTL_ADD, sock, &ev);
		#endif

		devId = i;
		devType = InputEvent::DEV_ZEEMOTE;
		return OK;
	}

	void close()
	{
		if(sock > 0)
		{
			if(Base::hasFDEvents)
				Base::removePollEvent(sock);
			if(::close(sock) != 0)
				logMsg("error closing socket");
		}
		list.remove(this);
	}

	static base_pollHandlerFuncProto(fdDataHandler)
	{
		return static_cast<BluezZeemote*>(data)->handleAvailableInputData();
	}

	bool handleAvailableInputData()
	{
		using namespace ZeemoteDefs;
		int bytesToRead = fd_bytesReadable(sock);
		//logMsg("%d bytes ready", bytesToRead);
		do
		{
			int len = read(sock, &inputBuffer[inputBufferPos], IG::min((uint)bytesToRead, packetSize - inputBufferPos));
			if(unlikely(len <= 0))
			{
				logMsg("error reading packet, closing Zeemote");
				close();
				closeDeviceCommon(this);
				return 0;
			}
			if(inputBufferPos == 0) // get data size
			{
				packetSize = inputBuffer[0] + 1;
				//logMsg("got packet size %d", packetSize);
			}
			//logDMsg("read %d bytes from Zeemote", len);

			if(packetSize > sizeof(inputBuffer) || packetSize < minPacketSize)
			{
				logErr("can't handle packet, closing Zeemote");
				close();
				closeDeviceCommon(this);
				return 0;
			}

			inputBufferPos += len;
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
						processBtnReport(key, devId, !Base::hasFDEvents);
					}
					bcase RID_8BA_2A_JS_REPORT:
						logMsg("got analog report %d %d", (schar)inputBuffer[4], (schar)inputBuffer[5]);
						processStickDataForButtonEmulation((schar*)&inputBuffer[4], devId, !Base::hasFDEvents);
				}
				inputBufferPos = 0;
			}
			bytesToRead -= len;
		} while(bytesToRead > 0);

		return 1;
	}
};

StaticDLList<BluezZeemote*, maxGamepadsPerTypeStorage> BluezZeemote::list;

struct BluezIControlPad : public BluezInputDevice, public IControlPad
{
private:
	CallResult getOKReply()
	{
		uchar ok;
		if(read(sock, &ok, 1) != 1)
		{
			logErr("read error");
			::close(sock);
			return IO_ERROR;
		}
		if(ok != IControlPadDefs::RESP_OKAY)
		{
			logErr("error: iCP didn't respond with OK");
			::close(sock);
			return INVALID_PARAMETER;
		}
		logMsg("got OK reply");
		return OK;
	}
	int sock;
public:
	bdaddr_t bdaddr;
	uchar inputBuffer[6];
	uint inputBufferPos;
	static StaticDLList<BluezIControlPad*, maxGamepadsPerTypeStorage> list;

	CallResult setup(int ePoll, uint i)
	{
		using namespace IControlPadDefs;
		logMsg("connecting to iCP");
		struct sockaddr_rc addr;
		mem_zero(addr);
		addr.rc_family = AF_BLUETOOTH;
		addr.rc_channel = (uint8_t)1;
		addr.rc_bdaddr = bdaddr;
		sock = ::socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
		if(sock == -1)
		{
			logMsg("error creating socket");
			return INVALID_PARAMETER;
		}
		if(connect(sock, (struct sockaddr *)&addr, sizeof addr) == -1)
		{
			logMsg("error connecting socket");
			return INVALID_PARAMETER;
		}
		logMsg("connected socket %d", sock);
		/*const uchar CMD_GET_BATT_LEVEL = 0x55;
		write(sock, &CMD_GET_BATT_LEVEL, 1);
		uchar lvl = 0;
		read(sock, &lvl, 1);
		logMsg("battery lvl %d", (int)lvl);*/

		/*const uchar CMD_GET_DIGITALS = 0xA5;
		write(iCP.sock, &CMD_GET_DIGITALS, 1);
		uchar pad1;
		read(iCP.sock, &pad1, 1);
		logMsg("read byte %d", pad1);
		uchar pad2;
		read(iCP.sock, &pad2, 1);
		logMsg("read byte %d", pad2);*/

		/*if(write(sock, IControlPad::turnOnLEDControl, sizeof IControlPad::turnOnLEDControl) != sizeof IControlPad::turnOnLEDControl)
		{
			logErr("error writing FORCE_LED_CTRL");
			return IO_ERROR;
		}
		doOrReturn(getOKReply());

		if(write(sock, IControlPad::turnOnLED, sizeof IControlPad::turnOnLED) != sizeof IControlPad::turnOnLED)
		{
			logErr("error writing SET_LED");
			return IO_ERROR;
		}
		doOrReturn(getOKReply());*/

		if(fd_writeAll(sock, setLEDPulseInverse, sizeof setLEDPulseInverse) != sizeof setLEDPulseInverse)
		{
			logErr("error writing SET_LED_MODE");
			::close(sock);
			return IO_ERROR;
		}
		doOrReturn(getOKReply());

		if(fd_writeAll(sock, turnOnReports, sizeof turnOnReports) != sizeof turnOnReports)
		{
			logErr("error writing GP_REPORTS");
			::close(sock);
			return IO_ERROR;
		}
		doOrReturn(getOKReply());

		/*iterateTimes(200, i)
		{
			uchar packet[6];
			int len = read(sock, packet, sizeof packet);
			printf("got read: ");
			printMem(packet, len);
		}*/

		inputBufferPos = 0;
		devId = i;
		devType = InputEvent::DEV_ICONTROLPAD;
		Base::sendInputDevChangeMessageToMain(i, InputEvent::DEV_ICONTROLPAD, Base::InputDevChange::ADDED);

		PollHandler::func = fdDataHandler;
		PollHandler::data = this;
		#ifdef CONFIG_BASE_HAS_FD_EVENTS
			Base::addPollEvent2(sock, *this);
		#else
			struct epoll_event ev = { 0 };
			ev.data.ptr = static_cast<Base::PollHandler*>(this);
			ev.events = EPOLLIN;
			assert(ePoll);
			epoll_ctl(ePoll, EPOLL_CTL_ADD, sock, &ev);
		#endif

		return OK;
	}

	void close()
	{
		if(sock > 0)
		{
			if(Base::hasFDEvents)
				Base::removePollEvent(sock);
			/*if(write(sock, shutdown, sizeof shutdown) != sizeof shutdown)
				logMsg("error writing shutdown command");*/
			if(::close(sock) != 0)
				logMsg("error closing socket");
		}
		list.remove(this);
	}

	static base_pollHandlerFuncProto(fdDataHandler)
	{
		return static_cast<BluezIControlPad*>(data)->handleAvailableInputData();
	}

	bool handleAvailableInputData()
	{
		int len;
		//logMsg("reading socket %d", sock);
		int bytesToRead = fd_bytesReadable(sock);
		//logMsg("%d bytes ready", bytesToRead);
		do
		{
			int len = read(sock, &inputBuffer[inputBufferPos], IG::min((uint)bytesToRead, uint(sizeof inputBuffer) - inputBufferPos));
			if(unlikely(len <= 0))
			{
				logMsg("error %d reading packet, closing iCP", len == -1 ? errno : 0);
				close();
				closeDeviceCommon(this);
				return 0;
			}
			//logDMsg("read %d bytes from iCP", len);
			inputBufferPos += len;
			assert(inputBufferPos <= 6);

			// check if inputBuffer is complete
			if(inputBufferPos == 6)
			{
				processNubDataForButtonEmulation((schar*)inputBuffer, devId, !Base::hasFDEvents);
				processBtnReport(&inputBuffer[4], devId, !Base::hasFDEvents);
				inputBufferPos = 0;
			}
			bytesToRead -= len;
		} while(bytesToRead > 0);

		//logDMsg("done reading iCP");
		return 1;
	}
};

StaticDLList<BluezIControlPad*, maxGamepadsPerTypeStorage> BluezIControlPad::list;

struct BluezWiimote : public BluezInputDevice, public Wiimote
{
	bdaddr_t addr;
	int ctlSocket, intSocket;
	int extension;
	static StaticDLList<BluezWiimote*, maxGamepadsPerTypeStorage> list;

	CallResult setup(int ePoll, uint i)
	{
		extension = 0;
		ushort ctlPsm = 17, intPsm = 19;
		struct sockaddr_l2 addr;
		mem_zero(addr);
		addr.l2_family = AF_BLUETOOTH;
		addr.l2_psm = htobs(ctlPsm);
		addr.l2_bdaddr = this->addr;
		ctlSocket = ::socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
		if(ctlSocket == -1)
		{
			logMsg("error creating control socket");
			return IO_ERROR;
		}
		if(connect(ctlSocket, (struct sockaddr *)&addr, sizeof addr) == -1)
		{
			logMsg("error connecting control socket");
			return IO_ERROR;
		}

		addr.l2_psm = htobs(intPsm);
		intSocket = ::socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
		if(intSocket == -1)
		{
			logMsg("error creating interrupt socket");
			return IO_ERROR;
		}
		if(connect(intSocket, (struct sockaddr *)&addr, sizeof addr) == -1)
		{
			logMsg("error connecting interrupt socket");
			return IO_ERROR;
		}

		// using 0xa2 instead of 0x52 for Wii Remote Plus
		uchar setLEDs[] = { 0xa2, 0x11, playerLeds(i) };
		int ret = fd_writeAll(intSocket, setLEDs, sizeof(setLEDs));
		requestStatus();

		PollHandler::func = fdDataHandler;
		PollHandler::data = this;
		#ifdef CONFIG_BASE_HAS_FD_EVENTS
			Base::addPollEvent2(intSocket, *this);
		#else
			struct epoll_event ev = { 0 };
			ev.data.ptr = static_cast<Base::PollHandler*>(this);
			ev.events = EPOLLIN;
			epoll_ctl(ePoll, EPOLL_CTL_ADD, intSocket, &ev);
		#endif

		devId = i;
		devType = InputEvent::DEV_WIIMOTE;
		Base::sendInputDevChangeMessageToMain(i, InputEvent::DEV_WIIMOTE, Base::InputDevChange::ADDED);
		return OK;
	}

	void sendDataMode30()
	{
		logMsg("setting mode 30");
		uchar setMode30[] = { 0xa2, 0x12, 0x00, 0x30 };
		int ret = fd_writeAll(intSocket, setMode30, sizeof(setMode30));
	}

	void sendDataMode32()
	{
		logMsg("setting mode 32");
		uchar setMode32[] = { 0xa2, 0x12, 0x00, 0x32 };
		int ret = fd_writeAll(intSocket, setMode32, sizeof(setMode32));
	}

	void requestStatus()
	{
		logMsg("requesting status");
		uchar reqStatus[] = { 0xa2, 0x15, 0x00 };
		int ret = fd_writeAll(intSocket, reqStatus, sizeof(reqStatus));
	}

	void writeReg(uchar offset, uchar val)
	{
		uchar toWrite[23] = { 0xa2, 0x16, 0x04, 0xA4, 0x00, offset, 0x01, val }; // extra 15 bytes padding
		int ret = fd_writeAll(intSocket, toWrite, sizeof(toWrite));
	}

	void readReg(uchar offset, uchar size)
	{
		uchar toRead[] = { 0xa2, 0x17, 0x04, 0xA4, 0x00, offset, 0x00, size }; // extra 15 bytes padding
		int ret = fd_writeAll(intSocket, toRead, sizeof(toRead));
	}

	void initExtension()
	{
		writeReg(0xF0, 0x55);
		uchar packet[23];
		iterateTimes(4, i)
		{
			// skip stray button reports
			int ret = read(intSocket, &packet, sizeof(packet));
			logMsg("got reply %d, %d bytes", packet[1], ret);
			if(packet[1] == 0x22)
				break;
		}
		writeReg(0xFB, 0x00);
		int ret = read(intSocket, &packet, sizeof(packet));
		logMsg("got reply %d, %d bytes", packet[1], ret);
		//readReg(0xFE, 2);
	}

	void processPacket(const uchar *packet, uint player)
	{
		if(unlikely(packet[0] != 0xa1))
		{
			logWarn("Unknown header in Wiimote packet");
			return;
		}
		switch(packet[1])
		{
			bcase 0x30:
			{
				//logMsg("got core report");
				processCoreButtons(packet, player, !Base::hasFDEvents);
				//logMsg("input handler finished");
			}

			bcase 0x32:
			{
				//logMsg("got core+extension report");
				processCoreButtons(packet, player, !Base::hasFDEvents);
				processClassicButtons(packet, player, !Base::hasFDEvents);
			}

			bcase 0x34:
			{
				//logMsg("got core+extension19 report");
				//processCoreButtons(packet, player, !Base::hasFDEvents);
			}

			bcase 0x20:
			{
				logMsg("got status report, bits 0x%X", packet[4]);
				if(extension && !(packet[4] & BIT(1)))
				{
					logMsg("extension disconnected");
					extension = 0;
				}
				else if(!extension && (packet[4] & BIT(1)))
				{
					logMsg("extension connected");
					initExtension();
					extension = 1;
					memset(prevCCData, 0xFF, sizeof(prevCCData));
				}
				else
				{
					logMsg("no extension change");
				}
				// set report mode
				if(extension)
					sendDataMode32();
				else
					sendDataMode30();
			}

			bcase 0x21:
			{
				logMsg("got read report, %X %X", packet[7], packet[8]);
			}

			bcase 0x22:
			{
				logMsg("ack output report, %X %X", packet[4], packet[5]);
			}

			bdefault:
			{
				logMsg("unhandled packet type %d from wiimote", packet[1]);
			}
		}
	}

	static base_pollHandlerFuncProto(fdDataHandler)
	{
		return static_cast<BluezWiimote*>(data)->handleAvailableInputData();
	}

	bool handleAvailableInputData()
	{
		uchar packet[23];
	    int bytesToRead = fd_bytesReadable(intSocket);
	    //logMsg("%d bytes ready", bytesToRead);
		do
		{
			int len = read(intSocket, packet, sizeof packet);
			if(unlikely(len <= 0))
			{
				logWarn("read error %d, closing wiimote %d", len, devId);
				close();
				closeDeviceCommon(this);
				return 0;
			}
			//logMsg("processing Wiimote packet type 0x%X", packet[1]);
			processPacket(packet, devId);
			bytesToRead -= len;
		} while(bytesToRead > 0);
		//logMsg("done reading Wiimote");
		return 1;
	}

	void close()
	{
		if(Base::hasFDEvents)
			Base::removePollEvent(intSocket);
		if(intSocket > 0 && ::close(intSocket) != 0)
		{
			logMsg("error closing interrupt socket");
		}
		intSocket = 0;
		if(ctlSocket > 0 && ::close(ctlSocket) != 0)
		{
			logMsg("error closing control socket");
		}
		ctlSocket = 0;
		list.remove(this);
	}
};

StaticDLList<BluezWiimote*, maxGamepadsPerTypeStorage> BluezWiimote::list;

using namespace Base;
uint maxGamepadsPerType = maxGamepadsPerTypeStorage;

#ifndef CONFIG_BASE_HAS_FD_EVENTS
static int ePoll = 0;
static int threadQuitPipe[2];
#endif

#define CONFIG_BLUEZ_ICP

static const uint maxBTInputs = maxGamepadsPerTypeStorage
#ifdef CONFIG_BLUEZ_ICP
+ maxGamepadsPerTypeStorage
#endif
;

StaticDLList<BluezInputDevice*, maxBTInputs> inputDevList;

static ThreadPThread runThread;

static int devId = -1, socket = -1;
static bool inDetect = 0;
BTConnectionEventFunc connectFunc = 0;

static void closeDeviceCommon(BluezInputDevice *dev)
{
	inputDevList.remove(dev);
	Base::sendInputDevChangeMessageToMain(dev->devId, dev->devType, Base::InputDevChange::REMOVED);
	delete dev;
}

uint wiimotes() { return BluezWiimote::list.size; }
uint iCPs() { return BluezIControlPad::list.size; }
uint zeemotes() { return BluezZeemote::list.size; }

bool devsAreConnected()
{
	return inputDevList.size;
}

void closeBT()
{
	#ifdef CONFIG_BASE_ANDROID
	if(!bluez_loaded)
		return;
	#endif
	if(runThread.running)
	{
		#ifndef CONFIG_BASE_HAS_FD_EVENTS
		int ret = write(threadQuitPipe[1], &threadQuitPipe[1], 1); // tell input thread to quit
		#endif
		runThread.join();
	}
	forEachInDLList(&inputDevList, e)
	{
		e->close();
		e_it.removeElem();
		callSafe(onInputDevChangeHandler, onInputDevChangeHandlerCtx, (InputDevChange){ e->devId, e->devType, Base::InputDevChange::REMOVED });
		delete e;
	}
	assert(BluezWiimote::list.size == 0);
	#ifdef CONFIG_BLUEZ_ICP
	assert(BluezIControlPad::list.size == 0);
	#endif
	assert(BluezZeemote::list.size == 0);
	assert(inputDevList.size == 0);
	inDetect = 0;
}

CallResult initBT()
{
	if(socket >= 0)
		return OK;
	#ifdef CONFIG_BASE_ANDROID
	if(bluez_dl() != OK)
		return INVALID_PARAMETER;
	#endif
	devId = hci_get_route(0);
	if(devId < 0)
	{
		logMsg("no routes");
		return INVALID_PARAMETER;
	}
	socket = hci_open_dev(devId);
	if(socket < 0)
	{
		logMsg("error opening socket");
		return INVALID_PARAMETER;
	}
	logMsg("BT present");
	#ifdef CONFIG_BLUEZ_ICP
	BluezIControlPad::list.init();
	#endif
	BluezZeemote::list.init();
	BluezWiimote::list.init();
	inputDevList.init();
	return OK;
}

#ifndef CONFIG_BASE_HAS_FD_EVENTS
static void readDevInput()
{
	struct epoll_event event[maxBTInputs+1];
	for(;;)
	{
		// TODO: handle -1 return
		int events = epoll_wait(ePoll, event, sizeofArray(event), -1);
		//logMsg("%d events ready", events);
		iterateTimes(events, i)
		{
			if(unlikely(event[i].data.ptr == 0))
			{
				uchar dummy;
				logMsg("getting quit message");
				int ret = read(threadQuitPipe[0], &dummy, 1);
				return;
			}

			PollHandler *e = (PollHandler*)event[i].data.ptr;
			e->func(e->data, event[i].events);
		}
	}
}
#endif

uint scanSecs = 4;

static CallResult detectDevs()
{
	inDetect = 1;
	logMsg("starting Bluetooth scan, max devices/type %d", maxGamepadsPerType);
	int devices = 0, maxDevices = 10;
	inquiry_info *deviceInfo = 0;
	devices = hci_inquiry(devId, scanSecs, maxDevices, 0, &deviceInfo, IREQ_CACHE_FLUSH);
	if(devices == -1)
	{
		logMsg("inquiry failed");
		return INVALID_PARAMETER;
	}

	logMsg("%d devices", devices);
	iterateTimes(devices, i)
	{
		if(!testSupportedBTDevClasses(deviceInfo[i].dev_class))
		{
			logMsg("skipping device due to class %X:%X:%X", deviceInfo[i].dev_class[0], deviceInfo[i].dev_class[1], deviceInfo[i].dev_class[2]);
			continue;
		}
		char name[248];
		if(hci_read_remote_name(socket, &deviceInfo[i].bdaddr, sizeof(name), name, 0) < 0)
		{
			logMsg("error reading device name");
			sendBTMessageToMain(MSG_ERR_NAME);
			continue;
		}
		logMsg("device name: %s", name);
		if(strstr(name, WiimoteDefs::btNamePrefix))
		{
			if((uint)BluezWiimote::list.size == maxGamepadsPerType)
			{
				logMsg("max wiimotes reached");
				continue;
			}
			BluezWiimote *dev = new BluezWiimote;
			if(!dev)
			{
				logErr("out of memory");
				break;
			}
			bacpy(&dev->addr, &deviceInfo[i].bdaddr);
			BluezWiimote::list.add(dev);
			inputDevList.add(dev);
		}
		#ifdef CONFIG_BLUEZ_ICP
		else if(strstr(name, IControlPadDefs::btNamePrefix))
		{
			if((uint)BluezIControlPad::list.size == maxGamepadsPerType)
			{
				logMsg("max iCPs reached");
				continue;
			}
			BluezIControlPad *dev = new BluezIControlPad;
			if(!dev)
			{
				logErr("out of memory");
				break;
			}
			bacpy(&dev->bdaddr, &deviceInfo[i].bdaddr);
			BluezIControlPad::list.add(dev);
			inputDevList.add(dev);
		}
		#endif
		#ifdef CONFIG_BLUEZ_ZEEMOTE
		else if(strstr(name, ZeemoteDefs::btName))
		{
			if((uint)BluezZeemote::list.size == maxGamepadsPerType)
			{
				logMsg("max Zeemotes reached");
				continue;
			}
			BluezZeemote *dev = new BluezZeemote;
			if(!dev)
			{
				logErr("out of memory");
				break;
			}
			bacpy(&dev->bdaddr, &deviceInfo[i].bdaddr);
			BluezZeemote::list.add(dev);
			inputDevList.add(dev);
		}
		#endif
	}
	if(deviceInfo)
		free(deviceInfo);
	logMsg("found wiimotes: %d, iCPs: %d", BluezWiimote::list.size, BluezIControlPad::list.size);

	if(!inputDevList.size)
	{
		sendBTMessageToMain(MSG_NO_DEVS);
		return INVALID_PARAMETER;
	}

	#ifdef CONFIG_BASE_HAS_FD_EVENTS
	int ePoll = 0; // ePoll value ignored when using event loop in base module
	#else
	if(!ePoll)
	{
		logMsg("doing one-time epoll & pipe creation");
		ePoll = epoll_create(maxBTInputs+1);
		if(pipe(threadQuitPipe) != 0)
		{
			logMsg("failed to create pipe");
			return INVALID_PARAMETER;
		}
		static struct epoll_event threadPipeEv;
		threadPipeEv.data.ptr = 0; // epoll loop exits when event ptr == 0 has data to read
		threadPipeEv.events = EPOLLIN;
		epoll_ctl(ePoll, EPOLL_CTL_ADD, threadQuitPipe[0], &threadPipeEv);
	}
	#endif

	uint player = 0;
	forEachInDLList(&BluezWiimote::list, e)
	{
		if(e->setup(ePoll, player) == OK)
		{
			logMsg("wiimote %d set up", player);
			player++;
		}
		else
		{
			sendBTMessageToMain(MSG_ERR_CHANNEL);
			logMsg("error setting up wiimote %d", player);
			e_it.removeElem();
			inputDevList.remove((BluezInputDevice*&)e);
			delete e;
		}
	}

	#ifdef CONFIG_BLUEZ_ICP
	player = 0;
	forEachInDLList(&BluezIControlPad::list, e)
	{
		if(e->setup(ePoll, player) == OK)
		{
			logMsg("iCP %d set up", player);
			player++;
		}
		else
		{
			sendBTMessageToMain(MSG_ERR_CHANNEL);
			logMsg("error setting up iCP %d", player);
			e_it.removeElem();
			inputDevList.remove((BluezInputDevice*&)e);
			delete e;
		}
	}
	#endif

	#ifdef CONFIG_BLUEZ_ZEEMOTE
	player = 0;
	forEachInDLList(&BluezZeemote::list, e)
	{
		if(e->setup(ePoll, player) == OK)
		{
			logMsg("Zeemote %d set up", player);
			player++;
		}
		else
		{
			sendBTMessageToMain(MSG_ERR_CHANNEL);
			logMsg("error setting up Zeemote %d", player);
			e_it.removeElem();
			inputDevList.remove((BluezInputDevice*&)e);
			delete e;
		}
	}
	#endif

	return OK;
}

static int runInput(void*)
{
	#if defined(CONFIG_BASE_ANDROID) && CONFIG_ENV_ANDROID_MINSDK < 9
	if(Base::android_attachInputThreadToJVM() != OK)
		return 0;
	#endif
	detectDevs();
	inDetect = 0;
	#ifndef CONFIG_BASE_HAS_FD_EVENTS
	if(inputDevList.size)
	{
		logMsg("entering epoll loop");
		readDevInput();
	}
	#endif
	#if defined(CONFIG_BASE_ANDROID) && CONFIG_ENV_ANDROID_MINSDK < 9
	Base::android_detachInputThreadToJVM();
	#endif
	return 0;
}

bool startBT(BTConnectionEventFunc btConnectFunc)
{
	connectFunc = btConnectFunc;
	if(!inDetect)
	{
		closeBT();
		runThread.create(0, runInput, 0);
		return 1;
	}
	else
	{
		logMsg("previous bluetooth detection still running");
		return 0;
	}
}

}

#undef thisModuleName
