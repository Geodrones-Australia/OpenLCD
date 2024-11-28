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
bool enc_change = false;
int last_enc_ticks = 0;
bool lcd_refresh = false;

// LCD Setup
int address = 0;
byte error = 1;
uint8_t slave_i2c_adr = 0x72;
hd44780_pinIO SerLCD(LCD_RS, LCD_RW, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
#define LCD_BUS Wire
multifuel_lcd_config lcd_config {
  SerLCD,
  LCD_BUS,
  slave_i2c_adr
};
MULTIFUEL_LCD lcd;

// Define button functions
void button_tick(void);
void ui_update(void);

void button_tick(void) {
  // Check buttons
  if (right_button.update_btn() && !lcd.blinking_cursor) {screen_idx++; button_pressed = true;}
  if (left_button.update_btn() && !lcd.blinking_cursor) {screen_idx--; button_pressed = true;}
  if (main_button.update_btn() && !lcd.blinking_cursor) {main_btn_pressed = true;} 
  if (menu_button.update_btn()) {menu_btn_pressed = true;}

  // Last run
  #ifdef LCD_DEBUG
    tim_freq = millis() - last_now;
    last_now = millis();
    count = 0;
  #endif
}

void ui_update(void) {
  // Update buttons
  button_tick();
  
  // Update encoder
  int tmp = encoder.read() / 4; // 1 detent == 4 positions
  encoder_pos = tmp - encoder_pos;
  last_enc_ticks = tmp; // save current encoder position

  // // encoder.update_encoder();
  // if (lcd.settings_mode) {
  //   // Check which direction you turned
  //   // encoder_pos = encoder.change;

  //   // Check if encoder ticks has changed
  //   if (encoder_pos != 0) enc_change = true;
  //   // if (encoder.change != 0) encoder.has_changed = true;
  //   else enc_change = false;

  //   if (!lcd.blinking_cursor) 
  //   {
  //     if (enc_change) {
  //       lcd.update_cursor_position(encoder_pos);
  //     }
  //   }  else if (lcd.active_screen == SCREEN::MAIN) 
  //   {
  //     if (enc_change) {
  //       if (lcd.current_line == 2) {
  //         // Update the displayed line
  //         lcd.save_menu_name(encoder_pos);
  //       }
  //     }
  //   } else if (lcd.active_screen == SCREEN::SUMMARY) 
  //   {
  //     if (enc_change) {
  //       if (lcd.current_line == 1) {
  //         lcd.en_backlight = !lcd.en_backlight;
  //         if (lcd.en_backlight) {lcd.backlight_on();}
  //         else {lcd.backlight_off();}

  //         // Save data and print it to LCD
  //         lcd.save_setting_data(false);
  //       }
  //       if (lcd.current_line == 2) {
  //         int tmp = lcd.backlight_color;
  //         tmp += encoder_pos;
  //         if (tmp >= lcd.NUM_COLORS) {tmp = 0;}
  //         if (tmp < 0) {tmp = lcd.NUM_COLORS-1;}
  //         lcd.set_backlight_color(tmp);
  //         lcd.save_setting_data(false);
  //       }
  //       if (lcd.current_line == 3) {
  //         if (lcd.source_data.max_current != 0) {
  //           lcd.source_data.max_current = 0;
  //         } else {
  //           lcd.source_data.max_current = 1;
  //         }
  //         lcd.save_setting_data(lcd.source_data.max_current);
  //       }
  //     }
  //   } else if (lcd.active_screen != SCREEN::MAIN) 
  //   {
  //     if (enc_change) {
  //       lcd.getSettings(lcd.active_screen);
  //       uint8_t tmp = lcd.get_settings(lcd.current_line);
  //       tmp += encoder_pos;
  //       if (tmp > 58) {tmp = 0;}
  //       if (tmp < 0) {tmp = 58;}
  //       lcd.save_setting_data(lcd.current_line, tmp, lcd.active_screen);
  //     } 
  //   }
  // }

  // // Refresh screen
  // if ((button_pressed || main_btn_pressed) || (menu_btn_pressed && !lcd.settings_mode)) {
  //   lcd_refresh = true;
  // }

  // Update screen based on the button press
  if (button_pressed) {
    // Save screen_idx as active screen
    lcd.active_screen = lcd.getScreenID(screen_idx);
    button_pressed = false;

    #ifdef LCD_DEBUG
    lcd.write_array("Button pressed", 3, 0);
    #endif
  }

  // if (main_btn_pressed) {
  //   // Only go back to main screen if pressing 
  //   if (lcd.active_screen == SCREEN::MAIN && lcd.settings_mode) {
  //     if (lcd.current_line == 1) {
  //       lcd.settings_mode = false;
  //     }
  //     if (lcd.current_line == 2) {
  //       lcd.active_screen = lcd.displayed_screens[lcd.menu_num];
  //       screen_idx = lcd.menu_num;
  //     }
  //   } else if (lcd.active_screen != SCREEN::MAIN) {
  //     lcd.active_screen = SCREEN::MAIN;
  //     screen_idx = 0;
  //   }
  //   main_btn_pressed = false;
  // }
  // if (menu_btn_pressed) {
  //   if (!lcd.settings_mode) {
  //     lcd.settings_mode = true;
  //     lcd.menu_num = 0;
  //     lcd.save_menu_name(lcd.menu_num);
  //     // lcd.change_screen(SCREEN::MAIN);
  //     screen_idx = SCREEN::MAIN;
  //     lcd.active_screen = SCREEN::MAIN;

  //     // Reset lcd screen when switching screens
  //   } else {
  //     if (lcd.blinking_cursor) {
  //       lcd.update_cursor_position(0);
  //     }
  //     lcd.blinking_cursor = !lcd.blinking_cursor;
  //   }
  //   menu_btn_pressed = false;
  // }

  // // If the main button is held go to main screen and exit settings mode
  // if (main_button.button_held && lcd.settings_mode) {
  //   // Dsable settings mode and blinking cursor
  //   lcd.settings_mode = false;
  //   lcd.blinking_cursor = false;

  //   // Change the active screen back to main
  //   lcd.active_screen = SCREEN::MAIN;
  //   screen_idx = 0;
  // }

  // // Update ui data
  // lcd.ui_data.lcd_mode = lcd.settings_mode;
  // lcd.ui_data.backlight_on = lcd.en_backlight;
  // lcd.ui_data.backlight_color = lcd.backlight_color;

  // // Send data to MCU
  // lcd.sendInputData(lcd.ui_data);

  // Refresh lcd
  // lcd.refresh(lcd_refresh);
  lcd.refresh(true);
}

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
  interrupts();  // Turn interrupts on, and let's go

  // Init LCD
  delay(100);
  lcd.init(lcd_config);

  // Get Ready for test
  delay(2000);
  lcd.clear();
}

void loop()
{
  wdt_reset();
  // Poll Inputs
  ui_update();

  // // Find MCU
  // int nDevices = 0;
  // char num_str[5];
  // lcd.clear();
  // lcd.write_array("I2C Testing",0,0);
  // lcd.write_array("Current Device: ",1,0);
  // for (uint8_t address = 0; address < 127; address++) {
  //   // Set the slave address to the value
  //   lcd.i2c_address = address;

  //   // Print address
  //   snprintf(num_str, 5, "%3d", lcd.i2c_address);
  //   lcd.locate(1, 16);
  //   lcd._lcd->write(num_str);

  //   lcd.lcd_scan();

  //   if (lcd.mcu_present) {
  //     snprintf(lcd.line_str, 21, "Device Found at 0x%02x", lcd.i2c_address);
  //     lcd.write_array(lcd.line_str,2,0);
  //     nDevices++;
  //     delay(2000);
  //   }
  //   delay(100);
  // }

  // if (nDevices == 0) {
  //   lcd.write_array("No devices found",2,0);
  // }

  // delay(5000);
}
