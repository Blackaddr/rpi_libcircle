//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "kernel.h"
#include "System.h"

#define uint8_t u8
#define int8_t i8

using namespace BALibrary;

static const char FromKernel[] = "kernel";

constexpr int WM8731_I2C_ADDR = 0x1A;
constexpr int MAX11607_I2C_ADDR = 0x34;

#define I2C_MASTER_DEVICE	1		// 0 on Raspberry Pi 1 Rev. 1 boards, 1 otherwise
#define I2C_MASTER_CONFIG	0		// 0 or 1 on Raspberry Pi 4, 0 otherwise
#define I2C_FAST_MODE		FALSE		// standard mode (100 Kbps) or fast mode (400 Kbps)
#define I2C_SLAVE_ADDRESS	0x1A	// 7 bit slave address

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer)
	//m_I2CMaster (I2C_MASTER_DEVICE, I2C_FAST_MODE, I2C_MASTER_CONFIG)
	//m_I2CEchoClient (I2C_SLAVE_ADDRESS, &m_I2CMaster)
{
	g_LoggerPtr       = &m_Logger;
	g_ScreenPtr       = &m_Screen;
	g_interruptSysPtr = &m_Interrupt;
	g_timerPtr        = &m_Timer;

	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}

		bOK = m_Logger.Initialize (pTarget);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	// if (bOK)
	// {
	// 	bOK = m_I2CMaster.Initialize ();
	// }

	m_codec.disable();
	m_codec.enable();
	m_codec.setAdcBypass(true);

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	CString Message;
	Message.Format("Compile time: " __DATE__ " " __TIME__ "\n");

	m_Logger.Write(FromKernel, LogNotice, Message);
	m_Screen.Write (Message, Message.GetLength());

	while(true) {
		LogScreen("Running...\n");
		delay(2000);
	}

	return ShutdownHalt;
}

// TShutdownMode CKernel::Run (void)
// {
// 	CString Message;
// 	Message.Format("Compile time: " __DATE__ " " __TIME__ "\n");
// 	//m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);
// 	m_Logger.Write(FromKernel, LogNotice, Message);
// 	m_Screen.Write (Message, Message.GetLength());

// 	constexpr uint8_t WM8731_RESET_REG = 15;
// 	unsigned devicesFound = 0;
// 	unsigned devicesMissing = 0;

// 	for (uint8_t i=0; i < 0x7F; i++) {
// 		uint8_t val = 0;

// 		if (m_I2CMaster.Write (i, &val, 1) != (int) 1)
// 		{

// 			//Message.Format ("I2C write error (data) at address %02X\n", i);
// 			//m_Screen.Write (Message, Message.GetLength());
// 			//m_Logger.Write (FromKernel, LogWarning, Message);
// 			devicesMissing++;

// 		} else {
// 			devicesFound++;

// 			Message.Format ("I2C device FOUND at address 0x%02X\n", i);
// 			m_Screen.Write (Message, Message.GetLength());
// 			m_Logger.Write (FromKernel, LogNotice, Message);
// 		}
// 	}
// 	Message.Format ("Scan complete, found %u devices, missing %u devices\n", devicesFound, devicesMissing);
// 	m_Screen.Write (Message, Message.GetLength());
// 	m_Logger.Write (FromKernel, LogNotice, Message);

// 	return ShutdownHalt;
// }
