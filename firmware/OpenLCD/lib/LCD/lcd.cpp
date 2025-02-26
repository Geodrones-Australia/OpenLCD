#include "lcd.h"

// Default character arrays
const char * const start_str   PROGMEM = "Multifuel Startup   Setting Up Unit     Getting Config      Elapse Time:   00.0s";
const char * const on_str      PROGMEM = " ON"; 
const char * const off_str     PROGMEM = "OFF";   
const char * const cursor_on   PROGMEM = ">";
const char * const cursor_off  PROGMEM = " ";
const char * const I2C_ERROR   PROGMEM = "I2C COMMS FAILED    ";
const char * const DEBUG_LINE  PROGMEM = "    DEBUG SCREEN    ";

// LCD Properties
const int LCD_LINES = 4;
const int LCD_COLLUMNS = 20;
const int LCD_SIZE = 80;


MULTIFUEL_LCD::MULTIFUEL_LCD() {
}

MULTIFUEL_LCD::~MULTIFUEL_LCD() {

}

MULTIFUEL_LCD::MULTIFUEL_LCD(multifuel_lcd_config config) {
    init(config);
}

void MULTIFUEL_LCD::init(multifuel_lcd_config config) {
    // Get data from config
    _lcd = &config._lcd;
    _i2c = &config._i2c;
    i2c_address = config.i2c_adr;

    // reset active screen
    active_screen = SCREEN::MAIN;

    // Compute line width
    lcd_str_width = LCD_COLLUMNS + 1;

    // Initialise lcd buffer with zeros
    for (int i = 0; i < LCD_SIZE; ++i) {
      lcd_buff[i] = start_str[i];
    }

    // Add data to status vector
    lcd_present = true;

    // Add screens you are displaying to vector
    displayed_screens[0] = SCREEN::MAIN;
    displayed_screens[1] = SCREEN::SUMMARY;
    for (int i = 0; i < (NUM_DISPLAY_SCREENS-2); i++) {
        displayed_screens[i+2] = static_cast<SCREEN>(i);
    }

    // Save values to setting screens
    settings_screens[0] = SCREEN::MAIN;
    settings_screens[1] = SCREEN::SUMMARY;
    settings_screens[2] = SCREEN::CORELLA_IN_DETAILS;
    settings_screens[3] = SCREEN::CORELLA_EXT_DETAILS;
    settings_screens[4] = SCREEN::LIPO_DETAILS;
    settings_screens[5] = SCREEN::GENERATOR_DETAILS;

    // Setup Menu names
    menu_names[SCREEN::LEAD_DETAILS]        = "Lead Settings     ";
    menu_names[SCREEN::LIPO_DETAILS]        = "Lipo 1 Settings   ";
    menu_names[SCREEN::CORELLA_IN_DETAILS]  = "BATT(INT) Settings"; 
    menu_names[SCREEN::CORELLA_EXT_DETAILS] = "BATT(EXT) Settings";
    menu_names[SCREEN::SOLAR_DETAILS]       = "Solar Settings    ";
    menu_names[SCREEN::GENERATOR_DETAILS]   = "Lipo 2 Settings   ";
    menu_names[SCREEN::ACDC_DETAILS]        = "ACDC Settings     "; 
    menu_names[SCREEN::DCDC_DETAILS]        = "DCDC Settings     ";
    menu_names[SCREEN::CHARGER_DETAILS]     = "Charger Settings  ";
    menu_names[SCREEN::VARDC_DETAILS]       = "VARDC Settings    ";
    menu_names[SCREEN::INVERTER_DETAILS]    = "Inverter Settings ";
    menu_names[SCREEN::LEAD_STEPUP_DETAILS] = "Lead Step Settings";
    menu_names[SCREEN::MAIN]                = "Main Settings     "; 
    menu_names[SCREEN::SUMMARY]             = "LCD Settings      ";

    // Initialise LCD
    startup();
}

void MULTIFUEL_LCD::reset_lcd() {
    // Set active screen back to main screen
    clear();
    startup();
}

void MULTIFUEL_LCD::sendCommand(Command cmd) {
  if (mcu_present) {
    _i2c->beginTransmission(i2c_address);
    for (int i = 0; i < COMMAND_LEN; i++) {
      _i2c->write(cmd.buffer[i]);
    }
    _i2c->endTransmission();
  }

  last_cmd.cmd = cmd.cmd;
  last_cmd.buffer[0] = cmd.buffer[0];
  last_cmd.buffer[1] = cmd.buffer[1];
  last_cmd.buffer[2] = cmd.buffer[2];
  last_cmd.buffer[3] = cmd.buffer[3];
  last_cmd.buffer[4] = cmd.buffer[4];
}

bool MULTIFUEL_LCD::twiReceive(size_t response_size, bool stop_bit)
{
  if (mcu_present) {
    i2c_resp_size = _i2c->requestFrom(i2c_address, uint8_t(response_size), uint8_t(stop_bit));
    for (size_t idx = 0; idx < response_size; idx++) {
        buffer[idx] = _i2c->read();
    };
    return true;
  } else {
    return false;
  }
};

void MULTIFUEL_LCD::clearBuffer(int quantity) {
  // Clear array to zeros
  if (quantity == 0) {
    for (int i = 0; i < BUFFER_LENGTH; ++i) {
      buffer[i] = 0;
    }
  } else {
    for (int i = 0; i < quantity; ++i) {
      buffer[i] = 0;
    }
  }
}
    
void MULTIFUEL_LCD::getConfig(bool autoset) {
    // Create command structure
    Command cmd(COMMANDS::GET_CONFIG, active_screen, 0, 0, 0);

    // Send command
    sendCommand(cmd);

    // Twi Receive
    if (twiReceive(4)) {
      // Parse buffer
      ui_data.contrast = buffer[0];
      ui_data.lcd_mode = buffer[1];
      ui_data.backlight_on = buffer[2];
      ui_data.backlight_color = buffer[3];

      // Save buffer
      last_cmd.cmd = cmd.cmd;
      last_cmd.buffer[1] = buffer[0];
      last_cmd.buffer[2] = buffer[1];
      last_cmd.buffer[3] = buffer[2];
      last_cmd.buffer[4] = buffer[3];

      // Clear buffer
      clearBuffer(4);

      // Save backlight properties
      if (ui_data.backlight_on != 0) {
        en_backlight = true;
      } else {
        en_backlight = false;
      }
      if (ui_data.backlight_color >= NUM_COLORS) {
        backlight_color = 0;
      } else {
        backlight_color = ui_data.backlight_color;
      }

      // Change contrast
      contrast = ui_data.contrast;

      // Apply settings to lcd
      if (autoset) {
        changeContrast(contrast);
      
        // Chnage the configuration of the lcd
        configure();
      }
    }
}

void MULTIFUEL_LCD::getSettings(uint8_t num) {
    // Create command structure
    Command cmd(COMMANDS::GET_SETTINGS, num, 0, 0, 0);

    // Send command
    sendCommand(cmd);

    // Twi Receive
    if (twiReceive(4)) {
      // Parse buffer
      source_data.screen_number = buffer[0];
      source_data.min_voltage = buffer[1];
      source_data.max_voltage = buffer[2];
      source_data.max_current = buffer[3];

      // Update last cmd
      last_cmd.cmd = cmd.cmd;
      last_cmd.buffer[1] = buffer[0];
      last_cmd.buffer[2] = buffer[1];
      last_cmd.buffer[3] = buffer[2];
      last_cmd.buffer[4] = buffer[3];

      // Clear buffer
      clearBuffer(4);
    }
}

void MULTIFUEL_LCD::getScreen(uint8_t num) {
    // Twi Receive
    int suc = 0;
    int start_idx = 0;
    int end_idx = LCD_COLLUMNS;
    for (int line = 0; line < LCD_LINES; ++line) {
      // Create command structure
      Command cmd(COMMANDS::GET_SCREEN, num, line, 0, 0);

      // Send command
      sendCommand(cmd);
      
      if (twiReceive(LCD_COLLUMNS)) {
        // Parse buffer
        start_idx = line * LCD_COLLUMNS;
        end_idx = (line + 1) * LCD_COLLUMNS;
        for (int i = start_idx; i < end_idx; ++i) {
          if (buffer[i-start_idx] != 0) {
            lcd_buff[i] = buffer[i-start_idx];
          }
        }

        // Update last cmd
        last_cmd.cmd = cmd.cmd;
        last_cmd.buffer[1] = buffer[0];
        last_cmd.buffer[2] = buffer[1];
        last_cmd.buffer[3] = buffer[2];
        last_cmd.buffer[4] = buffer[3];

        // Clear buffer
        clearBuffer(LCD_COLLUMNS);
        ++suc;
      }
    }
    i2c_resp_size = suc * LCD_COLLUMNS;
}

void MULTIFUEL_LCD::sendInputData(input_data data) {
    // Create command structure
    Command cmd(COMMANDS::SEND_INPUT_DATA, data.contrast, data.lcd_mode, data.backlight_on, data.backlight_color);

    // Send command
    sendCommand(cmd);
}

void MULTIFUEL_LCD::sendSettings(settings_data data) {
    // Create command structure
    Command cmd(COMMANDS::SEND_SETTINGS, data.screen_number, data.min_voltage, data.max_voltage, data.max_current);

    // Send command
    sendCommand(cmd);
}

SCREEN MULTIFUEL_LCD::getScreenID(int &i) {
    // Ensure integer is within the bounds of the screen ids
    if (!settings_mode) {
      if (i >= NUM_DISPLAY_SCREENS) {i = 0;};
      if (i < 0) {i = NUM_DISPLAY_SCREENS - 1;};

      // return screen from integer
      return static_cast<SCREEN>(displayed_screens[i]);
    } else {
      if (i >= NUM_SETTINGS_SCREENS) {i = 1;};
      if (i < 1) {i = NUM_SETTINGS_SCREENS - 1;};

      // return screen from integer
      return static_cast<SCREEN>(settings_screens[i]);
    }
}

int MULTIFUEL_LCD::get_settings(int line) {
  // Return the corrrect setting given a line
  int src_setting = 0;

  // get setting from data vector
  if (current_line == 1) {
    src_setting = source_data.min_voltage;
  } else if (current_line == 2 ) {
    src_setting = source_data.max_voltage;
  } else if (current_line == 3 ) {
    src_setting = source_data.max_current;
  }

  // return setting
  return src_setting;
}
void MULTIFUEL_LCD::update_cursor_position(int change) {
    // Ensure current line is within boundaries
    int max_line = 3;
    if (active_screen == SCREEN::MAIN) max_line = 2; // if on the main settings page there are only 2 settings
    
    // Erase cursor from every line
    for (int i = 1; i <= max_line; i++) {
      write_array(cursor_off, i, 0);
    }

    // Compute new cursor line
    int dir;
    if (change < 0) dir = -1; // 
    else dir = 1;

    if (change != 0) current_line += dir * (change / change); // makes change +- 1

    // Update current line
    if (current_line < 1) {current_line = max_line;}
    if (current_line > max_line) {current_line = 1;}

    // Write cursor to line
    write_array(cursor_on, current_line, 0);
}

void MULTIFUEL_LCD::save_menu_name(int change) {
    // Update menu num
    int dir;
    if (change < 0) dir = -1; // keep the change negative
    else dir = 1;

    if (change != 0) menu_num += dir; // make usre change is always +- 1

    // Get the screen number
    SCREEN screen_num = getScreenID(menu_num);
    if (menu_num < 1) menu_num = 1;

    // Format the screen
    char num_txt[3];
    snprintf(num_txt, sizeof(num_txt), "%2d", menu_num-1); // Update the menu number
    write_array(num_txt, 2, 18);
    snprintf(line_str, lcd_str_width, menu_name_format, menu_names[screen_num]); // Update the menu name
    write_array(line_str, 3, 0);
}

void MULTIFUEL_LCD::save_setting_data(int idx, uint8_t new_setting, SCREEN lcd_screen, bool autoprint) {
    // Save screen number to source data
    source_data.screen_number = active_screen;

    // Update setting data
    if (current_line == 1) {
      source_data.min_voltage = new_setting;
    } else if (current_line == 2 ) {
      source_data.max_voltage = new_setting;
    } else if (current_line == 3 ) {
      source_data.max_current = new_setting;
    }

    // Save data
    sendSettings(source_data);
    
    // Print
    char num_str[3];
    snprintf(num_str, 3, "%2u", new_setting);
    if (autoprint) write_array(num_str, current_line, 15);
}

void MULTIFUEL_LCD::save_setting_data(int new_setting, SCREEN lcd_screen) {
    // Get information from screen_number data structure
    char update_str[4];
    // Save screen data
    dtostrf(new_setting, 3, 0, update_str);

    // Send data to mcu to save
    ui_data.contrast = new_setting;
    
    // Update array
    write_array(update_str, current_line, 17);
}

void MULTIFUEL_LCD::save_setting_data(bool val, bool autoprint) {
    // Update the main module screen
    if (current_line == 1) {
        if (en_backlight) {
            // Save screen data
            write_array(on_str, current_line,17);
        }
        else {
            // Save screen data
            write_array(off_str, current_line,17);
        }
        ui_data.backlight_on = en_backlight;
    } else if (current_line == 2) {
        // Save screen data
        write_array(lcd_color_str[backlight_color], current_line,14);
        ui_data.backlight_color = backlight_color;
    }
}

void MULTIFUEL_LCD::clear() {
    // Cleat the LCD
    _lcd->clear();
}

void MULTIFUEL_LCD::write_array(const char *str, uint8_t row, uint8_t col) {
    locate(row, col);
    _lcd->write(str);
}

void MULTIFUEL_LCD::print_error(const char *str, uint8_t row, uint8_t col) {
    locate(row, col);
    _lcd->write(str);
}

void MULTIFUEL_LCD::write_array(uint8_t *arr, uint8_t row, uint8_t col) {
    locate(row, col);

    for (int i = 0; i < LCD_SIZE; ++i) {
      _lcd->write(arr[i]);
    }
}

void MULTIFUEL_LCD::locate(uint8_t row, uint8_t column) {
    _lcd->setCursor(column, row);
}

void MULTIFUEL_LCD::test_screen(bool DEBUG)
{
  // Get current screen
  getScreen(active_screen);
  // getSettings(SCREEN::DCDC_DETAILS);
  // getConfig();

  if (DEBUG) {
    // Print the screen
    #ifdef LCD_DEBUG
    write_array(DEBUG_LINE,0,0);
    
    // Write if MCU is found
    if (mcu_present) {
      write_array("OK", 0, 18);
    } else {
      write_array("ERR", 0, 17);
    }

    // Response length
    snprintf(line_str, lcd_str_width, "Response Length: %3d", i2c_resp_size);
    write_array(line_str, 1 ,0);

    // Check last command
    snprintf(line_str, lcd_str_width, "%3d,%3d,%3d,%3d,%3d", last_cmd.buffer[0], last_cmd.buffer[1],last_cmd.buffer[2], last_cmd.buffer[3], last_cmd.buffer[4]);
    write_array(line_str, 2 ,0);

    // How many times the screen has refreshed since start
    snprintf(line_str, lcd_str_width, "Refresh Count: %5d", debug_count);
    debug_count++;
    write_array(line_str, 3, 0);
    #else
    write_array(lcd_buff);
    #endif
  }
}

void MULTIFUEL_LCD::refresh(bool FORCE)
{
  // Measure refresh rate
  mes_rate = millis() - last_scr_update;

  // Update cursor
  if (settings_mode && blinking_cursor) {blink_cursor();}

  // Update rate
  if ((mes_rate > LCD_UPDATE_RATE) || (FORCE))
  {
      // Scan for the mcu
      lcd_scan();

      if (mcu_present && (mcu_present != prev_mode)) {
        // reset was detected
        delay(100);
        getConfig();
        prev_mode = mcu_present;

        // Return to main screen
        active_screen = SCREEN::MAIN;
        settings_mode = false;
      }

      // Update UI data
      sendInputData(ui_data);

      // If i've finished printing the old screen get a new screen, or forced update
      if ((FORCE) || (!print_block  && !settings_mode)) {
        // Get new data
        getScreen(active_screen); 

        // Unblock printing and reset position
        cur_row = 0;
        cur_col = 0;
        pos = 0;
        print_block = true;
      }

      last_scr_update = millis();
  } else if (print_block) {
    // Write next character from lcd
    locate(cur_row, cur_col);
    _lcd->write(lcd_buff[pos]);

    // Update position
    ++pos;
    if (pos < LCD_SIZE) {
      // Update row and collumn positions
      cur_row = pos / LCD_COLLUMNS;
      cur_col = pos - (cur_row * LCD_COLLUMNS);

      // Ensure the row and collumn are bounded
      cur_row = cur_row % LCD_LINES;
      cur_col = cur_col % LCD_COLLUMNS;
    } else {
      // We've finished printing the screen waiting until next refresh to print again
      // In settings mode we only print the screen once (we are updating it in real time)
      print_block = false;
    }
  }

}

void MULTIFUEL_LCD::blink_cursor() {

    // Make cursor blink
    if ((millis() - last_cursor_update) > CURSOR_BLINK_RATE) {
        if (show_cursor) {
            write_array(cursor_on, current_line, 0);
            show_cursor = false;
        } else {
            write_array(cursor_off, current_line, 0);
            show_cursor = true;
        }
        last_cursor_update = millis();
    }
    // _lcd.blink();
}

//Change the digital contrast
//Press a or z to adjust, x to exit
void MULTIFUEL_LCD::changeContrast(uint8_t val)
{
  // Save contrast to lcd class
  contrast = val;

  // Update eeprom contrast setting
  EEPROM.update(LOCATION_CONTRAST, contrast); //Store this new contrast

  //Go to this new contrast
  analogWrite(LCD_CONTRAST, contrast);
}

void MULTIFUEL_LCD::backlight_on() {
    unsigned long rgb = lcd_colors[backlight_color];
    // convert from hex triplet to byte values
    byte r = (rgb >> 16) & 0x0000FF;
    byte g = (rgb >> 8) & 0x0000FF;
    byte b = rgb & 0x0000FF;
    set_backlight_color(r,g,b);
}

void MULTIFUEL_LCD::backlight_off() {
   set_backlight_color(0,0,0);
}

void MULTIFUEL_LCD::set_backlight_color(int idx) {
  // make sure idx is in range
  backlight_color = idx % NUM_COLORS;

  // Set the backlight color
  if (en_backlight) backlight_on();
}

void MULTIFUEL_LCD::set_backlight_color(byte r, byte g, byte b) {
    //update red
    EEPROM.update(LOCATION_RED_BRIGHTNESS, r); //Record new setting
    analogWrite(BL_RW, 255 - r); //Controlled by PNP so reverse the brightness value

    //update green
    EEPROM.update(LOCATION_GREEN_BRIGHTNESS, g); //Record new setting
    analogWrite(BL_G, 255 - g); //Controlled by PNP so reverse the brightness value

    //update blue (SoftPWM)
    EEPROM.update(LOCATION_BLUE_BRIGHTNESS, b); //Record new setting
    //analogWrite(BL_B, 255 - brightness); //Controlled by PNP so reverse the brightness value
    SoftPWMSet(BL_B, b); //Controlled by software PWM. Reversed by SoftPWM
    delay(10);
}

//**************PRIVATE FUNCTIONS**************/
void MULTIFUEL_LCD::startup() {
    // Setup LCD
    setupLCD(); //Initialize the LCD

    setupContrast(); //Set contrast

    setupBacklight(); //Turn on any backlights

    setupPower(); //Power down peripherals that we won't be using

    // Setup I2C
    _i2c->begin();

    // Set i2c clock speed
    _i2c->setClock(400000UL);
    _i2c->setWireTimeout(5000);

    // Print startup message
    clear();
    petSafeDelay(100);
    write_array(lcd_buff, 0 ,0);

    // Wait a bit
    delay(1000);

    // Get Config, lcd mode should be false, wait until we get false before proceeding
    last_scr_update = millis();
    double count = 0;
    unsigned long loop_delay = 100;
    char num_str[7];

    // Set lcd mode to settings
    ui_data.lcd_mode = 1;

    while (!mcu_present) {
      petSafeDelay(loop_delay);
      count += loop_delay / 1000.0;
      dtostrf(count, 6, 1, num_str);
      write_array(num_str, 3 ,13);

      if (lcd_scan()) {
        delay(100); // wait a bit for the mcu to be ready
        getConfig();
        prev_mode = mcu_present;
      }
    }
    last_scr_update = millis();
    // write_array("Acquired Config     ", 3, 0);
    
    // Set Active screen to main
    active_screen = SCREEN::MAIN; 

    // Get current screen
    petSafeDelay(100);

    // Display current screem
    getScreen(SCREEN::MAIN);
    print_block = true;

    // wait until screen is fully printed
    while (print_block) {
      refresh();
    }

    // // Grab initial setting data
    // for (int i = 0; i < NUM_DISPLAY_SCREENS; ++i) {
    //     // Get setting for source
    //     getSettings(displayed_screens[i]);
    // }
}

bool MULTIFUEL_LCD::lcd_scan() {
    // Scan I2C bus
    byte error;

    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    _i2c->beginTransmission(i2c_address);
    error = _i2c->endTransmission();

    if (error == 0)
    {
      mcu_present = true;
      return true;
    }
    else
    {
      mcu_present = false;
      prev_mode = mcu_present;
      return false;
    }    
}

void MULTIFUEL_LCD::setupLCD()
{
  _lcd->begin(20, 4); //Setup the width and lines for this LCD
  _lcd->lineWrap(); // enable line wrap

  //Clear any characters in the frame buffer
  _lcd->clear();
}

void MULTIFUEL_LCD::setupContrast() {
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

//Look up and start the 3 backlight pins in analog mode
void MULTIFUEL_LCD::setupBacklight()
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

void MULTIFUEL_LCD::setupPower()
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

void MULTIFUEL_LCD::configure() {
    // Setup LCD backlight and contrast
    if (en_backlight) backlight_on();
    else backlight_off();
    // _lcd.setContrast(45);
}

// Format number
char * formatNumber(float val, signed char width, unsigned char prec, char *sout) {
  // Format float into char
  dtostrf(val, width, prec, sout);

  // Add leading zeros for nicer formatting
  for (size_t i = 0; i < strlen(sout); ++i) {
    if (sout[i]==' ') {sout[i] = '0';}
  }

  return sout;
}

// Delay
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
      TCCR0B = TCCR0B & (0b11111000 | mode);
    } else {
      TCCR1B = TCCR1B & (0b11111000 | mode);
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
    TCCR2B = TCCR2B & (0b11111000 | mode);
  }
}