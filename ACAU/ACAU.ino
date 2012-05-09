/*
UserInputTest - Test the Library for talking to an RFID, keypad and LCD controller
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

#include <Wire.h>
#include <OpenAccessUser.h>
#include <avr/eeprom.h>

//#include <avr/wdt.h>

#define DS1307_I2C_ADDR 0x68

#define READER_I2C_ADDRESS 0x32
#define READER_RESET_PIN 4

#define EEPROM_I2C_ADDRESS 0x50

// RFID specific defines
#define RFID_CODE_LEN 10
#define PIN_CODE_LEN 4

#define CARD_DB_ADDRESS 0x0000          // 500 card codes
#define ACTION_DESCRIPTORS_ADDR 0x1580  // 256 action descriptors
#define AUDIT_REC_ADDRESS 0x2580        // 1447 16 byte audit records


OpenAccessUser reader1(8, READER_I2C_ADDRESS);

byte *dataIn;
bool done = false;

char tempBuffer[32];

byte loopCount = 0;

uint8_t second;
char timeBuffer[32];

// For reading battery voltage
long val;
float fVal;
float volts;

// Raw, low-level packet states
#define RCV_IDLE 0x00
#define RCV_BEGIN 0x01
#define RCV_TYPE 0x02
#define RCV_RECNUMH 0x03
#define RCV_RECNUML 0x04
#define RCV_DATA 0x05
#define RCV_CHKSUM 0x06

// Packet type codes
#define CMD_PING_REQ 0x00
#define CMD_PING_RESP 0x01
#define CMD_GEN_CMD 0x11
#define CMD_GEN_DEBUG 0x99 // General command to send debug text back to the DBS :-)
#define CMD_CHG_MODE 0x12
// Data modes
#define DATA_MODE_IDLE 0x00
#define DATA_MODE_CODES 0x01
#define DATA_MODE_ACTIONS 0x02
#define DATA_MODE_LCD_MSGS 0x03
#define DATA_MODE_AUDIT 0x04

#define CMD_DATA_PKT 0x20
#define CMD_NOP 0x30 // Placeholder for more commands
#define CMD_LAST 0x40 // Used for rudimentary validity check during receive

// Action types
#define OPEN_DOOR_ALWAYS 0x01
#define OPEN_DOOR_DATE_RANGE 0x02
#define OPEN_DOOR_LIMITED_USE 0x03
#define OPEN_DOOR_WEEKDAY_MASK 0x04
#define OPEN_DOOR_DBS_REQUEST 0x05
#define POWER_ON_EQUIPMENT 0x06
#define OTHER_ACTION 0x07
#define SOUND_ALARM 0x09

uint8_t rcvState = RCV_IDLE;
uint8_t rcvCount = 0;

uint8_t rcvCheckSum = 0;
int rcvRecNum;
uint8_t rcvSize;
uint8_t rcvType;
uint8_t rcvData[32];

uint8_t dataMode = DATA_MODE_IDLE;

bool txPingSent = false;
bool isUpDBS = false;
long pingTimeLast = 0;
long testTimeLast = 0;

uint8_t dateCurYear = 0;
uint8_t dateCurMonth = 0;
uint8_t dataCurDay = 0;
uint8_t dateCurWeekday = 0;
uint8_t dateCurHour = 0;
uint8_t dateCurMin = 0;
uint8_t dateCurSec = 0;

char wtf[32];

/////////////////////////////////////////////////////
// Initialize all systems
void setup()
{
	// Enable the watchdog timer for 2 seconds
	//wdt_enable(WDTO_2S);
	
	txPingSent = false;
	isUpDBS = false;
	pingTimeLast = 0;
	dataMode = DATA_MODE_IDLE;
	
	//Serial.begin(9600);
	Serial.begin(57600);
	reader1.begin();
	
	//reader1.resetUIC();
	
	
	//reader1.playTone(1);

	//burnEEPROMSample();

	//burnInternalEEPROM();

	//char wtf[32];
	//char *test;

//  	readStringFromEEPROM(0, wtf);
// 	Serial.println(wtf);
// 
//  	readStringFromEEPROM(1, wtf);
// 	Serial.println(wtf);
// 
//  	readStringFromEEPROM(2, wtf);
//  	Serial.println(wtf);

	done = false;


	
// 	Wire.beginTransmission(DS1307_I2C_ADDR);
// 	Wire.write((uint8_t)0x00);
// 	Wire.write(dec2bcd(00) | 0x80);   // set seconds (clock is stopped!)
// 	Wire.write(dec2bcd(55));           // set minutes
// 	Wire.write(dec2bcd(14) & 0x3f);      // set hours (24h clock!)
// 	Wire.write(dec2bcd(3+1));              // set dow (Day Of Week), do conversion from internal to RTC format
// 	Wire.write(dec2bcd(18));             // set day
// 	Wire.write(dec2bcd(4));            // set month
// 	Wire.write(dec2bcd(2012-2000));             // set year
// 	Wire.endTransmission();
// 
// 	Wire.beginTransmission(DS1307_I2C_ADDR);
// 	Wire.write((uint8_t)0x00);                      // Register 0x00 holds the oscillator start/stop bit
// 	Wire.endTransmission();
// 	Wire.requestFrom(DS1307_I2C_ADDR, 1);
// 	second = Wire.read() & 0x7f;       // save actual seconds and AND sec with bit 7 (sart/stop bit) = clock started
// 	Wire.beginTransmission(DS1307_I2C_ADDR);
// 	Wire.write((uint8_t)0x00);
// 	Wire.write((uint8_t)second);                    // write seconds back and start the clock
// 	Wire.endTransmission();
	
	//wdt_reset();
	loopCount = 0;
	
	// UGH! This doesn't work... >:-|
	//delay(5000);
	//reader1.writeLCD("SYSTEM READY");
	
	//Serial.println("ready!");
	
	//searchCardCode(1, "0000000000");
	
	rcvState = RCV_IDLE;
	rcvCount = 0;
	rcvCheckSum = 0;
}


void loop() {
	//wdt_reset();
	
	//reader1.writeLCD("SYSTEM READY");
	//Serial.println();
	//Serial.print("A");

	if( reader1.available() ) {
		reader1.getUserInput();
		//Serial.print("B");

		switch( reader1.getEventType() ) {
		case RESULT_REG_EVENT_CARDSWIPED:
			//Serial.println("YIPPEEEE a card!");
			sendDebugMessage( "YIPPEEEE a card!" );
			reader1.getCard(tempBuffer);
			tempBuffer[RFID_CODE_LEN] = 0;
			
			//Serial.print("C");
			searchCardCode( 1, tempBuffer);
			//Serial.print("D");
			break;
			
		case RESULT_REG_EVENT_PINENT:
			//Serial.println("YIPPEEEE a PIN!");
			reader1.getPIN(tempBuffer);
			tempBuffer[PIN_CODE_LEN] = 0;
			
			searchCardCode( 2, tempBuffer);
			break;
			
		default:
			break;
		}
	}
	
	if( Serial.available() ) {
		// Call to state-machine driven packet parser
		processPacket();
	}

	
	if( (loopCount % 10000) == 0 ) {
		Wire.beginTransmission(DS1307_I2C_ADDR);
		Wire.write((uint8_t)0);
		Wire.endTransmission();
	
		int timePos = 0;
		
		Wire.requestFrom(DS1307_I2C_ADDR, 8);
		while (Wire.available()) {
			tempBuffer[timePos++] = Wire.read();
			//outhex8(Wire.read());
			//Serial.print(" ");
		}
		
		//sprintf(timeBuffer, "%02X/%02X/20%02X %02X:%02X:%02X\n", tempBuffer[5], tempBuffer[4], tempBuffer[6], tempBuffer[2], tempBuffer[1], tempBuffer[0]);
		
		dateCurYear = tempBuffer[6];
		dateCurMonth = tempBuffer[5];
		dataCurDay = tempBuffer[4];
		dateCurWeekday = tempBuffer[3];
		dateCurHour = tempBuffer[2];
		dateCurMin = tempBuffer[1];
		dateCurSec = tempBuffer[0];
		
		//reader1.writeLCD(timeBuffer);
		
		//Serial.print(timeBuffer);
		//Serial.print(" -- ");
		
		val = analogRead(0);
		fVal = val * 27.72f;
		volts = fVal / 1024;
		
		//Serial.print(volts);
		//Serial.println(" volts");
		char charBuf[10];
		char voltBuf[15];
		
		dtostrf(volts, 5, 1, charBuf);
		
		sprintf(voltBuf, "%s volts\n", charBuf);
		
		
		sprintf(timeBuffer, "%02X/%02X/20%02X %02X:%02X:%02X\n", tempBuffer[5], tempBuffer[4], tempBuffer[6], tempBuffer[2], tempBuffer[1], tempBuffer[0]);
		
		if( (millis() - pingTimeLast > 500) && txPingSent ) {
			isUpDBS = false;
			txPingSent = false;
		}
		
		if( millis() - testTimeLast > 1000 ) {
			//sendDebugMessage( timeBuffer );
			sendDebugMessage( voltBuf );
			// Send pings continuously
// 			if( !txPingSent ) {
// 				sendDBSPing(CMD_PING_REQ);
// 				pingTimeLast =  millis();
// 			}

			readStringFromEEPROM(0, wtf);
			wtf[31] = 0;
			//sendDebugMessage(wtf);
			
			readStringFromEEPROM(1, wtf);
			wtf[31] = 0;
			//sendDebugMessage(wtf);
			
			readStringFromEEPROM(2, wtf);
			wtf[31] = 0;
			//sendDebugMessage(wtf);
			
			testTimeLast = millis();
		}
		
		// Monitor DBS health here.
		if( isUpDBS ) {
			digitalWrite(13, HIGH);
		} else {
			digitalWrite(13, LOW);
		}
		
		// Send pings continuously
		if( !txPingSent ) {
			sendDBSPing(CMD_PING_REQ);
			pingTimeLast =  millis();
		}
	}
	
	//digitalWrite(13, HIGH);
	//delay(3000);
  
// This code dumps the contents of the 32K EEPROM to Serial
//   if( !done ) {
//     Wire.beginTransmission(EEPROM_I2C_ADDRESS);
//     Wire.write((byte)(0 >> 8)); // MSB
//     Wire.write((byte)(0 & 0xFF)); // LSB
//     Wire.endTransmission();
// 
//     for( int j=0; j < 10; j++) {
//      //i2c_eeprom_write_page(EEPROM_I2C_ADDRESS, j*16, dataIn, 16); 
//       Wire.requestFrom(EEPROM_I2C_ADDRESS, 11);    // request 6 bytes from slave device #2
// 
//       while(Wire.available())    // slave may send less than requested
//       { 
//         char c = Wire.read(); // receive a byte as character
//         //Serial.print(c);         // print the character
//         outhex8(c);
//       }
//       
//       Serial.println();
//     }
//     
//     done = true;
// 	while( 1) {}
//   }
	loopCount++;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//                                  SUBROUTINES
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t dec2bcd(uint8_t num)
{
	return ((num/10 * 16) + (num % 10));
}



// !
void searchCardCode( int codeType, char *cardCode ) {
  char buffer[RFID_CODE_LEN+2];
  
  sendStartEEPROMAddr(CARD_DB_ADDRESS);
  
  // TODO: CHANGE THIS TO A FOREACH-TYPE LOOP (with data end delimeter or something)
  for( int k = 0; k < 5; k++ ) {
    Wire.requestFrom(EEPROM_I2C_ADDRESS, RFID_CODE_LEN+1); // Request a code record from the EEPROM
    
    for ( int c = 0; c < RFID_CODE_LEN+1; c++ ) {
      if (Wire.available()) buffer[c] = Wire.read();
    }
	
	//                         1         2         3
	//                12345678901234567890123456789012
	//reader1.writeLCD("This is a test of the LCD thing.");
	//reader1.writeLCD("ABCDEFGHIJKLMNOPQRSTUVWXYZ123456");
	//reader1.writeLCD("This is some text.");
	
	//Serial.print("WTF: ");
	//Serial.println(buffer);
    
    
	if( codeType == 1 ) {
      // Now compare the strings... if found, then signal the go ahead to open the door
      if( strncmp(cardCode, &buffer[1], RFID_CODE_LEN) == 0 ) {
        // Found the code in the DB
		//Serial.println("YAY! (FounD CARD)");
		outhex8( buffer[0] ); // Action code
		Serial.println(); 
		
		// Look-up and process appropriate action descriptor
		performAction( buffer[0] );
		
		reader1.playTone(1);
		//reader1.writeLCD("This is a test of the LCD thing.");
		reader1.writeLCD(cardCode);
		//reader1.setLED(1);
		//delay(250);
		//reader1.setLED(0);
		
        return;
      }
    }
    
	if( codeType == 2 ) {
      // Now compare the strings... if found, then signal the go ahead to open the door(or whatever)
      if( strncmp(cardCode, &buffer[1], PIN_CODE_LEN) == 0 ) {
        // Found the code in the DB
        //Serial.println("YAY! (FounD PIN)");
		outhex8( buffer[0] ); 
		
		// Look-up and process appropriate action descriptor
		performAction( buffer[0] );
		
		reader1.playTone(1);
		
        return;
      }
    }
  }
  
  // Didn't find any matching code
  reader1.playTone(3); // BOOOP!
  //delay(600);
  //reader1.writeLCD("ERROR! Try Again.");
  //Serial.println("WTF");
  
}


void sendStartEEPROMAddr( int dataStoreAddr ) {
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  Wire.write((byte)(dataStoreAddr >> 8)); // MSB
  Wire.write((byte)(dataStoreAddr & 0xFF)); // LSB
  Wire.endTransmission();
}


// WARNING: address is a page address, 6-bit end will wrap around
  // also, data can be maximum of about 30 bytes, because the Wire library has a buffer of 32 bytes
  void i2c_eeprom_write_page( int deviceaddress, unsigned int eeaddresspage, byte* data, byte length ) {
    byte c;
    
    if( length > 30 ) {
       return; 
    }
    
    Wire.beginTransmission(deviceaddress);
    Wire.write((byte)(eeaddresspage >> 8)); // MSB
    Wire.write((byte)(eeaddresspage & 0xFF)); // LSB
    
    for ( c = 0; c < length; c++) {
      Wire.write(data[c]);
    }
    
    Wire.endTransmission();
    
    delay(10);
  }
  
  
  int getCurrentDateTime() {
	Wire.beginTransmission(DS1307_I2C_ADDR);
	Wire.write((uint8_t)0);
	Wire.endTransmission();

	int timePos = 0;

	Wire.requestFrom(DS1307_I2C_ADDR, 8);
	while (Wire.available()) {
		tempBuffer[timePos++] = Wire.read();
	}

	dateCurYear = tempBuffer[6];
	dateCurMonth = tempBuffer[5];
	dataCurDay = tempBuffer[4];
	dateCurWeekday = tempBuffer[3];
	dateCurHour = tempBuffer[2];
	dateCurMin = tempBuffer[1];
	dateCurSec = tempBuffer[0];
	
	return( 1 );
  }
  
  
  // Compare with date and time in dateCurXXX
  int compareTime( uint8_t hour, uint8_t min, uint8_t sec ) {
	return 0;
  }
  
  int compareDate( uint8_t year, uint8_t month, uint8_t day ) {
	return 0;
  }
  
  int compareDateTime( uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec ) {
	return 0;
  }


///////////////////////////////////////////////////////////////////////////////////////////////

// Helper functions for debugging

void outhex8(uint8_t ch)
{
  hexalpha(ch >> 4);
  hexalpha(ch & 0x0f);
}

void hexalpha(uint8_t ch)
{
  static char hexchars[] = "0123456789ABCDEF";
  Serial.print(hexchars[ch]);
}


// 
void burnEEPROMSample() {
 //byte cardNo2[] = {0x01, 0x30, 0x34, 0x30, 0x30, 0x31, 0x42, 0x34, 0x36, 0x30, 0x39};
 byte cardNo2[] = "\001" "04001B4609";
 byte cardNo1[] = {0x01, 0x30, 0x46, 0x30, 0x33, 0x30, 0x32, 0x39, 0x34, 0x43, 0x30};
 byte cardNo3[] = {0x02, 0x35, 0x36, 0x39, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
 byte cardNo4[] = {0x03, 0x31, 0x32, 0x33, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
 
 i2c_eeprom_write_page(EEPROM_I2C_ADDRESS, (unsigned int)0, cardNo1, 11);
 i2c_eeprom_write_page(EEPROM_I2C_ADDRESS, (unsigned int)11, cardNo2, 11);
 i2c_eeprom_write_page(EEPROM_I2C_ADDRESS, (unsigned int)22, cardNo3, 11);
 i2c_eeprom_write_page(EEPROM_I2C_ADDRESS, (unsigned int)33, cardNo4, 11);
 
 // Each action desriptor is 16 bytes in length
 //----------------------------------------------
 // OPEN_DOOR_ALWAYS -- Just open the damn door! :-)
 byte action01[] = { OPEN_DOOR_ALWAYS,                   // Action type Always
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // blank
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // blank
					0x00,                                // blank
					0x03,                                // success LCD message id
					0x04};                               // Fail LCD message id (only fails if door can't be opened?)

// OPEN_DOOR_DATE_RANGE and OPEN_DOOR_LIMITED_USE
// Date range and use count (ex. unlimited uses on Apr 10, 2012 from 7:30pm to 9:30pm)
byte action02[] = { OPEN_DOOR_DATE_RANGE,                // Action type Range_Use
					0x12, 0x04, 0x10, 0x19, 0x30, 0x00,  // Valid from
					0x12, 0x04, 0x10, 0x21, 0x30, 0x00,  // Expires at
					0xff,                                // Num uses (0xff = unlimited)
					0x05,                                // success LCD message id
					0x06};                               // Fail LCD message id
					
// OPEN_DOOR_WEEKDAY_MASK -- Weekday filter (ex. Mon, Wed, Fri 3:00 to 9:00)
byte action03[] = { OPEN_DOOR_WEEKDAY_MASK,              // Action type Weekday
					B00101010,                           // Weekday mask (msb not used) (ex. M, W, F)
					0x15, 0x00, 0x00,                    // Start time (ex. 3:00)
					0x21, 0x00, 0x00,                    // End time (ex. 9:00)
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // blank
					0x07,                                // success LCD message id
					0x08};                               // Fail LCD message id
					
// OPEN_DOOR_DBS_REQUEST -- Modular DBS query system
byte action04[] = { OPEN_DOOR_DBS_REQUEST,                    // Action type DBS Request
					0x20,                                     // Header Byte -- High order nybble has module number
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Seven bytes of request data
					0x00, 0x00, 0xff, 0x01,                   // Four byte Reply match
					0x00,                                     // blank
					0x09,                                     // success LCD message id
					0x0a};                                    // Fail LCD messa

byte action05[] = { OPEN_DOOR_DBS_REQUEST,                    // Action type DBS Request
					0x10,                                     // Header Byte -- High order nybble has module number
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Seven bytes of request data
					0x00, 0x00, 0xfe, 0xfe,                   // Four byte Reply match
					0x00,                                     // blank
					0x0b,                                     // success LCD message id
					0x0c};                                    // Fail LCD messa
}



void burnInternalEEPROM() {
	//                       1         2         3
	//              12345678901234567890123456789012
	//             |               |               |
	char msg1[] =  "ABCD                            ";
	char msg2[] =  "04-12   13:12:37                ";
	char msg3[] =  "test2                           ";
	char msg4[] =  "error                           ";
	
	writeStringToEEPROM(0, msg1);
	writeStringToEEPROM(1, msg2);
	writeStringToEEPROM(2, msg3);
	writeStringToEEPROM(3, msg4);
}


// TODO: Aaaaaaaargh! FIX THIS!!!!!!!!!! (p.s. use eeprom_write_block(...32)
void writeStringToEEPROM( int stringNumber, char *stringToWrite ) {
	uint8_t address = stringNumber * 32;
	
// 	while( *stringToWrite ) {
// 		eeprom_write_byte((unsigned char *) address++, *stringToWrite++);
// 	}
// 	
// 	eeprom_write_byte((unsigned char *) address, 0);
	
	eeprom_write_block((const void*)stringToWrite, (void*)address, 32);
}



void readStringFromEEPROM( int stringNumber, char *stringToRead) {
	uint8_t address = stringNumber * 32;
	
	eeprom_read_block((void*)stringToRead, (const void*)address, 32);
}


void processPacket() {
	uint8_t rcvByte = (uint8_t)Serial.read();
	rcvCheckSum += rcvByte;
	
	switch(rcvState) {
		case RCV_IDLE:
			// read preamble byte, get a "lock" on the packet
			if( rcvByte == 0xA5 ) {
				rcvState = RCV_BEGIN;
			}
			
			break;
		
		case RCV_BEGIN:
			rcvType = rcvByte;
			
			if( rcvType < CMD_LAST ) {
				rcvState = RCV_TYPE;
			} else {
				// Bail! this is not a valid packet.
				rcvState = RCV_IDLE;
				rcvCheckSum = 0;
			}
			
			break;
			
		case RCV_TYPE:
                    // We now have the packet type now, read the size
                    rcvSize = rcvByte;

                    if( rcvSize < 33 ) {
                        // Now we have the size of the data
                        // if it is zero then it changes the way we read the rest of the packet
                        if( rcvSize > 0 ) {
                            rcvState = RCV_RECNUMH;
                        } else {
                            rcvState = RCV_CHKSUM;
                        }
                    } else {
                            // This looks to be a bad packet
                            rcvState = RCV_IDLE;
                            rcvCheckSum = 0;
                    }

                    break;

            case RCV_RECNUMH:
                    rcvRecNum = rcvByte << 8;
                    rcvState = RCV_RECNUML;
                    break;

            case RCV_RECNUML:
                    rcvRecNum |= rcvByte;
                    rcvState = RCV_DATA;
                    rcvCount = 0; // This is important!
                    break;

            case RCV_DATA:
                    // Now we should be receiving data bytes
                    // Count them off using rcvSize
                    rcvData[rcvCount++] = rcvByte;

                    if( rcvCount >= rcvSize ) {
                        rcvState = RCV_CHKSUM;
                    }

                    break;
		
		case RCV_CHKSUM:
			// This must be the checksum byte
			// If all goes well here, we can process the packet's contents
			if( rcvCheckSum == 0 ) {
				// All good, go ahead and use this data/command
				// TODO: Do we send an ACK?
				
				rcvState = RCV_IDLE;
				rcvCheckSum = 0;
				
				processCommand();
			} else {
				// This is the end-of-the-line 
				// Bad packet
				// TODO: Do we send a NACK
				rcvState = RCV_IDLE;
				rcvCheckSum = 0;
			}
			
			break;
		
		default:
			// Don't know what happened here buuuut, lets get right
			rcvState = RCV_IDLE;
			rcvCheckSum = 0;
			break;
	}
}



void processCommand() {
	int thisCommand = 0;
	int address = 0;
	
	switch( rcvType ) {
		case CMD_PING_REQ:
			// return a ping reply here
			sendDBSPing(CMD_PING_RESP);
			//isUpDBS = false;
			break;
		
		case CMD_PING_RESP:
			// Check if this is a valid response (did we send a ping request?)
			if( txPingSent ) {
				isUpDBS = true;
				txPingSent = false;
				//digitalWrite(13, HIGH);
			}
			
			break;
		
		case CMD_GEN_CMD:
			thisCommand = rcvData[0];
			
			if( thisCommand == 0 ) {
				break;
			}
			
			if( thisCommand == 1 ) {
				break;
			}
			
			break;
		
		case CMD_CHG_MODE:
			// Process a mode change here
			// Mode to change to is in rcvData[0]
			
			// TODO: Stop blabbing about it and do it
			sendDebugMessage("CMD_CHG_MODE\n");
			
			// Valid Modes
			// ===========
			switch( rcvData[0] ) {
				// DATA_MODE_IDLE
				// In Idle Mode,the ACAU can send asynchronous information requests to the DBS 
				// It could also be called "normal mode" or something like that. :-)
				case DATA_MODE_IDLE: 
					dataMode = DATA_MODE_IDLE;
					break;
				
				// DATA_MODE_CODES
				// Codes Mode is to allow the DBS to send a new list of 
				// valid codes (RFID code or PIN code) and an action code for each record
				case DATA_MODE_CODES: 
					dataMode = DATA_MODE_CODES;
					break;
				
				// DATA_MODE_ACTIONS
				// This is data that describes actions. These descriptors can define how
				// the code number match is handled. 
				case DATA_MODE_ACTIONS: 
					dataMode = DATA_MODE_ACTIONS;
					break;
				
				// DATA_MODE_LCD_MSGS
				// this mechanism allows the DBS to directly engage the user outside the door at the UIC.
				case DATA_MODE_LCD_MSGS: 
					dataMode = DATA_MODE_LCD_MSGS;
					break;
				
				// DATA_MODE_AUDIT
				// This data mode is for the DBS to have access to the audit logs taken by the ACAU.
				// This audit data is then matched to existing data in DBS and new information added
				// to the DBS records.
				case DATA_MODE_AUDIT: 
					dataMode = DATA_MODE_AUDIT;
					break;
			}
			
			break;
		
		case CMD_DATA_PKT:
			// Copy the data record in rcvData[] to the correct area (internal EEPROM, external, etc)
			// depending on the current data mode.
			// If data received in Idle Mode then there is an error
			switch( dataMode ) {
				case DATA_MODE_IDLE: 
					// Error! We got qa data packet in idle mode
					// Do nothing with the data
					break;
				
				case DATA_MODE_CODES: 
					// Write data to CARD_DB_ADDRESS
					address = (rcvRecNum * 11) + CARD_DB_ADDRESS;
					i2c_eeprom_write_page(EEPROM_I2C_ADDRESS, (unsigned int)address, rcvData, 11);
					//sendDebugMessage("DATA_MODE_CODES!\n");
					break;
				
				case DATA_MODE_ACTIONS: 
					// Write data to ACTION_DESCRIPTORS_ADDR
					address = (rcvRecNum * 16) + ACTION_DESCRIPTORS_ADDR;
					i2c_eeprom_write_page(EEPROM_I2C_ADDRESS, (unsigned int)address, rcvData, 16);
					sendDebugMessage("DATA_MODE_ACTIONS!\n");
					break;
				
				case DATA_MODE_LCD_MSGS: 
					// Write data to local EEPROM
					// The first 32 bytes of internal EEPROM is reserved
					address = rcvRecNum;
					writeStringToEEPROM(address, (char*)rcvData);
					sendDebugMessage("DATA_MODE_LCD_MSGS!\n");
					
// 					char textMsg[64];
// 					sprintf(textMsg, "<%d>\n", address);//(char*)rcvData, address);
// 					textMsg[rcvSize+2] = 0; // null terminate
// 					
// 					sendDebugMessage( textMsg );
					break;
				
				// NOTE: This is not used in the ACAU. The DBS only reads audit data
				case DATA_MODE_AUDIT: 
					// Read from AUDIT_REC_ADDRESS
					// Do nothing with the data
					break;
			}
			
			break;
		
		default:
			// Handle unknown data mode specifications here. Break in error?
			break;
			
	}
}

// performAction -- A function to "do stuff". After a code presented by the user
// is matched, there is a corresponding action code given which determines what 
// the access control system will do with the code match. For example: the action 
// could be as simple as "open the door" or as complex as asking the server if
// there is a certain other person or piece of equipment present. Also, this 
// mechanism can be extended to determining if a person is authorized to use a 
// power tool or other resource by keeping certification and/or payment records.
int performAction( uint8_t actionIndex ) {
	char buffer[16];

	sendStartEEPROMAddr(ACTION_DESCRIPTORS_ADDR + (16 * actionIndex));

	Wire.requestFrom(EEPROM_I2C_ADDRESS, 16); // Request a code record from the EEPROM

	for ( int c = 0; c < 16; c++ ) {
		if (Wire.available()) buffer[c] = Wire.read();
	}
	
	int actionType = buffer[0];
	
	switch( actionType ) {
		case OPEN_DOOR_ALWAYS:
			// Yep! Here is where the door is actually opened. The moment you've all been waiting for.
			//openDoor();
			break;
		
		case OPEN_DOOR_DATE_RANGE:
		case OPEN_DOOR_LIMITED_USE:
			
			break;
		
		case OPEN_DOOR_WEEKDAY_MASK:
			if( getCurrentDateTime() ) {
				if( (buffer[1] & dateCurWeekday) 
					&& compareTime(buffer[2], buffer[3], buffer[4]) > 0
					&& compareTime(buffer[5], buffer[6], buffer[7]) < 0) {
					// Open the door
					//openDoor();
				}
			}
			
			break;
		
		case OPEN_DOOR_DBS_REQUEST:
			// This is the "can of worms" modular system
			
			break;
		
		case POWER_ON_EQUIPMENT:
			
			break;
		
		case SOUND_ALARM:
			
			break;
		
		default: // Invalid action given, What's going on here?!?
			break;
	}
}


// DBS --> ACAU: A5 00 00 5B = Ping request
// ACAU --> DBS: A5 01 00 5A = Ping response
int sendDBSPing(uint8_t pingType) {
	uint8_t packetToSend[4];
	
	if( pingType == CMD_PING_REQ ) {
		packetToSend[0] = 0xA5;
		packetToSend[1] = 0x00;
		packetToSend[2] = 0x00;
		packetToSend[3] = 0x5B;
		
		Serial.write(packetToSend, 4);
		txPingSent = true;
	} else {
		packetToSend[0] = 0xA5;
		packetToSend[1] = 0x01;
		packetToSend[2] = 0x00;
		packetToSend[3] = 0x5A;
		
		Serial.write(packetToSend, 4);
	}
}



void writePacket(uint8_t packetType, uint8_t size, uint8_t* data, int recnum) {
	uint8_t packetToSend[64];
	uint8_t checksum = (uint8_t)0xA5 + packetType + size;
	
	packetToSend[0] = 0xA5; // First things first
	packetToSend[1] = packetType;
	packetToSend[2] = size;
	
	if( size > 0 ) {
		packetToSend[3] = (uint8_t)((recnum & 0xFF00) >> 8);
		packetToSend[4] = (uint8_t)(recnum & 0x00FF);
		checksum += (packetToSend[3] + packetToSend[4]); 
		
		for( int i = 0; i < size; i++ ) {
			packetToSend[5 + i] = data[i];
			checksum += data[i];
		}
	}
	
	packetToSend[5 + size] = (uint8_t)(-checksum);
	
	Serial.write( packetToSend, 6 + size );
	
}




void sendDebugMessage(char* text) {
	uint8_t textToSend[48];
	
	// Prepend the general command byte CMD_GEN_DEBUG indicating the transfer of debug text
	textToSend[0] = CMD_GEN_DEBUG;
	
	uint8_t *ptr = &textToSend[1]; // Now the rest is the text
	uint8_t *textin = (uint8_t*)text;
	
	while( *ptr++ = *textin++ ) {} // TODO: Change this to a maximum limit. In case there is no null terminator. strncpy?
	
	writePacket( CMD_GEN_CMD, strlen(text) + 1, textToSend, 0 ); // TODO: change this to indicate how many bytes were copied. See above.
}











// This code dumps the contents of the 32K EEPROM to Serial
//  if( !done ) {
//    Wire.beginTransmission(EEPROM_I2C_ADDRESS);
//    Wire.write((byte)(0 >> 8)); // MSB
//    Wire.write((byte)(0 & 0xFF)); // LSB
//    Wire.endTransmission();
//
//    for( int j=0; j < 2048; j++) {
//     //i2c_eeprom_write_page(EEPROM_I2C_ADDRESS, j*16, dataIn, 16); 
//      Wire.requestFrom(EEPROM_I2C_ADDRESS, 16);    // request 6 bytes from slave device #2
//
//      while(Wire.available())    // slave may send less than requested
//      { 
//        char c = Wire.read(); // receive a byte as character
//        //Serial.print(c);         // print the character
//        outhex8(c);
//      }
//      
//      Serial.println();
//    }
//    
//    done = true;
//  }
