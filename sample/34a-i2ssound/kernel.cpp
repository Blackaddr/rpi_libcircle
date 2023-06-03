//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2022  R. Stange <rsta2@o2online.de>
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
#include "config.h"
#include <circle/sound/pwmsoundbasedevice.h>
#include <circle/sound/i2ssoundbasedevice.h>
#include <circle/sound/hdmisoundbasedevice.h>
#include <circle/sound/usbsoundbasedevice.h>
#include <circle/machineinfo.h>
#include <circle/util.h>
#include <assert.h>
#include <circle/globsystem.h>


#define FORMAT		SoundFormatSigned16
#define TYPE		s16
#define TYPE_SIZE	sizeof (s16)
#define FACTOR		((1 << 15)-1)
#define NULL_LEVEL	0


static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_I2CMaster (CMachineInfo::Get ()->GetDevice (DeviceI2CMaster), TRUE),
	m_USBHCI (&m_Interrupt, &m_Timer, FALSE),
    m_Codec(&m_I2CMaster, DAC_I2C_ADDRESS),
	m_pSound (0)
{
	m_ActLED.Blink (5);	// show we are alive
	g_LoggerPtr       = &m_Logger;
	g_ScreenPtr       = &m_Screen;
	g_interruptSysPtr = &m_Interrupt;
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

	if (bOK)
	{
		bOK = m_I2CMaster.Initialize ();
	}

	if (bOK)
	{
		bOK = m_USBHCI.Initialize ();
	}

	return bOK;
}

void rxIsr(void *param)
{
	LogScreen("RX ISR called\n");
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);
	LogScreen("Compile time: " __DATE__ " " __TIME__ "\n");

	m_Codec.disable();
	m_Codec.enable();

	LogScreen("Using I2S sound with slave peripheral (codec is master)\n");
	m_pSound = new CI2SSoundBaseDevice (&m_Interrupt, SAMPLE_RATE, CHUNK_SIZE, TRUE,
						nullptr, DAC_I2C_ADDRESS, CI2SSoundBaseDevice::DeviceModeTXRX);
	assert (m_pSound != 0);

	m_pSound->RegisterSecondaryRxHandler(rxIsr, nullptr);

	// configure sound device
	if (!m_pSound->AllocateQueue (QUEUE_SIZE_MSECS))
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot allocate sound queue");
		LogScreen("Cannot allocate sound queue\n");
	}
	if (!m_pSound->AllocateReadQueue (QUEUE_SIZE_MSECS))
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot allocate input sound queue");
		LogScreen("Cannot allocate input sound queue\n");
	}
	m_pSound->SetWriteFormat (FORMAT, WRITE_CHANNELS);
	m_pSound->SetReadFormat (FORMAT, WRITE_CHANNELS);

	// start sound device
	if (!m_pSound->Start ())
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot start sound device");
		LogScreen("Cannot start sound device\n");
	}

	m_Logger.Write (FromKernel, LogNotice, "Playing pass thru audio\n");
	LogScreen("Playing pass thru audio\n");

	// copy sound data
	for (unsigned nCount = 0; 1; nCount++)
	{
		u8 Buffer[TYPE_SIZE*WRITE_CHANNELS*128];
		int nBytes = m_pSound->Read (Buffer, sizeof Buffer);
		if (nBytes > 0)
		{
			int nResult = m_pSound->Write (Buffer, nBytes);
			if (nResult != nBytes)
			{
				m_Logger.Write (FromKernel, LogWarning, "Sound data dropped");
				LogScreen("Sound data dropped\n");
			}
		}

		m_Screen.Rotor (0, nCount);
		m_Scheduler.Yield ();
	}

	return ShutdownHalt;
}
