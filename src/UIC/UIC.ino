/*
OpenSezMe - User Interface Console (UIC)
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

#define SERIAL_NUM "123456789AB"

#include <SoftwareSerial.h>
#define RxPin 8
#define TxPin 9 // possible contention here with sofware serial lib

// include the library code:
#include <LiquidCrystal.h>

#include <Wire.h>

//define slave i2c address
#define I2C_SLAVE_ADDRESS 0x32

#include <Keypad.h>
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {5, 4, 3, 2}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {A0, A1, A2, A3}; //connect the columns of the keypad to the analog port as digital input

#define KEYPAD_CODE_EXPIRE_TIME 1000  // Time-out for partial access (PIN) codes entered

// Define some LCD stuff
#define LCD_BL_MAX 255

// Pins used
#define LCD_BL_DIM 9
#define LCD_ENABLE 11
#define LCD_RS 12
#define LCD_D7 2
#define LCD_D6 3
#define LCD_D5 4
#define LCD_D4 5

#define LCD_COLS 16
#define LCD_ROWS 2
#define LCD_MAX_CHARS 32 // TDOD: multiply the above nums

// More pins
#define RFID_EN 6
#define AUDIO_OUT 10
#define LED_OUT 13
#define IRQ_OUT 7

// RFID specific defines
#define RFID_CODE_LEN 10
#define PIN_CODE_LEN 4

#define RFID_WAIT_FOR_START 0
#define RFID_READING_DATA 1

// Delay's in ms
#define CARD_VERIFY_INTERVAL 700
#define CARD_DELAY_INTERVAL 1000

#define RFID_CARD_WAIT 0
#define RFID_CARD_VERIFY 1
#define RFID_CARD_SENT 2

/////////////////////////////////////////////////////
// instantiate some global variables
char cardStr[RFID_CODE_LEN + 1]; 
int cardDataCount = 0;

int cardReadStatus = RFID_CARD_WAIT;
long timeLast = 0;
//long timeWait = 0;
long timeDiff = 0;
char codeLast[RFID_CODE_LEN + 1];

char lastKey = NO_KEY;
long timeLastKey = 0;
byte buildCount = 0;

int incomingByte = 0;   // for incoming serial data

byte RFID_ReadMode = RFID_WAIT_FOR_START;

/////////////////////////////////////////////////////
// I2C slave result registers
// This is the data that the master reads
#define RESULT_REG_STATUS_SWITCH 0x80

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

// I2C requested data
RESULT_REGISTERS userData;

/////////////////////////////////////////////////////
// I2C slave command registers
// Commands are executed when these registers are 
// written to by the I2C master (Door control)
#define COMMAND_REG_CTL2_SWITCH 0x80
#define COMMAND_REG_CTL2_SHWLCD 0x02

struct COMMAND_REGISTERS{
  uint8_t Control1;
  uint8_t Control2;
  char DisplayText[LCD_MAX_CHARS + 1]; // Add one for null termination
};

// I2C received data
COMMAND_REGISTERS displayData;

// RFID reader SOUT pin connected to Serial RX pin at 2400bps to pin8
SoftwareSerial RFID = SoftwareSerial(RxPin,TxPin); 

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(LCD_RS, LCD_ENABLE, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// Initialize keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

/////////////////////////////////////////////////////
// Initialize all systems
void setup()
{ 
  pinMode(LED_OUT, OUTPUT);       // Set digital pin 13 as OUTPUT for LED
  digitalWrite(LED_OUT, LOW);    // Pin 13 LED OFF
  
  pinMode(IRQ_OUT, OUTPUT);       // This IRQ line goes to the master to tell of events that occurred
  digitalWrite(IRQ_OUT, HIGH);    // It is active-low
  
  memcpy(userData.SerialNum, SERIAL_NUM, 11);
  
  //Serial.begin(9600);  // Hardware serial for Monitor (for debugging)
  RFID.begin(2400);
  
  RFID_ReadMode = RFID_WAIT_FOR_START;
  cardDataCount = 0;
  
  // Set backlight brightness to max
  analogWrite(LCD_BL_DIM, LCD_BL_MAX);
  
  // Start the LCD library
  lcd.begin(LCD_COLS, LCD_ROWS);
  
  // Clear LCD
  lcd.clear();
  
  // Here are some example LCD messages (for testing)
  //lcd.print("READY FOR SCAN"); // test
  //lcd.print("ENTER PIN: "); // test
  lcd.print("Ready..."); // test
  //lcd.print("All OlyMEGA members welcome."); // test
  //lcd.print("ENTER EVENT CODE"); // test
  
  // Setup keypad library
  keypad.addEventListener(keypadEvent);          // Add an event listener.
  keypad.setHoldTime(500);                       // Default is 1000mS

  // Enable the RFID reader
  pinMode(RFID_EN, OUTPUT);       // Set digital pin RFID_EN as OUTPUT to connect it to the RFID /ENABLE pin 
  digitalWrite(RFID_EN, LOW);    // Activate the RFID reader 
  
  // Play initialize tone
  playTone(833, 800000);
  
  // Start the I2C slave interface
  Wire.begin(I2C_SLAVE_ADDRESS);// join i2c bus with I2C_SLAVE_ADDRESS
  Wire.onReceive(receiveEvent); // register event
  Wire.onRequest(requestEvent); // register event
}


/////////////////////////////////////////////////////
// Main loop to handle I/O
void loop() { 
  lastKey = keypad.getKey();
  
  if( millis() - timeLastKey > KEYPAD_CODE_EXPIRE_TIME ) {
    buildCount = 0;
  }
  
  if( RFID.available() ) {
    processRFID(RFID.read());
  }
  
  // Timeout system for "stuck card" 
  // Ignores further reads of the card after the first
  if( cardReadStatus == RFID_CARD_SENT ) {
    timeDiff = millis() - timeLast;
    
    if( timeDiff > CARD_DELAY_INTERVAL ) { //
      cardReadStatus = RFID_CARD_WAIT;
    }
  }
  
  // Enable the RFID reader (in case it was disabled)
  digitalWrite(RFID_EN, LOW);
} 

/////////////////////////////////////////////////////
//////////////////// FUNCTIONS //////////////////////
/////////////////////////////////////////////////////

/* requestEvent()
  This is called when the master performs a read from this slave.
*/
void requestEvent()
{
  Wire.write((const uint8_t *)&userData, sizeof(userData));
  
  // Clear status register event code once the master has read it
  userData.Status = userData.Status & 0xFC;
  
  // This erases the whole register set
  //memset( &userData, 0, sizeof(userData) );
}



/* receiveEvent()
  This is called when the master performs a write to this slave.
*/
void receiveEvent(int bytesReceived)
{
  uint8_t tempData2[32];
  int rcvCount = 0;
  uint8_t *address = (uint8_t *)&displayData;
  
  char lcdBuffer[17];
  
  uint8_t oldControl1 = displayData.Control1;
  uint8_t oldControl2 = displayData.Control2;
  
  while(Wire.available()) // loop through all
  {
    unsigned char c = Wire.read(); // receive byte as a character
    tempData2[rcvCount++] = c;
  }
  
  int offset = tempData2[0];
  
  // clear the memory first
  memset( address+offset, 0, bytesReceived -1 );
  
  // Copy the received data after the offset byte to our display data structure
  memcpy((address+offset), &tempData2[1], bytesReceived -1);
  
//   if( (offset + bytesReceived - 1) > 2 ) {
// 	address[offset + bytesReceived] = 0;
//   }
  
  if( (oldControl1 & 0xF0) != (displayData.Control1 & 0xF0) ) {
    // We have a tone command
    makeNoise((displayData.Control1 & 0xF0) >> 4);
    
    // Reset the noise
    displayData.Control1 = displayData.Control1 & 0x0F;
  }
  
  if( (oldControl1 & 0x0F) != (displayData.Control1 & 0x0F) ) {
    // We have a LCD BL brightness command
    analogWrite(LCD_BL_DIM, (displayData.Control1 & 0x0F) << 3);
  }
  
  if( displayData.Control2 & COMMAND_REG_CTL2_SHWLCD ) {
    lcd.clear();
	
	// First half of the display
	lcd.setCursor(0, 0);
	memcpy((void *)lcdBuffer, (const void *)displayData.DisplayText, 16);
    lcd.print(lcdBuffer);
	
	// Second half of the display
	lcd.setCursor(0, 1);
	memcpy((void *)lcdBuffer, (const void *)&displayData.DisplayText[16], 16);
	lcd.print(lcdBuffer);
	
    displayData.Control2 = displayData.Control2 & 0xFD; // Clear the command bit now that the command is processed.
  }
  
  digitalWrite(LED_OUT, (displayData.Control2 & COMMAND_REG_CTL2_SWITCH) >> 7);
  
  return;
}



/* keypadEvent()
  Read the keypad if a key was pressed
*/
void keypadEvent(KeypadEvent key) {
    static char buildStr[PIN_CODE_LEN + 1];
    //static byte buildCount;

    switch (keypad.getState())
    {
    case PRESSED:
        playTone(238, 10000); // Sounds like "Beek"
        break;

    case HOLD:
        break;

    case RELEASED:
        long time = millis();
        
        //if( displayData.Control2 & 0x01 ) {
        //  lcd.print(key);
        //}
        
        timeLastKey = time;
        
        buildStr[buildCount++] = key;
        buildStr[buildCount] = '\0';
        
        if (buildCount >= PIN_CODE_LEN) {
          buildCount = 0;    // Our string is full. Start fresh.
          
          memcpy(userData.PINCode, buildStr, PIN_CODE_LEN);
          userData.Status = RESULT_REG_EVENT_PINENT;
          notifyMaster();
        }
        break;

    }
}



/* notifyMaster()
  Sends an interrupt signal to the master 
  indicating that there is data ready
*/
void notifyMaster() {
   digitalWrite(IRQ_OUT, LOW);
   delay(100);
   digitalWrite(IRQ_OUT, HIGH);  
}



/* makeNoise()
  Produce tones in the speaker
*/
void makeNoise(int whichNoise) {
   if (whichNoise > 0) {
     if( whichNoise == 1 ) {
      // BOO-DEEP
      playTone(2000, 100000);
      delay(50);
      playTone(833, 50000);
    }
    
    if( whichNoise == 2 ) {
      // BEE-DOOP
      playTone(833, 100000);
      delay(100);
      playTone(2000, 200000);
    }
    
    if( whichNoise == 3 ) {
      // BOOOP
      playTone(10000, 500000);
    }
    
//    if( whichNoise == 4 ) {
//      // BOOOOOOP
//      tone(AUDIO_OUT, 60, 1000);
//    }
  } 
}


/* playTone()
  Plays a tone on the speaker
  period is in microseconds and is equal to 1/frequency
  duration is in microseconds as well (yeah, I know...)
*/
void playTone(long period, long duration) {
  long elapsed_time = 0;
  
  if (period > 0) {
    while (elapsed_time < duration) {

      digitalWrite(AUDIO_OUT,HIGH);
      delayMicroseconds(period / 2);

      // DOWN
      digitalWrite(AUDIO_OUT, LOW);
      delayMicroseconds(period / 2);

      // Keep track of how long we pulsed
      elapsed_time += (period);
    }
  }                               
}



/* processRFID()
  State machine to handle incoming card data from the RFID reader
*/
void processRFID(char readChar) {
  switch( RFID_ReadMode ) {
    case RFID_WAIT_FOR_START:
      if( readChar == 10 ) {
        RFID_ReadMode = RFID_READING_DATA;
      }
      
      break;
   
    case RFID_READING_DATA:
      cardStr[cardDataCount++] = readChar;
      cardStr[cardDataCount] = '\0';
     
      if (cardDataCount >= RFID_CODE_LEN) {
        cardDataCount = 0;
        RFID_ReadMode = RFID_WAIT_FOR_START;
      }
      
      break;
   }

  switch( cardReadStatus ) {
    case RFID_CARD_VERIFY:
        timeDiff = millis() - timeLast;
        
        if( timeDiff < CARD_VERIFY_INTERVAL 
            && strncmp(cardStr, codeLast, RFID_CODE_LEN) == 0
            //&& strncmp(cardStr, userData.CardNum, RFID_CODE_LEN) != 0 // Wait until master answers?
            ) {
          digitalWrite(RFID_EN, HIGH);
          playTone(454, 50000); // Blip
          
          //lcd.clear();
          //lcd.print(cardStr);
          
          memcpy(userData.CardNum, cardStr, RFID_CODE_LEN);
          userData.Status = RESULT_REG_EVENT_CARDSWIPED;
          notifyMaster();
          
          cardReadStatus = RFID_CARD_SENT;
        } else {
          cardReadStatus = RFID_CARD_VERIFY;
          memcpy(codeLast, cardStr, RFID_CODE_LEN);
          timeLast = millis();
        }
        
        break;
        
    case RFID_CARD_SENT: // Part of "stuck card" timeout
      memcpy(codeLast, cardStr, RFID_CODE_LEN);
      timeLast = millis();      
      break;
      
    default:
      cardReadStatus = RFID_CARD_VERIFY;
      memcpy(codeLast, cardStr, RFID_CODE_LEN);
      timeLast = millis();
    }
  }

 
