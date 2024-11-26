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
#include "lcd.h"
#include "button.h"
#include "Encoder.h" 

// Define Button pins
#define LEFT_BUTTON 0
#define RIGHT_BUTTON 10
#define MAIN_BUTTON 1
#define MENU_BUTTON 13
Button left_button;
Button main_button;
Button right_button;
Button menu_button;

// Button status
int screen_idx = 0;
bool button_pressed;
bool main_btn_pressed;
bool menu_btn_pressed;

// Define encoder pins
#define pinA 11
#define pinB 12
Encoder encoder( pinA, pinB);
int encoder_val = 0;
int encoder_pos = 0;
int encoder_chg = 0;
long tim_freq = 0;
long last_mes = 0;
long last_now = 0;
int count = 0;
char lcd_str[21];


// Timer settings      
void button_tick(void);

// Define functions
void button_tick(void) {
  // Check buttons
  if (right_button.update_btn()) {screen_idx++; button_pressed = true;}
  if (left_button.update_btn()) {screen_idx--; button_pressed = true;}
  if (main_button.update_btn()) {screen_idx = 0; main_btn_pressed = true;} 
  if (menu_button.update_btn()) {encoder_val = 0; menu_btn_pressed = true;}

  // Last run
  tim_freq = millis() - last_now;
  last_now = millis();
  count = 0;
}

// Setup I2C commands
#define PAYLOAD_SIZE 3
#define NUM_DISPLAY_SCREENS 11

enum {
    SEND_SCREEN = 169,
    UPDATE_SETTINGS = 172,
    UPDATE_SCREEN = 142,
};

typedef struct Command{
  uint8_t cmd;
  uint8_t payload[PAYLOAD_SIZE];
}Command;

void sendCommand(Command commandStruct, uint8_t slave_address,uint8_t responseSize){
  Wire.beginTransmission(slave_address);
  Wire.write((byte *)&commandStruct, sizeof(commandStruct));
  Wire.endTransmission ();
  if (Wire.requestFrom(slave_address, responseSize) == 0){
    Serial.println("Error");
  } else {
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
}

// LCD Setup
byte slave_i2c_adr = 0x72;
multifuel_lcd_config lcd_config {
  SerLCD,
  Wire,
  slave_i2c_adr
};
MULTIFUEL_LCD multifuel_lcd(lcd_config);

// Main Code
void setup()
{
  wdt_reset(); //Pet the dog
  wdt_disable(); //We don't want the watchdog during init
  noInterrupts();

  // Setup Encoder
  encoder_val = 0;
  encoder_pos = 0;
  encoder_chg = 0;
  screen_idx = 0;
  last_mes = 0;
  tim_freq = 0;
  button_pressed = false;
  menu_btn_pressed = false;
  main_btn_pressed = false;

  // Initialise Buttons
  left_button.init(LEFT_BUTTON);
  right_button.init(RIGHT_BUTTON);
  main_button.init(MAIN_BUTTON);
  menu_button.init(MENU_BUTTON);

  // Print Test to LCD
  multifuel_lcd.init(lcd_config);

  interrupts();  // Turn interrupts on, and let's go
  wdt_enable(WDTO_250MS); //Unleash the beast
}

void loop()
{
  wdt_reset(); //Pet the dog

  // Update buttons
  button_tick();
  
  // Update encoder
  int tmp = encoder.read() / 4; // 1 detent == 4 positions
  encoder_val += (tmp - encoder_pos);
  encoder_pos = tmp; // save current encoder position

  // Request Data from the main MCU
  if ((button_pressed || menu_btn_pressed) || main_btn_pressed) {
    // Make sure creen_idx is within limits
    if (screen_idx > NUM_DISPLAY_SCREENS) screen_idx -= NUM_DISPLAY_SCREENS;
    if (screen_idx < 0) screen_idx += NUM_DISPLAY_SCREENS;
  }
  
}
