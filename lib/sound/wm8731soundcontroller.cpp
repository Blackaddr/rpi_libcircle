//
// wm8731soundcontroller.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2022  R. Stange <rsta2@o2online.de>
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
#include <circle/sound/wm8731soundcontroller.h>
#include <circle/string.h>
#include <circle/types.h>
#include <assert.h>

#include <circle/screen.h>
#include <circle/serial.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/types.h>
#include <circle/globsystem.h>

#define delay(x) do { CTimer::Get()->MsDelay(x);} while(0)

CWM8731SoundController::CWM8731SoundController (CI2CMaster *pI2CMaster, u8 uchI2CAddress)
:	m_pI2CMaster (pI2CMaster),
	m_uchI2CAddress (uchI2CAddress)
{
    resetInternalReg();
}

boolean CWM8731SoundController::Probe (void)
{
	if (m_uchI2CAddress)
	{
		return InitWM8731 (m_uchI2CAddress);
	}

	if (InitWM8731 (0x1A))
	{
		m_uchI2CAddress = 0x1A;

		return TRUE;
	}

	return FALSE;
}

// For WM8731 i2c register is 7 bits and value is 9 bits,
// so let's have a helper for packing this into two bytes
//#define SHIFT_BIT(r, v) {((v&0x0100)>>8) | (r<<1), (v&0xff)}

// use const instead of define for proper scoping
constexpr int WM8731_I2C_ADDR = 0x1A;

// The WM8731 register map
constexpr int WM8731_REG_LLINEIN   = 0;
constexpr int WM8731_REG_RLINEIN   = 1;
constexpr int WM8731_REG_LHEADOUT  = 2;
constexpr int WM8731_REG_RHEADOUT  = 3;
constexpr int WM8731_REG_ANALOG     =4;
constexpr int WM8731_REG_DIGITAL   = 5;
constexpr int WM8731_REG_POWERDOWN = 6;
constexpr int WM8731_REG_INTERFACE = 7;
constexpr int WM8731_REG_SAMPLING  = 8;
constexpr int WM8731_REG_ACTIVE    = 9;
constexpr int WM8731_REG_RESET    = 15;

// Register Masks and Shifts
// Register 0
constexpr int WM8731_LEFT_INPUT_GAIN_ADDR = 0;
constexpr int WM8731_LEFT_INPUT_GAIN_MASK = 0x1F;
constexpr int WM8731_LEFT_INPUT_GAIN_SHIFT = 0;
constexpr int WM8731_LEFT_INPUT_MUTE_ADDR = 0;
constexpr int WM8731_LEFT_INPUT_MUTE_MASK = 0x80;
constexpr int WM8731_LEFT_INPUT_MUTE_SHIFT = 7;
constexpr int WM8731_LINK_LEFT_RIGHT_IN_ADDR = 0;
constexpr int WM8731_LINK_LEFT_RIGHT_IN_MASK = 0x100;
constexpr int WM8731_LINK_LEFT_RIGHT_IN_SHIFT = 8;
// Register 1
constexpr int WM8731_RIGHT_INPUT_GAIN_ADDR = 1;
constexpr int WM8731_RIGHT_INPUT_GAIN_MASK = 0x1F;
constexpr int WM8731_RIGHT_INPUT_GAIN_SHIFT = 0;
constexpr int WM8731_RIGHT_INPUT_MUTE_ADDR = 1;
constexpr int WM8731_RIGHT_INPUT_MUTE_MASK = 0x80;
constexpr int WM8731_RIGHT_INPUT_MUTE_SHIFT = 7;
constexpr int WM8731_LINK_RIGHT_LEFT_IN_ADDR = 1;
constexpr int WM8731_LINK_RIGHT_LEFT_IN_MASK = 0x100;
constexpr int WM8731_LINK_RIGHT_LEFT_IN_SHIFT = 8;
// Register 2
constexpr int WM8731_LEFT_HEADPHONE_VOL_ADDR = 2;
constexpr int WM8731_LEFT_HEADPHONE_VOL_MASK = 0x7F;
constexpr int WM8731_LEFT_HEADPHONE_VOL_SHIFT = 0;
constexpr int WM8731_LEFT_HEADPHONE_ZCD_ADDR = 2;
constexpr int WM8731_LEFT_HEADPHONE_ZCD_MASK = 0x80;
constexpr int WM8731_LEFT_HEADPHONE_ZCD_SHIFT = 7;
constexpr int WM8731_LEFT_HEADPHONE_LINK_ADDR = 2;
constexpr int WM8731_LEFT_HEADPHONE_LINK_MASK = 0x100;
constexpr int WM8731_LEFT_HEADPHONE_LINK_SHIFT = 8;
// Register 3
constexpr int WM8731_RIGHT_HEADPHONE_VOL_ADDR = 3;
constexpr int WM8731_RIGHT_HEADPHONE_VOL_MASK = 0x7F;
constexpr int WM8731_RIGHT_HEADPHONE_VOL_SHIFT = 0;
constexpr int WM8731_RIGHT_HEADPHONE_ZCD_ADDR = 3;
constexpr int WM8731_RIGHT_HEADPHONE_ZCD_MASK = 0x80;
constexpr int WM8731_RIGHT_HEADPHONE_ZCD_SHIFT = 7;
constexpr int WM8731_RIGHT_HEADPHONE_LINK_ADDR = 3;
constexpr int WM8731_RIGHT_HEADPHONE_LINK_MASK = 0x100;
constexpr int WM8731_RIGHT_HEADPHONE_LINK_SHIFT = 8;
// Register 4
constexpr int WM8731_ADC_BYPASS_ADDR = 4;
constexpr int WM8731_ADC_BYPASS_MASK = 0x8;
constexpr int WM8731_ADC_BYPASS_SHIFT = 3;
constexpr int WM8731_DAC_SELECT_ADDR = 4;
constexpr int WM8731_DAC_SELECT_MASK = 0x10;
constexpr int WM8731_DAC_SELECT_SHIFT = 4;
// Register 5
constexpr int WM8731_DAC_MUTE_ADDR = 5;
constexpr int WM8731_DAC_MUTE_MASK = 0x8;
constexpr int WM8731_DAC_MUTE_SHIFT = 3;
constexpr int WM8731_HPF_DISABLE_ADDR = 5;
constexpr int WM8731_HPF_DISABLE_MASK = 0x1;
constexpr int WM8731_HPF_DISABLE_SHIFT = 0;
constexpr int WM8731_HPF_STORE_OFFSET_ADDR = 5;
constexpr int WM8731_HPF_STORE_OFFSET_MASK = 0x10;
constexpr int WM8731_HPF_STORE_OFFSET_SHIFT = 4;
// Register 7
constexpr int WM8731_LRSWAP_ADDR = 5;
constexpr int WM8731_LRSWAP_MASK = 0x20;
constexpr int WM8731_LRSWAPE_SHIFT = 5;

// Register 9
constexpr int WM8731_ACTIVATE_ADDR = 9;
constexpr int WM8731_ACTIVATE_MASK = 0x1;

boolean CWM8731SoundController::InitWM8731 (u8 uchI2CAddress)
{
    disable();
    delay(100);
    enable();


	// // based on https://github.com/RASPIAUDIO/ULTRA/blob/main/ultra.c
	// // Licensed under GPLv3
	// static const u8 initBytes[][2] =
	// {
	// 	// reset (reg 15)
	// 	SHIFT_BIT(WM8731_REG_RESET, 0x000),
	// 	// Power up all domains except OUTPD and microphone (reg 6)
	// 	SHIFT_BIT(WM8731_REG_POWERDOWN, 0x12),
    //     // disable ADC bypass, enable DAC sel (reg 4)
    //     SHIFT_BIT(WM8731_ADC_BYPASS_ADDR, 0x1A), // normally 0x12
    //     // HPF disable, DAC unmuted (reg 5)
    //     SHIFT_BIT(WM8731_HPF_DISABLE_ADDR, 0x1),
    //     // Input Gains and mutes (reg 0/1)
    //     SHIFT_BIT(WM8731_LEFT_INPUT_GAIN_ADDR, 0x17),
    //     SHIFT_BIT(WM8731_RIGHT_INPUT_GAIN_ADDR, 0x17),
    //     // Mute headphones (reg 2/3)
    //     SHIFT_BIT(WM8731_REG_LHEADOUT, 0),
    //     SHIFT_BIT(WM8731_REG_RHEADOUT, 0),
    //     // Audio interface (reg 7)
    //     SHIFT_BIT(WM8731_REG_INTERFACE, 0x42),
    //     // Sampling (reg 8)
    //     SHIFT_BIT(WM8731_REG_SAMPLING, 0x20),
    //     // Activate interface (Reg 9)
    //     SHIFT_BIT(WM8731_ACTIVATE_ADDR, 0x1),
    //     // Power up outputs (reg 6)
    //     SHIFT_BIT(WM8731_REG_POWERDOWN, 0x2)
	// };

	// assert (m_pI2CMaster);
	// assert (uchI2CAddress);
	// for (auto &command : initBytes)
	// {
	// 	if (   m_pI2CMaster->Write (uchI2CAddress, &command, sizeof (command))
	// 	    != sizeof (command))
	// 	{
	// 		return FALSE;
	// 	}
	// }

	return TRUE;
}

// Reset the internal shadow register array to match
// the reset state of the codec.
void CWM8731SoundController::resetInternalReg(void) {
	// Set to reset state
	regArray[0] = 0x97;
	regArray[1] = 0x97;
	regArray[2] = 0x79;
	regArray[3] = 0x79;
	regArray[4] = 0x0a;
	regArray[5] = 0x8;
	regArray[6] = 0x9f;
	regArray[7] = 0xa;
	regArray[8] = 0;
	regArray[9] = 0;
}

// Powerdown and disable the codec
void CWM8731SoundController::disable(void)
{
	LogScreen("Disabling codec\n");
	// if (m_wireStarted == false) {
	//     Wire.begin();
	//     m_wireStarted = true;
	// }
	// setOutputStrength();
	//m_i2cMaster.Initialize ();

	// set OUTPD to '1' (powerdown), which is bit 4
	regArray[WM8731_REG_POWERDOWN] |= 0x10;
	write(WM8731_REG_POWERDOWN, regArray[WM8731_REG_POWERDOWN]);
	delay(100); // wait for power down

	// power down the rest of the supplies
	write(WM8731_REG_POWERDOWN, 0x9f); // complete codec powerdown
	delay(100);

	resetCodec();
}

// // Powerup and unmute the codec
// void CWM8731SoundController::enable(void)
// {

// 	disable(); // disable first in case it was already powered up

// 	LogScreen("Enabling codec\n");
// 	// if (m_wireStarted == false) {
// 	//     Wire.begin();
// 	//     m_wireStarted = true;
// 	// }
// 	// setOutputStrength();

// 	// Sequence from WAN0111.pdf
// 	// Begin configuring the codec
// 	resetCodec();
// 	delay(100); // wait for reset

// 	// Power up all domains except OUTPD and microphone
// 	regArray[WM8731_REG_POWERDOWN] = 0x12;
// 	write(WM8731_REG_POWERDOWN, regArray[WM8731_REG_POWERDOWN]);
// 	delay(100); // wait for codec powerup


// 	setAdcBypass(false); // causes a slight click
// 	setDacSelect(true);
// 	setHPFDisable(true);
// 	setLeftInputGain(0x17); // default input gain
//     setRightInputGain(0x17);
// 	setLeftInMute(false); // no input mute
// 	setRightInMute(false);
// 	setDacMute(false); // unmute the DAC

// 	// link, but mute the headphone outputs
// 	regArray[WM8731_REG_LHEADOUT] = WM8731_LEFT_HEADPHONE_LINK_MASK;
// 	write(WM8731_REG_LHEADOUT, regArray[WM8731_REG_LHEADOUT]);      // volume off
// 	regArray[WM8731_REG_RHEADOUT] = WM8731_RIGHT_HEADPHONE_LINK_MASK;
// 	write(WM8731_REG_RHEADOUT, regArray[WM8731_REG_RHEADOUT]);

// 	/// Configure the audio interface
// 	write(WM8731_REG_INTERFACE, 0x02); // I2S, 16 bit, MCLK slave
// 	regArray[WM8731_REG_INTERFACE] = 0x2;

// 	write(WM8731_REG_SAMPLING, 0x20);  // 256*Fs, 44.1 kHz, MCLK/1
// 	regArray[WM8731_REG_SAMPLING] = 0x20;
// 	delay(100); // wait for interface config

// 	// Activate the audio interface
// 	setActivate(true);
// 	delay(100);

// 	write(WM8731_REG_POWERDOWN, 0x02); // power up outputs
// 	regArray[WM8731_REG_POWERDOWN] = 0x02;
// 	delay(500); // wait for output to power up

// 	delay(100); // wait for mute ramp

// }

// Set the PGA gain on the Left channel
void CWM8731SoundController::setLeftInputGain(int val)
{
	regArray[WM8731_LEFT_INPUT_GAIN_ADDR] &= ~WM8731_LEFT_INPUT_GAIN_MASK;
	regArray[WM8731_LEFT_INPUT_GAIN_ADDR] |=
			((val << WM8731_LEFT_INPUT_GAIN_SHIFT) & WM8731_LEFT_INPUT_GAIN_MASK);
	write(WM8731_LEFT_INPUT_GAIN_ADDR, regArray[WM8731_LEFT_INPUT_GAIN_ADDR]);
}

// Mute control on the ADC Left channel
void CWM8731SoundController::setLeftInMute(bool val)
{
	if (val) {
		regArray[WM8731_LEFT_INPUT_MUTE_ADDR] |= WM8731_LEFT_INPUT_MUTE_MASK;
	} else {
		regArray[WM8731_LEFT_INPUT_MUTE_ADDR] &= ~WM8731_LEFT_INPUT_MUTE_MASK;
	}
	write(WM8731_LEFT_INPUT_MUTE_ADDR, regArray[WM8731_LEFT_INPUT_MUTE_ADDR]);
}

// Link the gain/mute controls for Left and Right channels
void CWM8731SoundController::setLinkLeftRightIn(bool val)
{
	if (val) {
		regArray[WM8731_LINK_LEFT_RIGHT_IN_ADDR] |= WM8731_LINK_LEFT_RIGHT_IN_MASK;
		regArray[WM8731_LINK_RIGHT_LEFT_IN_ADDR] |= WM8731_LINK_RIGHT_LEFT_IN_MASK;
	} else {
		regArray[WM8731_LINK_LEFT_RIGHT_IN_ADDR] &= ~WM8731_LINK_LEFT_RIGHT_IN_MASK;
		regArray[WM8731_LINK_RIGHT_LEFT_IN_ADDR] &= ~WM8731_LINK_RIGHT_LEFT_IN_MASK;
	}
	write(WM8731_LINK_LEFT_RIGHT_IN_ADDR, regArray[WM8731_LINK_LEFT_RIGHT_IN_ADDR]);
	write(WM8731_LINK_RIGHT_LEFT_IN_ADDR, regArray[WM8731_LINK_RIGHT_LEFT_IN_ADDR]);
}

// Set the PGA input gain on the Right channel
void CWM8731SoundController::setRightInputGain(int val)
{
	regArray[WM8731_RIGHT_INPUT_GAIN_ADDR] &= ~WM8731_RIGHT_INPUT_GAIN_MASK;
	regArray[WM8731_RIGHT_INPUT_GAIN_ADDR] |=
			((val << WM8731_RIGHT_INPUT_GAIN_SHIFT) & WM8731_RIGHT_INPUT_GAIN_MASK);
	write(WM8731_RIGHT_INPUT_GAIN_ADDR, regArray[WM8731_RIGHT_INPUT_GAIN_ADDR]);
}

// Mute control on the input ADC right channel
void CWM8731SoundController::setRightInMute(bool val)
{
	if (val) {
		regArray[WM8731_RIGHT_INPUT_MUTE_ADDR] |= WM8731_RIGHT_INPUT_MUTE_MASK;
	} else {
		regArray[WM8731_RIGHT_INPUT_MUTE_ADDR] &= ~WM8731_RIGHT_INPUT_MUTE_MASK;
	}
	write(WM8731_RIGHT_INPUT_MUTE_ADDR, regArray[WM8731_RIGHT_INPUT_MUTE_ADDR]);
}

// Left/right swap control
void CWM8731SoundController::setLeftRightSwap(bool val)
{
	if (val) {
		regArray[WM8731_LRSWAP_ADDR] |= WM8731_LRSWAP_MASK;
	} else {
		regArray[WM8731_LRSWAP_ADDR] &= ~WM8731_LRSWAP_MASK;
	}
	write(WM8731_LRSWAP_ADDR, regArray[WM8731_LRSWAP_ADDR]);
}

void CWM8731SoundController::setHeadphoneVolume(float volume)
{
	// the codec volume goes from 0x30 to 0x7F. Anything below 0x30 is mute.
	// 0dB gain is 0x79. Total range is 0x50 (80) possible values.
	unsigned vol;
	constexpr unsigned RANGE = 80.0f;
	if (volume < 0.0f) {
		vol = 0;
	} else if (volume > 1.0f) {
		vol = 0x7f;
	} else {
		vol = 0x2f + static_cast<unsigned>(volume * RANGE);
	}
	regArray[WM8731_LEFT_HEADPHONE_VOL_ADDR] &= ~WM8731_LEFT_HEADPHONE_VOL_MASK; // clear the volume first
	regArray[WM8731_LEFT_HEADPHONE_VOL_ADDR] |=
			((vol << WM8731_LEFT_HEADPHONE_VOL_SHIFT) & WM8731_LEFT_HEADPHONE_VOL_MASK);
	write(WM8731_LEFT_HEADPHONE_VOL_ADDR, regArray[WM8731_LEFT_HEADPHONE_VOL_ADDR]);
}

// Dac output mute control
void CWM8731SoundController::setDacMute(bool val)
{
	if (val) {
		regArray[WM8731_DAC_MUTE_ADDR] |= WM8731_DAC_MUTE_MASK;
	} else {
		regArray[WM8731_DAC_MUTE_ADDR] &= ~WM8731_DAC_MUTE_MASK;
	}
	write(WM8731_DAC_MUTE_ADDR, regArray[WM8731_DAC_MUTE_ADDR]);
}

// Switches the DAC audio in/out of the output path
void CWM8731SoundController::setDacSelect(bool val)
{
	if (val) {
		regArray[WM8731_DAC_SELECT_ADDR] |= WM8731_DAC_SELECT_MASK;
	} else {
		regArray[WM8731_DAC_SELECT_ADDR] &= ~WM8731_DAC_SELECT_MASK;
	}
	write(WM8731_DAC_SELECT_ADDR, regArray[WM8731_DAC_SELECT_ADDR]);
}

// Bypass sends the ADC input audio (analog) directly to analog output stage
// bypassing all digital processing
void CWM8731SoundController::setAdcBypass(bool val)
{
	if (val) {
		regArray[WM8731_ADC_BYPASS_ADDR] |= WM8731_ADC_BYPASS_MASK;
	} else {
		regArray[WM8731_ADC_BYPASS_ADDR] &= ~WM8731_ADC_BYPASS_MASK;
	}
	write(WM8731_ADC_BYPASS_ADDR, regArray[WM8731_ADC_BYPASS_ADDR]);
}

// Enable/disable the dynamic HPF (recommended, it creates noise)
void CWM8731SoundController::setHPFDisable(bool val)
{
	if (val) {
		regArray[WM8731_HPF_DISABLE_ADDR] |= WM8731_HPF_DISABLE_MASK;
	} else {
		regArray[WM8731_HPF_DISABLE_ADDR] &= ~WM8731_HPF_DISABLE_MASK;
	}
	write(WM8731_HPF_DISABLE_ADDR, regArray[WM8731_HPF_DISABLE_ADDR]);
}

// Activate/deactive the I2S audio interface
void CWM8731SoundController::setActivate(bool val)
{
	if (val) {
		write(WM8731_ACTIVATE_ADDR, WM8731_ACTIVATE_MASK);
	} else {
		write(WM8731_ACTIVATE_ADDR, 0);
	}

}

// Trigger the on-chip codec reset
void CWM8731SoundController::resetCodec(void)
{
	write(WM8731_REG_RESET, 0x0);
	resetInternalReg();
}

void CWM8731SoundController::recalibrateDcOffset(void)
{
    // mute the inputs
    setLeftInMute(true);
    setRightInMute(true);

    // enable the HPF and DC offset store
    setHPFDisable(false);
    regArray[WM8731_HPF_STORE_OFFSET_ADDR] |= WM8731_HPF_STORE_OFFSET_MASK;
    write(WM8731_REG_DIGITAL, regArray[WM8731_REG_DIGITAL]);

    delay(1000); // wait for the DC offset to be calculated over 1 second
    setHPFDisable(true); // disable the dynamic HPF calculation
    delay(500);

    // unmute the inputs
    setLeftInMute(false);
    setRightInMute(false);
}

// Direct write control to the codec
bool CWM8731SoundController::writeI2C(unsigned int addr, unsigned int val)
{
	return write(addr, val);
}

// Low level write control for the codec via the Teensy I2C interface
bool CWM8731SoundController::write(unsigned int reg, unsigned int val)
{
	bool done = false;

	while (!done) {

        u8 buffer[2];
		buffer[0] = (reg << 1) | ((val >> 8) & 1);
		buffer[1] = val & 0xFFU;
		int result = m_pI2CMaster->Write (WM8731_I2C_ADDR, buffer, 2);
		if (result != 2) {
			// retry
			CString msg;
			msg.Format("CWM8731SoundController::write(): could not write 2 bytes, only wrote %d\n", result);
			LogScreenMsg(msg, msg.GetLength());
		} else {
			done = true;
		}
	}

	return true;
}

// Powerup and unmute the codec
void CWM8731SoundController::enable(void)
{

    disable(); // disable first in case it was already powered up

    // if (Serial) { Serial.println("Enabling codec"); }
	LogScreen("Enabling codec in master mode\n");
    // if (m_wireStarted == false) {
    //     Wire.begin();
    //     m_wireStarted = true;
    // }
    // setOutputStrength();

    // Sequence from WAN0111.pdf
    // Begin configuring the codec
    resetCodec();
    delay(100); // wait for reset

    // Power up all domains except OUTPD and microphone
    regArray[WM8731_REG_POWERDOWN] = 0x12;
    write(WM8731_REG_POWERDOWN, regArray[WM8731_REG_POWERDOWN]);
    delay(100); // wait for codec powerup


    setAdcBypass(false); // causes a slight click
    setDacSelect(true);
    setHPFDisable(true);
    setLeftInputGain(0x17); // default input gain
    setRightInputGain(0x17);
    setLeftInMute(false); // no input mute
    setRightInMute(false);
    setDacMute(false); // unmute the DAC

    // link, but mute the headphone outputs
    regArray[WM8731_REG_LHEADOUT] = WM8731_LEFT_HEADPHONE_LINK_MASK;
    write(WM8731_REG_LHEADOUT, regArray[WM8731_REG_LHEADOUT]);      // volume off
    regArray[WM8731_REG_RHEADOUT] = WM8731_RIGHT_HEADPHONE_LINK_MASK;
    write(WM8731_REG_RHEADOUT, regArray[WM8731_REG_RHEADOUT]);

    /// Configure the audio interface
    write(WM8731_REG_INTERFACE, 0x42); // I2S, 16 bit, MCLK master
    regArray[WM8731_REG_INTERFACE] = 0x42;

    write(WM8731_REG_SAMPLING, 0x20);  // 256*Fs, 44.1 kHz, MCLK/1
    regArray[WM8731_REG_SAMPLING] = 0x20;
    delay(100); // wait for interface config

    // Activate the audio interface
    setActivate(true);
    delay(100);

    write(WM8731_REG_POWERDOWN, 0x02); // power up outputs
    regArray[WM8731_REG_POWERDOWN] = 0x02;
    delay(500); // wait for output to power up

    delay(100); // wait for mute ramp

}
