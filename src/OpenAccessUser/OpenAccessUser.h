/*
OpenAccessUser.h - Library for talking to an RFID, keypad and LCD controller
Copyright (C) 2012  OlyMEGA -- Keith Youngblood

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OpenAccessUser_h
#define OpenAccessUser_h

#include "Arduino.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <avr/io.h>
#include <Wire.h>

#include "avr/interrupt.h"

// RFID specific defines
#define RFID_CODE_LEN 10
#define PIN_CODE_LEN 4

#define LCD_COLS 16
#define LCD_ROWS 2
#define LCD_MAX_CHARS 32




/////////////////////////////////////////////////////
// I2C slave result registers
// This is the data that we as the master reads
#define RESULT_REG_STATUS_SWITCH 0x80  ' B10000000

#define ON   0x1
#define OFF  0x0

#define RESULT_REG_EVENT_MASK 0x03

#define RESULT_REG_EVENT_STATUS 0
#define RESULT_REG_EVENT_CARDSWIPED 1
#define RESULT_REG_EVENT_PINENT 2
#define RESULT_REG_EVENT_TAMPER 3

struct RESULT_REGISTERS{
	byte Status;
	char CardNum[RFID_CODE_LEN];
	char PINCode[PIN_CODE_LEN];
	char SerialNum[11];
};


/////////////////////////////////////////////////////
// I2C slave command registers
// Commands are executed when these registers are 
// written to by the I2C master (Door control)
#define COMMAND_REG_CTL2_SWITCH 0x80
#define COMMAND_REG_CTL2_SHWLCD 0x02

struct COMMAND_REGISTERS{
	uint8_t Control1;
	uint8_t Control2;
	char DisplayText[LCD_MAX_CHARS];
};


class OpenAccessUser
{
  public:
	OpenAccessUser( int pinReset, int i2cAddr);
	void begin();
	bool available();
	int getEventType();
	void getCard(char *bufferIn);
	void getPIN(char *bufferIn);
	void getUserInput();
	void writeLCD(char *lcdText);
	
	void setLED(bool onOff);
	void refreshLCD();
	void playTone(byte toneCode);
	void setLCDBright(byte brtLevel);
	void resetUIC();
	
  private:
	int _pinReset;
	int _slaveAddress;
	bool _dataAvailable;
	
	void sendCommand();

	// I2C requested data
	RESULT_REGISTERS _userData;

	// I2C received data
	COMMAND_REGISTERS _displayData;
};

#endif
