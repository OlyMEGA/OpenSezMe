/*
OpenAccessUser.cpp - Library for talking to an RFID, keypad and LCD controller
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


#include "Arduino.h"
#include "OpenAccessUser.h"
#include "wiring_private.h"

volatile int rxState = 0; // Gotta start somewhere..

volatile unsigned long transitionTimeLast = 0;
volatile unsigned long transitionPeriod = 0;

volatile bool dataAvailable;




////////////////////////////////////////////////////////////
// Constructor
OpenAccessUser::OpenAccessUser( int pinReset, int i2cAddr ) {
	pinMode(pinReset, OUTPUT);
	_pinReset = pinReset;
	_slaveAddress = i2cAddr;
	
	dataAvailable = false;
	
	_displayData.Control1 = 0x0F;
	_displayData.Control2 = 0x00;
	
	// Setup an interrupt on pin 2
#if defined(EICRA) && defined(ISC00) && defined(EIMSK)
	EICRA = (EICRA & ~((1 << ISC00) | (1 << ISC01))) | (FALLING << ISC00); // Mode = ? (FALLING)
	EIMSK |= (1 << INT0);
#endif
	
	rxState = 0;
}


////////////////////////////////////////////////////////////
// 
void OpenAccessUser::begin() {
	Wire.begin();
	
	// Pull slave reset(s) high (taking them out of reset)
	digitalWrite( _pinReset, HIGH );
	
	//Serial.println("Starting...");
}


////////////////////////////////////////////////////////////
// 
bool OpenAccessUser::available() {
	return dataAvailable;
}


////////////////////////////////////////////////////////////
// 
void OpenAccessUser::getUserInput() {
	uint8_t tempData2[LCD_MAX_CHARS];
	int rcvCount = 0;
	uint8_t *address = (uint8_t *)&_userData; // Get a pointer to a block of bytes where the structure resides
	int dataSize = sizeof(RESULT_REGISTERS);
	
	Wire.requestFrom(_slaveAddress, dataSize);
	
	while(Wire.available())
	{
		unsigned char c = Wire.read();
		tempData2[rcvCount++] = c;
	}
	
	// Copy it to our structure
	memcpy(address, tempData2, dataSize);
	
	dataAvailable = false;
}


////////////////////////////////////////////////////////////
// 
int OpenAccessUser::getEventType() {
	return _userData.Status & RESULT_REG_EVENT_MASK;
}


////////////////////////////////////////////////////////////
// 
void OpenAccessUser::getCard(char *bufferIn) {
	memcpy(bufferIn, _userData.CardNum, RFID_CODE_LEN);
}


////////////////////////////////////////////////////////////
// 
void OpenAccessUser::getPIN(char *bufferIn) {
	memcpy(bufferIn, _userData.PINCode, PIN_CODE_LEN);
}


////////////////////////////////////////////////////////////
// 
void OpenAccessUser::setLED(bool onOff) {
	if( onOff ) {
		_displayData.Control2 = _displayData.Control2 | COMMAND_REG_CTL2_SWITCH;
	} else {
		_displayData.Control2 = _displayData.Control2 & !COMMAND_REG_CTL2_SWITCH;
	}
	
	sendCommand();
}


////////////////////////////////////////////////////////////
// 
void OpenAccessUser::refreshLCD() {
	_displayData.Control2 = _displayData.Control2 | COMMAND_REG_CTL2_SHWLCD;
	
	sendCommand();
}


////////////////////////////////////////////////////////////
// 
void OpenAccessUser::playTone(byte toneCode) {
	_displayData.Control1 = (_displayData.Control1 & 0x0f) | ((toneCode & 0x0f) << 4);
	
	sendCommand();
	
	delay(250); // Wait for tone sequence to end?
	
	// Turn off the noise 
	_displayData.Control1 = _displayData.Control1 & 0x0F;
}


////////////////////////////////////////////////////////////
// 
void OpenAccessUser::setLCDBright(byte brtLevel) {
	_displayData.Control1 = (_displayData.Control1 & 0xf0) | (brtLevel & 0x0f);
	
	sendCommand();
}


////////////////////////////////////////////////////////////
// 
void OpenAccessUser::resetUIC() {
	digitalWrite( _pinReset, LOW );
	delay( 100 );
	digitalWrite( _pinReset, HIGH );
}



////////////////////////////////////////////////////////////
// 
void OpenAccessUser::writeLCD(char *lcdTextIn) {
	uint8_t lcdBuffer[25];
	int len;
	
	if( (len = strlen(lcdTextIn)) > 16 ) {
		lcdBuffer[0] = 0x12; // Offset byte
		
		memcpy((void *)&lcdBuffer[1], (const void *)&lcdTextIn[16], len - 16);
		
		Wire.beginTransmission(_slaveAddress);
		Wire.write(lcdBuffer, len - 15);
		Wire.endTransmission();
	}
	
	lcdBuffer[0] = 0x00; // Offset byte
	lcdBuffer[1] = (uint8_t)_displayData.Control1; // 
	lcdBuffer[2] = (uint8_t)(_displayData.Control2 | COMMAND_REG_CTL2_SHWLCD); // 0x02=showLCD text 
	
	memcpy((void *)&lcdBuffer[3], (const void *)lcdTextIn, 16);
	
	Wire.beginTransmission(_slaveAddress);
	Wire.write(lcdBuffer, 19);
	Wire.endTransmission();
}


////////////////////////////////////////////////////////////
// 
void OpenAccessUser::sendCommand() {
	uint8_t cmdBuffer[4];
	
	cmdBuffer[0] = 0x00; // Offset byte
	cmdBuffer[1] = (uint8_t)_displayData.Control1;
	cmdBuffer[2] = (uint8_t)_displayData.Control2;

	Wire.beginTransmission(_slaveAddress);
	Wire.write(cmdBuffer, 3);
	Wire.endTransmission();
}

////////////////////////////////////////////////////////////
// Interrupt service routine
ISR(INT0_vect) {
	dataAvailable = true;
}






