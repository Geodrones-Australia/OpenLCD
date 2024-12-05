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
#include "GPIO.h"
#include "multifuel_button.h"
#include "Encoder.h" 

// Define Button pins
#define LEFT_BUTTON BOARD::D0
#define RIGHT_BUTTON BOARD::D10
#define MAIN_BUTTON BOARD::D1
#define MENU_BUTTON BOARD::D13
Multifuel_Button<LEFT_BUTTON,50> left_button;
Multifuel_Button<MAIN_BUTTON,50> main_button;
Multifuel_Button<RIGHT_BUTTON,50> right_button; // super bouncy for some reason
Multifuel_Button<MENU_BUTTON,50> menu_button; // rotary button isn't as bouncy

// Button status
int screen_idx = 0;
bool button_pressed;
bool left_btn_pressed;
bool right_btn_pressed;
bool main_btn_pressed;
bool menu_btn_pressed;
uint32_t left_time;
uint32_t right_time;
uint32_t menu_time;
uint32_t main_time;
int left_count = 0;
int right_count = 0;
int main_count = 0;
int menu_count = 0;


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
int MAX_SETTING = 58;

// Define functions
void button_tick(void);
void ui_update(void);
int sign(int x); // return sign of x (-1 or 1)
int wrap(int x, int low_bound, int upper_bound);
int update_value(int val, int change, int max = 0, int min = 0);

int sign(int x) {
    return (x > 0) - (x < 0);
}

int wrap(int x, int low_bound, int upper_bound)
{
    int range_size = upper_bound - low_bound + 1;

    if (x < low_bound)
        x += range_size * ((low_bound - x) / range_size + 1);

    return low_bound + (x - low_bound) % range_size;
}


int update_value(int val, int change, int max, int min) {
  if (change != 0) {
    val += sign(change);
  }

  // ensure its within the bound of max_setting and rollover both directions if it is
  if ((max - min) > 0) {
    val = wrap(val, min, max);
  }
  return val;
}


void button_tick(void) {
  // Set flags as false
  button_pressed = false;
  left_btn_pressed = false;
  menu_btn_pressed = false;
  main_btn_pressed = false;
  right_btn_pressed = false;

  // Check buttons
  if (right_button.update_btn() && !lcd.blinking_cursor) {screen_idx++; button_pressed = true; right_btn_pressed = true; right_count++;}
  if (left_button.update_btn() && !lcd.blinking_cursor) {screen_idx--; button_pressed = true; left_btn_pressed = true; left_count++;}
  if (main_button.update_btn() && !lcd.blinking_cursor) {main_btn_pressed = true; main_count++;} 
  if (menu_button.update_btn()) {menu_btn_pressed = true; menu_count++;}

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
  encoder_pos = tmp - last_enc_ticks;
  last_enc_ticks = tmp; // save current encoder position

  // encoder.update_encoder();
  if (lcd.settings_mode) {
    // Check which direction you turned
    // encoder_pos = encoder.change;

    // Check if encoder ticks has changed
    if (encoder_pos != 0) enc_change = true;
    // if (encoder.change != 0) encoder.has_changed = true;
    else enc_change = false;

    if (!lcd.blinking_cursor) 
    {
      if (enc_change) {
        lcd.update_cursor_position(encoder_pos);
      }
    }  else if (lcd.active_screen == SCREEN::MAIN) 
    {
      if (enc_change) {
        if (lcd.current_line == 2) {
          // Update the displayed line
          lcd.save_menu_name(encoder_pos);
        }
      }
    } else if (lcd.active_screen == SCREEN::SUMMARY) 
    {
      if (enc_change) {
        if (lcd.current_line == 1) {
          lcd.en_backlight = !lcd.en_backlight;
          if (lcd.en_backlight) {lcd.backlight_on();}
          else {lcd.backlight_off();}

          // Save data and print it to LCD
          lcd.save_setting_data(false);
        }
        if (lcd.current_line == 2) {
          int tmp = update_value(lcd.backlight_color, encoder_pos, lcd.NUM_COLORS-1);
          lcd.set_backlight_color(tmp);
          lcd.save_setting_data(false);
        }
        if (lcd.current_line == 3) {
          int tmp = update_value(lcd.contrast, encoder_pos, 60);
          lcd.changeContrast(tmp);
          lcd.save_setting_data(tmp, lcd.active_screen);
        }
      }
    } else if (lcd.active_screen != SCREEN::MAIN) 
    {
      if (enc_change) {
        lcd.getSettings(lcd.active_screen);
        int tmp = lcd.get_settings(lcd.current_line);
        tmp = update_value(tmp, encoder_pos, MAX_SETTING);
        lcd.save_setting_data(lcd.current_line, tmp, lcd.active_screen);
      } 
    }
  }

  // Refresh screen when changing screens 
  if ((button_pressed || main_btn_pressed) || (menu_btn_pressed && !lcd.settings_mode)) {
    lcd_refresh = true;
  } else {
    lcd_refresh = false;
  }

  // Update screen based on the button press
  if (button_pressed) {
    // Save screen_idx as active screen
    lcd.active_screen = lcd.getScreenID(screen_idx);
    button_pressed = false;
    lcd.current_line = 1;
    #ifdef LCD_DEBUG
    lcd.write_array("Button pressed", 3, 0);
    #endif
  }

  if (main_btn_pressed) {
    // Only go back to main screen if pressing 
    if (lcd.active_screen == SCREEN::MAIN && lcd.settings_mode) {
      if (lcd.current_line == 1) {
        lcd.settings_mode = false;
      }
      if (lcd.current_line == 2) {
        lcd.active_screen = lcd.displayed_screens[lcd.menu_num];
        screen_idx = lcd.menu_num;
      }
    } else if (lcd.active_screen != SCREEN::MAIN) {
      lcd.active_screen = SCREEN::MAIN;
      screen_idx = 0;
      lcd.current_line = 1;
    }
    main_btn_pressed = false;
  }

  if (menu_btn_pressed) {
    if (!lcd.settings_mode) {
      lcd.settings_mode = true;
      lcd.menu_num = 1;
      lcd.save_menu_name(0);
      // lcd.change_screen(SCREEN::MAIN);
      screen_idx = 0;
      lcd.active_screen = SCREEN::MAIN;

      // Reset lcd screen when switching screens
    } else {
      if (lcd.blinking_cursor) {
        lcd.update_cursor_position(0);
      }
      lcd.blinking_cursor = !lcd.blinking_cursor;
    }
    menu_btn_pressed = false;
  }

  // Update ui data
  lcd.ui_data.contrast = lcd.contrast;
  lcd.ui_data.lcd_mode = lcd.settings_mode;
  lcd.ui_data.backlight_on = lcd.en_backlight;
  lcd.ui_data.backlight_color = lcd.backlight_color;
  

  // Refresh lcd
  lcd.refresh(lcd_refresh);
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
  interrupts();  // Turn interrupts on, and let's go

  // Init LCD
  delay(100);
  lcd.init(lcd_config);

  // // Get Ready for test
  // int num_loops = 100;
  // lcd.clear();
  // lcd.write_array("Testing command time");
  // delay(1000);
  // unsigned long start = millis();
  // for (int i = 0; i < num_loops; i++) {
  //   // Create command structure
  //   lcd.getScreen(SCREEN::CORELLA_IN_DETAILS);
  // }
  // int print_time = millis() - start;
  // lcd.clear();

  // snprintf(lcd.line_str, lcd.lcd_str_width, "Loops: %d", num_loops);
  // lcd.write_array(lcd.line_str,0,0);
  // snprintf(lcd.line_str, lcd.lcd_str_width, "time: %dms", print_time);
  // lcd.write_array(lcd.line_str,1,0);
  // char num_str[5];
  // double loop_time = double(print_time) / num_loops;
  // dtostrf(loop_time, 5, 2, num_str);
  // snprintf(lcd.line_str, lcd.lcd_str_width, "time/loop: %sms", num_str);
  // lcd.write_array(lcd.line_str,2,0);
  // double rate = 800 / loop_time;
  // // double rate = 72 / loop_time;
  // // double rate = 20 / loop_time;
  // dtostrf(rate, 5, 1, num_str);
  // snprintf(lcd.line_str, lcd.lcd_str_width, "Rate: %skb/s", num_str);
  // lcd.write_array(lcd.line_str,3,0);
  // while(1);
  lcd.clear();
  lcd.write_array("Button test");
  left_time = left_button.timestamp();
  right_time = right_button.timestamp();
  main_time = main_button.timestamp();
  menu_time = menu_button.timestamp();
}

void loop()
{
  // // Poll Inputs
  ui_update();
}
