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

// Header libraries
#include "OpenLCD.hpp"

void setup()
{
  wdt_reset(); //Pet the dog
  wdt_disable(); //We don't want the watchdog during init

  //During testing reset everything
  //for(int x = 0 ; x < 200 ; x++)
  //  EEPROM.write(x, 0xFF);
  currentMode = MODE_NORMAL;

  setupSystemMessages(); //Load settings, such as displaySystemMessages
  
  setupLCD(); //Initialize the LCD

  setupContrast(); //Set contrast

  setupBacklight(); //Turn on any backlights

  setupSplash(); //Read and display the user's splash screen

  setupCustomChars(); //Pre-load user's custom chars from EEPROM

  setupUART(); //Setup serial, check for emergency reset after the splash is done

  setupSPI(); //Initialize SPI stuff (enable, mode, interrupts)

  setupTWI(); //Initialize I2C stuff (address, interrupt, enable)

  setupPower(); //Power down peripherals that we won't be using

  interrupts();  // Turn interrupts on, and let's go
  wdt_enable(WDTO_250MS); //Unleash the beast
}

void loop()
{
  wdt_reset(); //Pet the dog

  //The TWI interrupt will fire whenever it fires and adds incoming I2C characters to the buffer
  //As does the SPI interrupt
  // //Serial is the only one that needs special attention
  // serialEvent(); //Check the serial buffer for new data

  while (buffer.tail != buffer.head) updateDisplay(); //If there is new data in the buffer, display it!

  //Once we've cleared the buffer, go to sleep
  sleep_mode(); //Stop everything and go to sleep. Wake up if serial character received
}