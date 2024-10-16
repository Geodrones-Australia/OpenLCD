#pragma once
#ifndef _OPENLCD_H_
#define _OPENLCD_H_

#include "Arduino.h"
#include <Wire.h> //For I2C functions
#include <SPI.h> //For SPI functions
#include <LiquidCrystalFast.h> //Faster LCD commands. From PJRC https://www.pjrc.com/teensy/td_libs_LiquidCrystal.html
#include <EEPROM.h>  //Brightness, Baud rate, and I2C address are stored in EEPROM
#include <avr/wdt.h> //Watchdog to prevent system freeze
#include <avr/sleep.h> //Needed for sleep_mode
#include <avr/power.h> //Needed for powering down perihperals such as the ADC/TWI and Timers
#include <SoftPWM.h> //Software PWM for Blue backlight: From https://github.com/bhagman/SoftPWM

//Hardware pin definitions
#define LCD_RS A0
#define LCD_RW A1
#define LCD_EN A2
#define LCD_D4 A3
#define LCD_D5 2
#define LCD_D6 7
#define LCD_D7 4
#define BL_RW 5 //PWM
#define BL_G  6 //PWM
#define BL_B  3 //Not PWM so we use SoftPWM
#define SIZE_JUMPER 8
#define LCD_CONTRAST 9 //PWM

// LCD


#define SPI_CS 10 //As a slave device CS pin must always be 10
#define SPI_MOSI 11
#define SPI_MISO 12
#define SPI_SCK 13

//Define the different baud rate levels
#define BAUD_1200	0
#define BAUD_2400	1
#define BAUD_4800	2
#define BAUD_9600	3
#define BAUD_14400	4
#define BAUD_19200	5
#define BAUD_38400	6
#define BAUD_57600	7
#define BAUD_115200	8
#define BAUD_230400	9
#define BAUD_460800	10
#define BAUD_921600	11
#define BAUD_1000000	12

const byte DEFAULT_TWI_ADDRESS = 0x72; //0x71 for Serial7Segment. 0x72 for SerLCD.
const byte DEFAULT_BAUD = BAUD_9600;  //9600 for 8MHz, 2x speed
const byte DEFAULT_RED = 255;
const byte DEFAULT_GREEN = 255;
const byte DEFAULT_BLUE = 255;
const byte DEFAULT_LINES = 2;
const byte DEFAULT_WIDTH = 16;
const byte DEFAULT_SPLASH = true; //Default on
const byte DEFAULT_CONTRAST_LCD = 5;
const byte DEFAULT_DISPLAY_SYSTEM_MESSAGES = true; //Enable messages

//Internal EEPROM locations for the user settings
#define LOCATION_BAUD 0
#define LOCATION_TWI 1
#define LOCATION_SPLASH_ONOFF 2
#define LOCATION_LINES 3
#define LOCATION_WIDTH 4
#define LOCATION_RED_BRIGHTNESS 5
#define LOCATION_GREEN_BRIGHTNESS 6
#define LOCATION_BLUE_BRIGHTNESS 7
#define LOCATION_IGNORE_RX 8
#define LOCATION_TWI_ADDRESS 9
#define LOCATION_CONTRAST 10 //8 bit
#define LOCATION_DISPLAY_SYSTEM_MESSAGES 11 //8 bit
#define LOCATION_SPLASH_CONTENT 20 //This is 4*20 or 80 bytes wide
#define LOCATION_CUSTOM_CHARACTERS 100 //This is 8*8 or 64 bytes wide

//Define the various commands
#define SPECIAL_COMMAND 254 //0xFE: The command to do special HD77480 commands
#define SPECIAL_SETTING '|' //124, 0x7C, the pipe character: The command to do special settings: baud, lines, width, backlight, splash, etc

#define SPECIAL_RED_MIN 128 //Command minimum for red/white backlight brightness
#define SPECIAL_GREEN_MIN (SPECIAL_RED_MIN+30) //Command for green backlight brightness
#define SPECIAL_BLUE_MIN (SPECIAL_GREEN_MIN+30) //Command for blue backlight brightness

//Used for the different color backlights
#define RED    0
#define BLUE   1
#define GREEN  2

const byte BUFFER_SIZE = 128; //Number of characters we can hold in the incoming buffer
const byte DISPLAY_BUFFER_SIZE = 4*20; //4x20 the max number of characters we will display at one time

#define SYSTEM_MESSAGE_DELAY 500 //Amount of time (ms) we spend displaying splash and system messages

//Global setting variables
byte settingLCDwidth;
byte settingLCDlines;
bool settingSplashEnable;
byte settingUARTSpeed;
bool settingIgnoreRX;
bool settingDisplaySystemMessages; //User can turn on/off the messages that are displayed when setting (like contrast) is changed

// OpenLCD.cpp structures
const byte firmwareVersionMajor = 1;
const byte firmwareVersionMinor = 4;
byte characterCount = 0;
char currentFrame[DISPLAY_BUFFER_SIZE]; //Max of 4 x 20 LCD

byte customCharData[8]; //Records incoming custom character data
byte customCharSpot = 0 ; //Keeps track of where we are in custCharData array
byte customCharNumber = 0; //LCDs can store 8 custom chars, this keeps track

//New variables for Set RGB command
byte rgbData[3]; //Records incoming backlight rgb triplet
byte rgbSpot = 0 ; //Keeps track of where we are in rgbData array

enum displayMode
{
  MODE_NORMAL, //No mode, just print
  MODE_COMMAND, //Used to indicate if a command byte has been received
  MODE_SETTING, //Used to indicate if a setting byte has been received
  MODE_CONTRAST, //First setting mode, then contrast change mode, then the value to change to
  MODE_TWI, //First setting mode, then I2C mode, then change I2C address
  MODE_RECORD_CUSTOM_CHAR, //First setting mode, then custom char mode, then record 8 bytes
  MODE_SET_RGB //First setting mode, then RGB mode, then get 3 bytes
} currentMode;

// Struct for circular data buffer
// Data received over UART, SPI and I2C are all sent into a single buffer
struct dataBuffer
{
  byte data[BUFFER_SIZE];  // THE data buffer
  volatile byte head;  // store new data at this index
  volatile byte tail;  // read oldest data from this index
} buffer;  // our data buffer is creatively named - buffer



// OpenLCD.cpp
void updateDisplay();
void clearFrameBuffer();
void displayFrameBuffer(void);

// Settings_Control.cpp
void changeIgnore();
void enableDisplaySystemMessages();
void disableDisplaySystemMessages();
void displayFirmwareVersion();
void changeContrast(byte contrast);
void changeTWIAddress(byte newAddress);
void changeSplashContent();
void changeBLBrightness(byte color, byte brightness);
void changeBacklightRGB(byte red, byte green, byte blue);
void changeUARTSpeed(byte setting);
void changeSplashEnable();
void enableSplash();
void disableSplash();
void changeLinesWidths(byte setting);

// System_Functions.cpp
// SPI byte received interrupt routine
ISR(SPI_STC_vect)
{
  //noInterrupts();  // don't be rude! I'll be quick...

  byte c = SPDR;  // Read data byte into c, from SPI data register
  byte i = (buffer.head + 1) % BUFFER_SIZE;  // read buffer head position and increment

  if (i != buffer.tail)  // As long as the buffer isn't full, we can store the data in buffer
  {
    buffer.data[buffer.head] = c;  // Store the data into the buffer's head
    buffer.head = i;  // update buffer head, since we stored new data
  }

  //interrupts();  // Fine, you were saying?
}
void serialEvent();
void twiReceive(int rxCount);
void setupPower();
void setupUART();
void setupSPI();
void setupTWI();
void setupContrast();
void setupSystemMessages();
void setupLCD();
void setupBacklight();
void setupSplash();
void setupCustomChars();
void checkEmergencyReset(void);
long lookUpBaudRate(byte setting);
void petSafeDelay(int delayAmount);
void setPwmFrequency(int pin, int divisor);

/*
  OpenLCD is an LCD with serial/i2c/spi interfaces.
  By: Nathan Seidle
  SparkFun Electronics
  Date: February 13th, 2015
  License: This code is public domain but you buy me a beer if you use this and we 
  meet someday (Beerware license).

  OpenLCD gives the user multiple interfaces (serial, I2C, and SPI) to control an LCD. SerLCD 
  was the original serial LCD from SparkFun that ran on the PIC 16F88 with only a serial interface 
  and limited feature set. This is an updated serial LCD.

  Select 'SparkFun SerLCD' as the board. We use an ATmega328P running at 11.0592MHz in 
  order to have error free serial comm.

  v11: 
    Add faster backlight command
    Change EEPROM write() to update()  

  v12:
    Add command to disable system messages displayed when settings change (contrast, UART, etc).
    Add discrete commands to enableSplash and disableSplash. Better for embedded systems if it needs to disable splash.

  v13:
    emergency reset bug fixed - https://github.com/sparkfun/OpenLCD/pull/21
    
  v14:
    enable true off for blue LED - https://github.com/sparkfun/OpenLCD/pull/22
*/

//Firmware version. This is sent when requested. Helpful for tech support.

//SoftPWM uses Timer 2
LiquidCrystalFast SerLCD(LCD_RS, LCD_RW, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
// updateDisplay(): This beast of a function is called by the main loop
// If the data relates to a commandMode or settingMode will be set accordingly or a command/setting
// will be executed from this function.
// If the incoming data is just a character it will be displayed
void updateDisplay()
{
  wdt_reset(); //Pet the dog

  // First we read from the oldest data in the buffer
  byte incoming = buffer.data[buffer.tail];
  buffer.tail = (buffer.tail + 1) % BUFFER_SIZE;  // and update the tail to the next oldest

  //If the last byte received wasn't special
  if (currentMode == MODE_NORMAL)
  {
    //Check to see if the incoming byte is special
    if (incoming == SPECIAL_SETTING) //SPECIAL_SETTING is 127
    {
      currentMode = MODE_SETTING;
    }
    else if (incoming == SPECIAL_COMMAND) //SPECIAL_COMMAND is 254
    {
      currentMode = MODE_COMMAND;
    }
    else if (incoming == 8) //Backspace
    {
      if (characterCount == 0) characterCount = settingLCDwidth * settingLCDlines; //Special edge case

      characterCount--; //Back up

      currentFrame[characterCount] = ' '; //Erase this spot from the buffer
      displayFrameBuffer(); //Display what we've got
    }
    else //Simply display this character to the screen
    {
      SerLCD.write(incoming);

      currentFrame[characterCount++] = incoming; //Record this character to the display buffer
      if (characterCount == settingLCDwidth * settingLCDlines) characterCount = 0; //Wrap condition
    }
  }
  else if (currentMode == MODE_SETTING)
  {
    currentMode = MODE_NORMAL; //We assume we will be returning to normal

    //LCD width and line settings
    if (incoming >= 3 && incoming <= 7) //Ctrl+c to Ctrl+g
    {
      //Convert incoming value down to 0 to 4
      changeLinesWidths(incoming - 3);
    }
    //Software reset
    else if (incoming == 8) //Ctrl+h
    {
      while (1); //Hang out and let the watchdog punish us
    }
    //Enable / disable splash setting
    else if (incoming == 9) //Ctrl+i
    {
      changeSplashEnable();
    }
    //Save current buffer as splash
    else if (incoming == 10) //Ctrl+j
    {
      changeSplashContent();
    }
    //Set baud rate
    else if (incoming >= 11 && incoming <= 23) //Ctrl+k to ctrl+w
    {
      //Convert incoming value down to 0
      changeUARTSpeed(incoming - 11);
    }
    //Set contrast
    else if (incoming == 24) //Ctrl+x
    {
      currentMode = MODE_CONTRAST; //Go to new mode
      //We now grab the next character on the next loop and use it to change the contrast
    }
    //Set TWI address
    else if (incoming == 25) //Ctrl+y
    {
      currentMode = MODE_TWI; //Go to new mode
      //We now grab the next character on the next loop and use it to change the TWI address
    }
    //Control ignore RX on boot
    else if (incoming == 26) //Ctrl+z
    {
      changeIgnore();
    }
    //Record custom characters
    else if (incoming >= 27 && incoming <= 34)
    {
      //User can record up to 8 custom chars
      customCharNumber = incoming - 27; //Get the custom char spot to record to

      currentMode = MODE_RECORD_CUSTOM_CHAR; //Change to this special mode
    }

    //Display custom characters, 8 characters allowed, 35 to 42 inclusive
    else if (incoming >= 35 && incoming <= 42)
    {
      SerLCD.write(byte(incoming - 35)); //You write location zero to display customer char 0
    }
    //Set Backlight RGB in one command to eliminate flicker
    else if (incoming == 43) //+ character
    {
      currentMode = MODE_SET_RGB; //Go to new mode
    }
    //Display current firmware version
    else if (incoming == 44) //, character
    {
      displayFirmwareVersion();
    }
    //Clear screen and buffer
    else if (incoming == 45) //- character
    {
      SerLCD.clear();
      SerLCD.setCursor(0, 0);

      clearFrameBuffer(); //Get rid of all characters in our buffer
    }
    //Enable the displaying of system messages
    else if (incoming == 46) //. character
    {
      enableDisplaySystemMessages();
    }
    //Disable the displaying of system messages
    else if (incoming == 47) // / character
    {
      disableDisplaySystemMessages();
    }
    //Enable the splash screen at power on
    else if (incoming == 48) //0 character
    {
      enableSplash();
    }
    //Disable the splash screen at power on
    else if (incoming == 49) //1 character
    {
      disableSplash();
    }

    //If we get a second special setting character, then write it to the display
    //This allows us to print a pipe by escaping it as a double
    else if (incoming == 124) //| character
    {
      SerLCD.write(incoming);

      currentFrame[characterCount++] = incoming; //Record this character to the display buffer
      if (characterCount == settingLCDwidth * settingLCDlines) characterCount = 0; //Wrap condition
    }

    //The following commands start at integer value 128
    
    //Backlight Red or standard white
    else if (incoming >= SPECIAL_RED_MIN && incoming <= (SPECIAL_RED_MIN + 29))
    {
      byte brightness = map(incoming, SPECIAL_RED_MIN, SPECIAL_RED_MIN + 29, 0, 255); //Covert 30 digit value to 255 digits
      changeBLBrightness(RED, brightness);
    }
    //Backlight Green
    else if (incoming >= SPECIAL_GREEN_MIN && incoming <= (SPECIAL_GREEN_MIN + 29))
    {
      byte brightness = map(incoming, SPECIAL_GREEN_MIN, SPECIAL_GREEN_MIN + 29, 0, 255); //Covert 30 digit value to 255 digits
      changeBLBrightness(GREEN, brightness);
    }
    //Backlight Blue
    else if (incoming >= SPECIAL_BLUE_MIN && incoming <= (SPECIAL_BLUE_MIN + 29))
    {
      byte brightness = map(incoming, SPECIAL_BLUE_MIN, SPECIAL_BLUE_MIN + 29, 0, 255); //Covert 30 digit value to 255 digits
      changeBLBrightness(BLUE, brightness);
    }
  }
  else if (currentMode == MODE_TWI)
  {
    //Custom TWI address
    changeTWIAddress(incoming);

    currentMode = MODE_NORMAL; //Return to normal operation
  }
  else if (currentMode == MODE_COMMAND) //Deal with lower level commands
  {
    currentMode = MODE_NORMAL; //In general, return to normal mode

    if (incoming >> 7 == 1) //This is a cursor position command
    {
      incoming &= 0x7F; //Get rid of the leading 1

      byte line = 0;
      byte spot = 0;
      if (incoming >= 0 && incoming <= 19)
      {
        spot = incoming;
        line = 0;
      }
      else if (incoming >= 64 && incoming <= 83)
      {
        spot = incoming - 64;
        line = 1;
      }
      else if (incoming >= 20 && incoming <= 39)
      {
        spot = incoming - 20;
        line = 2;
      }
      else if (incoming >= 84 && incoming <= 103)
      {
        spot = incoming - 84;
        line = 3;
      }

      SerLCD.setCursor(spot, line); //(x, y) - Set to X spot on the given line
    }
    else if (incoming >> 6 == 1) //This is Set CGRAM address command
    {
      //User is trying to create custom character

      incoming &= 0b10111111; //Clear the ACG bit

      //User can record up to 8 custom chars
      customCharNumber = incoming - 27; //Get the custom char spot to record to

      currentMode = MODE_RECORD_CUSTOM_CHAR; //modeRecordCustomChar = true; //Change to this special mode
    }
    else if (incoming >> 4 == 1) //This is a scroll/shift command
    {
      /*See page 24/25 of the datasheet: https://www.sparkfun.com/datasheets/LCD/HD44780.pdf
        Bit 3: (S/C) 1 = Display shift, 0 = cursor move
        Bit 2: (R/L) 1 = Shift to right, 0 = shift left
      */

      //Check for display shift or cursor shift
      if (incoming & 1 << 3) //Display shift
      {
        if (incoming & 1 << 2) SerLCD.scrollDisplayRight(); //Go right
        else SerLCD.scrollDisplayLeft(); //Go left
      }
      else //Cursor move
      {
        //Check for right/left cursor move
        if (incoming & 1 << 2) //Right shift
        {
          characterCount++; //Move cursor right
          if (characterCount == settingLCDwidth * settingLCDlines) characterCount = 0; //Wrap condition
        }
        else
        {
          if (characterCount == 0) characterCount = settingLCDwidth * settingLCDlines; //Special edge case
          characterCount--; //Move cursor left
        }
        SerLCD.setCursor(characterCount % settingLCDwidth, characterCount / settingLCDwidth); //Move the cursor
      }
    }
    else if (incoming >> 3 == 1) //This is a cursor or display on/off control command
    {
      /*See page 24 of the datasheet: https://www.sparkfun.com/datasheets/LCD/HD44780.pdf

        Bit 3: Always 1 (1<<3)
        Bit 2: 1 = Display on, 0 = display off
        Bit 1: 1 = Cursor displayed (an underline), 0 = cursor not displayed
        Bit 0: 1 = Blinking box displayed, 0 = blinking box not displayed

        You can combine bits 1 and 2 to turn on the underline and then blink a box. */

      //Check for blinking box cursor on/off
      if (incoming & 1 << 0) SerLCD.blink();
      else SerLCD.noBlink();

      //Check for underline cursor on/off
      if (incoming & 1 << 1) SerLCD.cursor();
      else SerLCD.noCursor();

      //Check for display on/off
      if (incoming & 1 << 2) SerLCD.display();
      else SerLCD.noDisplay();
    }
    else if (incoming >> 4 != 0b00000011) //If not the data length (DL) command then send it to LCD
    {
      //We ignore the command that could set LCD to 8bit mode
      //But otherwise give the user the ability to pass commands directly
      //into the LCD.
      SerLCD.command(incoming);
    }
  }
  else if (currentMode == MODE_RECORD_CUSTOM_CHAR)
  {
    //We get into this mode if the user has sent the correct setting or system command

    customCharData[customCharSpot] = incoming; //Record this byte to the array

    customCharSpot++;
    if (customCharSpot > 7)
    {
      //Once we have 8 bytes, stop listening
      customCharSpot = 0; //Wrap variable at max of 7

      SerLCD.createChar(customCharNumber, customCharData); //Record the array to CGRAM

      //Record this custom char to EEPROM
      for (byte charSpot = 0 ; charSpot < 8 ; charSpot++)
        EEPROM.update(LOCATION_CUSTOM_CHARACTERS + (customCharNumber * 8) + charSpot, customCharData[charSpot]); //addr, val

      //For some reason you need to re-init the LCD after a custom char is created
      SerLCD.begin(settingLCDwidth, settingLCDlines);

      currentMode = MODE_NORMAL; //Exit this mode
    }
  }
  else if (currentMode == MODE_CONTRAST)
  {
    //We get into this mode if the user has sent the ctrl+x (24) command to change contast
    changeContrast(incoming);
    currentMode = MODE_NORMAL; //Exit this mode
  }
  else if (currentMode == MODE_SET_RGB)
  {
    //We get into this mode if the user has sent the + (43) command to set the backlight rgb values
    rgbData[rgbSpot] = incoming; //Record this byte to the array

    rgbSpot++;
    if (rgbSpot > 2)
    {
      //Once we have 3 bytes, stop listening and change the backlight color
      rgbSpot = 0;
      changeBacklightRGB(rgbData[0], rgbData[1], rgbData[2]);
      currentMode = MODE_NORMAL; //Exit this mode
    } //if (rgbSpot > 2)
  } // else if modeSetRGB

}

//Flushes all characters from the frame buffer
void clearFrameBuffer()
{
  //Clear the frame buffer
  characterCount = 0;
  for (byte x = 0 ; x < (settingLCDwidth * settingLCDlines) ; x++)
    currentFrame[x] = ' ';
}

//Display the LCD buffer and return the cursor to where it was before the system message
void displayFrameBuffer(void)
{
  //Return display to previous buffer
  SerLCD.clear();
  SerLCD.setCursor(0, 0);

  for (byte x = 0 ; x < (settingLCDlines * settingLCDwidth) ; x++)
    SerLCD.write(currentFrame[x]);

  //Return the cursor to its original position
  SerLCD.setCursor(characterCount % settingLCDwidth, characterCount / settingLCDwidth);
}

/*
  OpenLCD System Functions

  See main file for license and information.

  These are the ISRs for system functions that allow SerLCD to run

  This is heavily based on the Serial 7 Segment firmware

*/

// This is effectively the UART0 byte received interrupt routine
// But not quite: serialEvent is only called after each loop() interation
void serialEvent()
{
  while (Serial.available())
  {
    byte c = Serial.read();  // Read data byte into c, from UART0 data register
    byte i = (buffer.head + 1) % BUFFER_SIZE;  // read buffer head position and increment

    if (i != buffer.tail)  // As long as the buffer isn't full, we can store the data in buffer
    {
      buffer.data[buffer.head] = c;  // Store the data into the buffer's head
      buffer.head = i;  // update buffer head, since we stored new data
    }
  }
}

// I2C byte receive interrupt routine
// Note: this isn't an ISR. I'm using wire library (because it just works), so
// Wire.onReceive(twiReceive); should be called
void twiReceive(int rxCount)
{
  while (Wire.available()) // Do this while data is available in Wire buffer
  {
    byte c = Wire.read();  // Read data byte into c, from Wire data buffer
    byte i = (buffer.head + 1) % BUFFER_SIZE;  // read buffer head position and increment

    if (i != buffer.tail)  // As long as the buffer isn't full, we can store the data in buffer
    {
      buffer.data[buffer.head] = c;  // Store the data into the buffer's head
      buffer.head = i;  // update buffer head, since we stored new data
    }
  }
}

//Powers down all the unused parts of the ATmega
//Gets the micro ready to sleep when nothing is received
void setupPower()
{
  //Power down various bits of hardware to lower power usage
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();

  //Shut down peripherals we don't need
  ADCSRA &= ~(1 << ADEN); //Disable ADC
  ACSR = (1 << ACD); //Disable the analog comparator
  DIDR0 = 0x3F; //Disable digital input buffers on all ADC0-ADC5 pins
  DIDR1 = (1 << AIN1D) | (1 << AIN0D); //Disable digital input buffer on AIN1/0

  power_adc_disable(); //Not needed
  //power_twi_disable(); //We need this for I2C comm
  //power_spi_disable(); //We need this for SPI comm
  //power_timer0_disable(); //We need this for delay()
  //power_timer1_disable(); //We need this for PWMing the contrast on pin 9
  //power_timer2_disable(); //We need this for PWMing the Blue backlight on pin 3
}

//This sets up the UART with the stored baud rate in EEPROM
void setupUART()
{
  //Check to see if we are ignoring the RX reset or not
  settingIgnoreRX = EEPROM.read(LOCATION_IGNORE_RX);

  if (settingIgnoreRX == false) //If we are NOT ignoring RX, then
    checkEmergencyReset(); //Look to see if the RX pin is being pulled low

  //Read what the current UART speed is from EEPROM memory
  //Default is 9600
  settingUARTSpeed = EEPROM.read(LOCATION_BAUD);
  if (settingUARTSpeed > BAUD_1000000) //Check to see if the baud rate has ever been set
  {
    settingUARTSpeed = DEFAULT_BAUD; //Reset UART to 9600 if there is no baud rate stored
    EEPROM.update(LOCATION_BAUD, settingUARTSpeed);
  }

  //Initialize the UART
  Serial.begin(lookUpBaudRate(settingUARTSpeed));
}

// Initialize SPI, sets up hardware pins and enables spi and receive interrupt
// SPI is set to MODE 0 (CPOL=0, CPHA=0), slave mode, LSB first
void setupSPI()
{
  pinMode(SPI_SCK, INPUT);
  pinMode(SPI_MOSI, INPUT);
  pinMode(SPI_MISO, OUTPUT);
  pinMode(SPI_CS, INPUT); //There is a 10k pull up on the SS pin

  SPCR = (1 << SPIE) | (1 << SPE); // Enable SPI interrupt, enable SPI
  // DORD = 0, LSB First
  // MSTR = 0, SLAVE
  // CPOL = 0, sck low when idle                  } MODE 0
  // CPHA = 0, data sampled on leading clock edge } MODE 0
  // SPR1:0 = 0, no effect (slave mode)
}

// Initializes I2C
// I'm using the rock-solid Wire library for this. We'll initialize TWI, setup the address,
// and tell it what interrupt function to jump to when data is received.
void setupTWI()
{
  byte twiAddress;

  twiAddress = EEPROM.read(LOCATION_TWI_ADDRESS);  // read the TWI address from

  if ((twiAddress == 0) || (twiAddress > 0x7F))
  { // If the TWI address is invalid, use a default address
    twiAddress = DEFAULT_TWI_ADDRESS;
    EEPROM.update(LOCATION_TWI_ADDRESS, DEFAULT_TWI_ADDRESS);
  }

  Wire.begin(twiAddress);  //Initialize Wire library as slave at twiAddress address
  Wire.onReceive(twiReceive);  //Setup interrupt routine for when data is received
}

//This sets up the contrast
void setupContrast()
{
  //Read what the current contrast is, default is changes depending on display type
  byte settingContrast = EEPROM.read(LOCATION_CONTRAST);
  if (settingContrast == 255) //Check to see if the contrast has ever been set
  {
    settingContrast = DEFAULT_CONTRAST_LCD; //Default
    EEPROM.update(LOCATION_CONTRAST, settingContrast);
  }

  //Change contrast without notification message
  setPwmFrequency(LCD_CONTRAST, 1); //Set the freq of this pin so that it doesn't cause LCD to ripple
  pinMode(LCD_CONTRAST, OUTPUT);
  analogWrite(LCD_CONTRAST, settingContrast);
}

//Look up settings like the enabling of system messages
void setupSystemMessages()
{
  //Look up if we should display messages or not
  settingDisplaySystemMessages = EEPROM.read(LOCATION_DISPLAY_SYSTEM_MESSAGES);
  if (settingDisplaySystemMessages) //True = 1, false = 0
  {
    settingDisplaySystemMessages = DEFAULT_DISPLAY_SYSTEM_MESSAGES;
    EEPROM.update(LOCATION_DISPLAY_SYSTEM_MESSAGES, settingDisplaySystemMessages);
  }
}

//Look up and initialize the LCD with the lines and width
void setupLCD()
{
  //Look up LCD lines and width
  settingLCDlines = EEPROM.read(LOCATION_LINES);
  if (settingLCDlines > 4)
  {
    settingLCDlines = DEFAULT_LINES;
    EEPROM.update(LOCATION_LINES, settingLCDlines);
  }

  settingLCDwidth = EEPROM.read(LOCATION_WIDTH);
  if (settingLCDwidth > 20)
  {
    settingLCDwidth = DEFAULT_WIDTH;
    EEPROM.update(LOCATION_WIDTH, settingLCDwidth);
  }

  //Check the display jumper
  //If the jumper is set, use it
  pinMode(SIZE_JUMPER, INPUT_PULLUP);
  if (digitalRead(SIZE_JUMPER) == LOW)
  {
    settingLCDlines = 4;
    settingLCDwidth = 20;
  }

  pinMode(SIZE_JUMPER, INPUT); //Turn off pullup to save power

  SerLCD.begin(settingLCDwidth, settingLCDlines); //Setup the width and lines for this LCD

  //Clear any characters in the frame buffer
  clearFrameBuffer();
}

//Look up and start the 3 backlight pins in analog mode
void setupBacklight()
{
  pinMode(BL_RW, OUTPUT);
  pinMode(BL_G, OUTPUT);
  pinMode(BL_B, OUTPUT);

  //By default EEPROM is 255 or 100% brightness
  //Because there's a PNP transistor we need to invert the logic (or subtract the user value from 255)
  analogWrite(BL_RW, 255 - EEPROM.read(LOCATION_RED_BRIGHTNESS));
  analogWrite(BL_G, 255 - EEPROM.read(LOCATION_GREEN_BRIGHTNESS));

  // SoftPWM has a true off (0 is really off), but not a true on (255 is not 100% duty cycle), so invert the logic to be able to fully turn off the blue LED.
  SoftPWMBegin(SOFTPWM_INVERTED ); //Start PWM. 
  SoftPWMSet(BL_B, EEPROM.read(LOCATION_BLUE_BRIGHTNESS)); //Setup this pin to be controlled with SoftPWM. Initialize to EEPROM value
  SoftPWMSetFadeTime(BL_B, 0, 0); //Don't fade - go immediately to this set PWM brightness
}

void setupSplash()
{
  //Find out if we should display the splash or not
  settingSplashEnable = EEPROM.read(LOCATION_SPLASH_ONOFF);
  if (settingSplashEnable)
  {
    settingSplashEnable = DEFAULT_SPLASH;
    EEPROM.update(LOCATION_SPLASH_ONOFF, settingSplashEnable);
  }

  if (settingSplashEnable == true)
  {
    //Look up user content from memory
    byte content = EEPROM.read(LOCATION_SPLASH_CONTENT);

    if (content == 0xFF)
    {
      //Display the default splash screen
      //This should work with both 16 and 20 character displays
      SerLCD.clear();
      SerLCD.setCursor(0, 0); //First position, 1st row
      SerLCD.print(F("SparkFun OpenLCD"));
      SerLCD.setCursor(0, 1); //First position, 2nd row
      SerLCD.print(F("Baud:"));

      //Read what the current UART speed is from EEPROM memory
      //Default is 9600
      settingUARTSpeed = EEPROM.read(LOCATION_BAUD);
      if (settingUARTSpeed > BAUD_1000000) //Check to see if the baud rate has ever been set
      {
        settingUARTSpeed = DEFAULT_BAUD; //Reset UART to 9600 if there is no baud rate stored
        EEPROM.update(LOCATION_BAUD, settingUARTSpeed);
      }

      SerLCD.print(lookUpBaudRate(settingUARTSpeed));

      //Display firmware version
      SerLCD.print(F(" v"));
      SerLCD.print(firmwareVersionMajor);
      SerLCD.print(F("."));
      SerLCD.print(firmwareVersionMinor);
    }
    else
    {
      //Pull splash content from EEPROM

      //Copy the EEPROM to the character buffer
      for (byte x = 0 ; x < settingLCDlines * settingLCDwidth ; x++)
        currentFrame[x] = EEPROM.read(LOCATION_SPLASH_CONTENT + x);

      //Now display the splash
      displayFrameBuffer();
    }

    //While we hold the splash screen monitor for incoming serial
    Serial.begin(9600); //During this period look for characters at 9600bps
    for (byte x = 0 ; x < (SYSTEM_MESSAGE_DELAY / 10) ; x++)
    {
      //Reverse compatiblity with SerLCD 2.5: a ctrl+r during splash will reset unit to 9600bps.
      if (Serial.available())
      {
        if (Serial.read() == 18) //ctrl+r
        {
          //Reset baud rate
          SerLCD.clear();
          SerLCD.setCursor(0, 0); //First position, 1st row

          SerLCD.print("Baud Reset");

          EEPROM.update(LOCATION_BAUD, BAUD_9600);

          petSafeDelay(SYSTEM_MESSAGE_DELAY);

          break;
        }
      } //This assumes that Serial.begin() will happen later

      //serialEvent(); //Check the serial buffer for new data
      petSafeDelay(10); //Hang out looking for new characters
    }

    //Now erase it and the buffer
    clearFrameBuffer();

    SerLCD.clear(); //Trash the splash
    SerLCD.setCursor(0, 0); //Reset cursor

    //After this function we go back to system baud rate
  }
}

//Look up the 8 custom chars from EEPROM and push them to the LCD
//We have to re-init the LCD after we send the chars
void setupCustomChars()
{
  for (byte charNumber = 0 ; charNumber < 8 ; charNumber++)
  {
    for (byte charSpot = 0 ; charSpot < 8 ; charSpot++)
      customCharData[charSpot] = EEPROM.read(LOCATION_CUSTOM_CHARACTERS + (charNumber * 8) + charSpot);

    SerLCD.createChar(charNumber, customCharData); //Record the array to CGRAM
  }

  //For some reason you need to re-init the LCD after a custom char is loaded
  SerLCD.begin(settingLCDwidth, settingLCDlines);
}

//Check to see if we need an emergency UART reset
//Scan the RX pin for 2 seconds
//If it's low the entire time, then reset to 9600bps
void checkEmergencyReset(void)
{
  byte rxPin = 0; //The RX pin is zero

  pinMode(rxPin, INPUT_PULLUP); //Turn the RX pin into an input with pullups

  if (digitalRead(rxPin) == HIGH) return; //Quick pin check
  //Wait 2 seconds, blinking backlight while we wait
  pinMode(BL_RW, OUTPUT);
  digitalWrite(BL_RW, HIGH); //Set the STAT2 LED
  for (byte i = 0 ; i < 80 ; i++)
  {
    wdt_reset(); //Pet the dog
    delay(25);

    //Blink backlight
    if (digitalRead(BL_RW))
      digitalWrite(BL_RW, LOW);
    else
      digitalWrite(BL_RW, HIGH);

    if (digitalRead(rxPin) == HIGH) return; //Check to see if RX is not low anymore
  }

  //If we make it here, then RX pin stayed low the whole time
  //Reset all EEPROM locations to factory defaults.
  for (int x = 0 ; x < 200 ; x++) EEPROM.update(x, 0xFF);

  //Change contrast without notification message
  analogWrite(LCD_CONTRAST, DEFAULT_CONTRAST_LCD); //Set contrast to default

  //Force ignoreRX to false.
  EEPROM.update(LOCATION_IGNORE_RX, false);

  //Change backlight to defaults
  changeBLBrightness(RED, DEFAULT_RED);
  changeBLBrightness(GREEN, DEFAULT_GREEN);
  changeBLBrightness(BLUE, DEFAULT_BLUE);

  SerLCD.clear();
  SerLCD.print("System reset");
  SerLCD.setCursor(0, 1); //First position, 2nd row
  SerLCD.print("Power cycle me");

  //Now sit in forever loop indicating system is now at 9600bps
  digitalWrite(BL_RW, HIGH);
  while (1)
  {
    petSafeDelay(500);

    //Blink backlight
    if (digitalRead(BL_RW))
      digitalWrite(BL_RW, LOW);
    else
      digitalWrite(BL_RW, HIGH);
  }
}

//We store the baud rate as a single digit in EEPROM
//This function converts the byte to the actual baud rate
long lookUpBaudRate(byte setting)
{
  switch (setting)
  {
    case BAUD_1200: return (1200);
    case BAUD_2400: return (2400);
    case BAUD_4800: return (4800);
    case BAUD_9600: return (9600);
    case BAUD_14400: return (14400);
    case BAUD_19200: return (19200);
    case BAUD_38400: return (38400);
    case BAUD_57600: return (57600);
    case BAUD_115200: return (115200);
    case BAUD_230400: return (230400);
    case BAUD_460800: return (460800);
    case BAUD_921600: return (921600);
    case BAUD_1000000: return (1000000);
  }

}

//Delays for a specified period that is pet safe
void petSafeDelay(int delayAmount)
{
  long startTime = millis();

  while ((unsigned long)(millis() - startTime) <= delayAmount)
  {
    wdt_reset(); //Pet the dog

    //Max 100ms delay
    for (byte x = 0 ; x < 100 ; x++)
    {
      delay(1);
      if ( (unsigned long)(millis() - startTime) >= delayAmount) break;
    }
  }

  wdt_reset(); //Pet the dog
}

//Comes from http://playground.arduino.cc/Code/PwmFrequency
//Allows us to set the base PWM freq for various PWM pins
//Pins 3, 9, 10, and 11 has base freq of 31250 Hz
//Pins 5, 6 has base freq of 62500 Hz
//Note that this function will have side effects on anything else that uses timers:
//Changes on pins 3, 5, 6, or 11 may cause the delay() and millis() functions to stop working. Other timing-related functions may also be affected.
//Changes on pins 9 or 10 will cause the Servo library to function incorrectly.
void setPwmFrequency(int pin, int divisor)
{
  byte mode;
  if (pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch (divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if (pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if (pin == 3 || pin == 11) {
    switch (divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}

/*
  OpenLCD control for changing settings

  See main file for license and information.
*/

//Toggle the ignore rx setting
void changeIgnore()
{
  settingIgnoreRX = EEPROM.read(LOCATION_IGNORE_RX);

  //Toggle the setting
  if (settingIgnoreRX == true)
    settingIgnoreRX = false;
  else
    settingIgnoreRX = true;

  //Record this new setting
  EEPROM.update(LOCATION_IGNORE_RX, settingIgnoreRX);

  if (settingDisplaySystemMessages == true)
  {
    //Display new settings to the user
    SerLCD.clear();
    SerLCD.setCursor(0, 0);

    SerLCD.print(F("Ignore RX O"));

    if (settingIgnoreRX == false)
      SerLCD.print(F("FF"));
    else
      SerLCD.print(F("N"));

    petSafeDelay(SYSTEM_MESSAGE_DELAY);

    displayFrameBuffer(); //Return the contents of the display
  }
}

//Turn on messages like 'Contrast: 5' when user changes setting
void enableDisplaySystemMessages()
{
  settingDisplaySystemMessages = true;

  //Record this new setting
  EEPROM.update(LOCATION_DISPLAY_SYSTEM_MESSAGES, settingDisplaySystemMessages);

  //Display new setting to the user
  SerLCD.clear();
  SerLCD.setCursor(0, 0);
  SerLCD.print(F("Messages ON"));
  petSafeDelay(SYSTEM_MESSAGE_DELAY);
  displayFrameBuffer(); //Return the contents of the display
}

//Turn off system messsages
void disableDisplaySystemMessages()
{
  settingDisplaySystemMessages = false;

  //Record this new setting
  EEPROM.update(LOCATION_DISPLAY_SYSTEM_MESSAGES, settingDisplaySystemMessages);
}

//Display the current firmware version for a set amount of time
void displayFirmwareVersion()
{
  SerLCD.clear();
  SerLCD.setCursor(0, 0);

  SerLCD.print(F("Firmware v"));
  SerLCD.print(firmwareVersionMajor);
  SerLCD.print(F("."));
  SerLCD.print(firmwareVersionMinor);

  petSafeDelay(SYSTEM_MESSAGE_DELAY);

  displayFrameBuffer(); //Return the contents of the display
}

//Change the digital contrast
//Press a or z to adjust, x to exit
void changeContrast(byte contrast)
{
  EEPROM.update(LOCATION_CONTRAST, contrast); //Store this new contrast

  //Go to this new contrast
  analogWrite(LCD_CONTRAST, contrast);

  if (settingDisplaySystemMessages == true)
  {
    //Display the new contrast
    SerLCD.clear();
    SerLCD.setCursor(0, 0); //First position, 1st row
    SerLCD.print("Contrast Set");
    SerLCD.setCursor(0, 1); //First position, 2nd row
    SerLCD.print(contrast);

    petSafeDelay(SYSTEM_MESSAGE_DELAY);

    displayFrameBuffer(); //Display what was there before
  }
}

//Change the I2C or TWI address
void changeTWIAddress(byte newAddress)
{
  //Record the new address
  EEPROM.update(LOCATION_TWI_ADDRESS, newAddress);

  setupTWI(); //Leverage the regular startup function

  if (settingDisplaySystemMessages == true)
  {
    //Display the new TWI address
    SerLCD.clear();
    SerLCD.setCursor(0, 0); //First position, 1st row

    SerLCD.print("New TWI: 0x");
    SerLCD.print(newAddress, HEX);

    petSafeDelay(SYSTEM_MESSAGE_DELAY);

    displayFrameBuffer(); //Display what was there before
  }
}

//Save the current frame buffer to EEPROM as the splash screen
void changeSplashContent()
{
  //Record the current frame to EEPROM
  for (byte x = 0 ; x < settingLCDlines * settingLCDwidth ; x++)
    EEPROM.update(LOCATION_SPLASH_CONTENT + x, currentFrame[x]);

  if (settingDisplaySystemMessages == true)
  {
    //Display the backlight setting
    SerLCD.clear();
    SerLCD.setCursor(0, 0); //First position, 1st row

    SerLCD.print("Splash Recorded");

    petSafeDelay(SYSTEM_MESSAGE_DELAY);

    displayFrameBuffer(); //Display what was there before
  }
}

//Changes the brightness of a given pin and updates the EEPROM location with that value
//Incoming brightness should be 0 to 255
void changeBLBrightness(byte color, byte brightness)
{
  if (color == RED)
  {
    EEPROM.update(LOCATION_RED_BRIGHTNESS, brightness); //Record new setting
    analogWrite(BL_RW, 255 - brightness); //Controlled by PNP so reverse the brightness value
  }
  else if (color == GREEN)
  {
    EEPROM.update(LOCATION_GREEN_BRIGHTNESS, brightness); //Record new setting
    analogWrite(BL_G, 255 - brightness); //Controlled by PNP so reverse the brightness value
  }
  else if (color == BLUE)
  {
    EEPROM.update(LOCATION_BLUE_BRIGHTNESS, brightness); //Record new setting
    //analogWrite(BL_B, 255 - brightness); //Controlled by PNP so reverse the brightness value
    SoftPWMSet(BL_B, brightness); //Controlled by software PWM. Reversed by SoftPWM
  }

  if (settingDisplaySystemMessages == true)
  {
    //Display the backlight setting
    SerLCD.clear();
    SerLCD.setCursor(0, 0); //First position, 1st row

    if (color == RED)
      SerLCD.print(F("Backlight"));
    else if (color == GREEN)
      SerLCD.print(F("Green"));
    else if (color == BLUE)
      SerLCD.print(F("Blue"));

    SerLCD.print(F(": "));

    brightness = map(brightness, 0, 255, 0, 100); //Covert to percentage
    SerLCD.print(brightness);
    SerLCD.print(F("%"));
    petSafeDelay(SYSTEM_MESSAGE_DELAY);

    displayFrameBuffer(); //Display what was there before
  }
}

//Changes the brightness of all three backlight pins and updates the EEPROM locations
//with their rgb values to eliminate flicker. Incoming brightness values should be 0 to 255
void changeBacklightRGB(byte red, byte green, byte blue) {
  //update red
  EEPROM.update(LOCATION_RED_BRIGHTNESS, red); //Record new setting
  analogWrite(BL_RW, 255 - red); //Controlled by PNP so reverse the brightness value

  //update green
  EEPROM.update(LOCATION_GREEN_BRIGHTNESS, green); //Record new setting
  analogWrite(BL_G, 255 - green); //Controlled by PNP so reverse the brightness value

  //update blue (SoftPWM)
  EEPROM.update(LOCATION_BLUE_BRIGHTNESS, blue); //Record new setting
  //analogWrite(BL_B, 255 - brightness); //Controlled by PNP so reverse the brightness value
  SoftPWMSet(BL_B, blue); //Controlled by software PWM. Reversed by SoftPWM
}

//Changes the baud rate setting
//Assumes caller is passing a number 0 to 12
void changeUARTSpeed(byte setting)
{
  //This is ugly but we need to maintain compatibility with SerLCD v2.5 units
  switch (setting)
  {
    case 0: //Ctrl+k
      settingUARTSpeed = BAUD_2400;
      break;
    case 1: //Ctrl+l
      settingUARTSpeed = BAUD_4800;
      break;
    case 2: //Ctrl+m
      settingUARTSpeed = BAUD_9600;
      break;
    case 3: //Ctrl+n
      settingUARTSpeed = BAUD_14400;
      break;
    case 4: //Ctrl+o
      settingUARTSpeed = BAUD_19200;
      break;
    case 5: //Ctrl+p
      settingUARTSpeed = BAUD_38400;
      break;
    case 6: //Ctrl+q
      settingUARTSpeed = BAUD_57600;
      break;
    case 7: //Ctrl+r
      settingUARTSpeed = BAUD_115200;
      break;
    case 8: //Ctrl+s
      settingUARTSpeed = BAUD_230400;
      break;
    case 9: //Ctrl+t
      settingUARTSpeed = BAUD_460800;
      break;
    case 10: //Ctrl+u
      settingUARTSpeed = BAUD_921600;
      break;
    case 11: //Ctrl+v
      settingUARTSpeed = BAUD_1000000;
      break;
    case 12: //Ctrl+w
      settingUARTSpeed = BAUD_1200;
      break;
  }

  //Record this new buad rate
  EEPROM.update(LOCATION_BAUD, settingUARTSpeed);

  //Go to this new baud rate
  Serial.begin(lookUpBaudRate(settingUARTSpeed));

  if (settingDisplaySystemMessages == true)
  {
    //Display that we are at this new speed
    SerLCD.clear();
    SerLCD.setCursor(0, 0); //First position, 1st row
    SerLCD.print(F("Baud now:"));
    SerLCD.print(lookUpBaudRate(settingUARTSpeed));
    petSafeDelay(SYSTEM_MESSAGE_DELAY);

    displayFrameBuffer(); //Display what was there before
  }
}

void changeSplashEnable()
{
  settingSplashEnable = EEPROM.read(LOCATION_SPLASH_ONOFF);

  //Toggle setting
  if (settingSplashEnable == true)
    settingSplashEnable = false;
  else
    settingSplashEnable = true;

  //Record this new setting
  EEPROM.update(LOCATION_SPLASH_ONOFF, settingSplashEnable);

  if (settingDisplaySystemMessages == true)
  {
    //Display new settings to the user
    SerLCD.clear();
    SerLCD.setCursor(0, 0);

    SerLCD.print(F("Splash O"));

    if (settingSplashEnable == true)
      SerLCD.print(F("FF"));
    else
      SerLCD.print(F("N"));

    petSafeDelay(SYSTEM_MESSAGE_DELAY);

    displayFrameBuffer(); //Return the contents of the display
  }
}

//Turn on splash at power on
void enableSplash()
{
  settingSplashEnable = true;

  //Record this new setting
  EEPROM.update(LOCATION_SPLASH_ONOFF, settingSplashEnable);

  if (settingDisplaySystemMessages == true)
  {
    //Display new settings to the user
    SerLCD.clear();
    SerLCD.setCursor(0, 0);

    SerLCD.print(F("Splash ON"));

    petSafeDelay(SYSTEM_MESSAGE_DELAY);

    displayFrameBuffer(); //Return the contents of the display
  }
}

//Disable the power on splash
void disableSplash()
{
  settingSplashEnable = false;

  //Record this new setting
  EEPROM.update(LOCATION_SPLASH_ONOFF, settingSplashEnable);

  if (settingDisplaySystemMessages == true)
  {
    //Display new settings to the user
    SerLCD.clear();
    SerLCD.setCursor(0, 0);

    SerLCD.print(F("Splash OFF"));

    petSafeDelay(SYSTEM_MESSAGE_DELAY);

    displayFrameBuffer(); //Return the contents of the display
  }
}

void changeLinesWidths(byte setting)
{
  switch (setting)
  {
    case 0:
      settingLCDwidth = 20;
      break;
    case 1:
      settingLCDwidth = 16;
      break;
    case 2:
      settingLCDlines = 4;
      break;
    case 3:
      settingLCDlines = 2;
      break;
    case 4:
      settingLCDlines = 1;
      break;
  }

  SerLCD.begin(settingLCDwidth, settingLCDlines); //Go to new setting

  //Very funky characters can show up here because the buffer has been resized
  //Not sure if we should clear the buffer or not. User would loose the characters on the current screen
  clearFrameBuffer();

  //Record this new setting
  EEPROM.update(LOCATION_WIDTH, settingLCDwidth);
  EEPROM.update(LOCATION_LINES, settingLCDlines);

  if (settingDisplaySystemMessages == true)
  {
    //Display new settings to the user
    SerLCD.clear();
    SerLCD.setCursor(0, 0);

    SerLCD.print(F("Lines:"));
    SerLCD.print(settingLCDlines);

    //If we have a single line LCD then clear the message after a second and print more
    if (settingLCDlines == 1)
    {
      petSafeDelay(SYSTEM_MESSAGE_DELAY);
      SerLCD.clear();
      SerLCD.setCursor(0, 0);
    }
    else
    {
      SerLCD.setCursor(0, 1); //We are assuming at least a two line LCD
    }

    SerLCD.print(F("Width:"));
    SerLCD.print(settingLCDwidth);
    petSafeDelay(SYSTEM_MESSAGE_DELAY);

    displayFrameBuffer(); //Return the contents of the display
  }
}

#endif