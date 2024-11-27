#include "lcd.h"



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
    i2c_address = config.i2c_address;

    // reset active screen
    active_screen = SCREEN::MAIN;

    // Compute line width
    lcd_str_width = LCD_COLLUMNS + 1;

    // Add data to status vector
    lcd_present = true;

    // Initialise data_vector
    data_vector[SCREEN::LEAD_DETAILS]        = settings_data(SCREEN::LEAD_DETAILS);
    data_vector[SCREEN::LIPO_DETAILS]        = settings_data(SCREEN::LIPO_DETAILS);
    data_vector[SCREEN::CORELLA_IN_DETAILS]  = settings_data(SCREEN::CORELLA_IN_DETAILS); 
    data_vector[SCREEN::CORELLA_EXT_DETAILS] = settings_data(SCREEN::CORELLA_EXT_DETAILS);
    data_vector[SCREEN::SOLAR_DETAILS]       = settings_data(SCREEN::SOLAR_DETAILS);
    data_vector[SCREEN::GENERATOR_DETAILS]   = settings_data(SCREEN::GENERATOR_DETAILS);
    data_vector[SCREEN::ACDC_DETAILS]        = settings_data(SCREEN::ACDC_DETAILS); 
    data_vector[SCREEN::DCDC_DETAILS]        = settings_data(SCREEN::DCDC_DETAILS);
    data_vector[SCREEN::CHARGER_DETAILS]     = settings_data(SCREEN::CHARGER_DETAILS);
    data_vector[SCREEN::VARDC_DETAILS]       = settings_data(SCREEN::VARDC_DETAILS);
    data_vector[SCREEN::INVERTER_DETAILS]    = settings_data(SCREEN::INVERTER_DETAILS);
    data_vector[SCREEN::LEAD_STEPUP_DETAILS] = settings_data(SCREEN::LEAD_STEPUP_DETAILS);
    data_vector[SCREEN::MAIN]                = settings_data(SCREEN::MAIN); 
    data_vector[SCREEN::SUMMARY]             = settings_data(SCREEN::SUMMARY);

    // Setup Menu names
    menu_names[SCREEN::LEAD_DETAILS]        = "Lead Settings     ";
    menu_names[SCREEN::LIPO_DETAILS]        = "Lipo Settings     ";
    menu_names[SCREEN::CORELLA_IN_DETAILS]  = "BATT(INT) Settings"; 
    menu_names[SCREEN::CORELLA_EXT_DETAILS] = "BATT(EXT) Settings";
    menu_names[SCREEN::SOLAR_DETAILS]       = "Solar Settings    ";
    menu_names[SCREEN::GENERATOR_DETAILS]   = "Generator Settings";
    menu_names[SCREEN::ACDC_DETAILS]        = "ACDC Settings     "; 
    menu_names[SCREEN::DCDC_DETAILS]        = "DCDC Settings     ";
    menu_names[SCREEN::CHARGER_DETAILS]     = "Charger Settings  ";
    menu_names[SCREEN::VARDC_DETAILS]       = "VARDC Settings    ";
    menu_names[SCREEN::INVERTER_DETAILS]    = "Inverter Settings ";
    menu_names[SCREEN::LEAD_STEPUP_DETAILS] = "Lead Step Settings";
    menu_names[SCREEN::MAIN]                = "Main Settings     "; 
    menu_names[SCREEN::SUMMARY]             = "Module Settings   ";

    // Add screens you are displaying to vector
    displayed_screens[0] = SCREEN::MAIN;
    displayed_screens[1] = SCREEN::SUMMARY;
    for (int i = 0; i < (NUM_DISPLAY_SCREENS-2); i++) {
        displayed_screens[i+2] = static_cast<SCREEN>(i);
    }
    
    // Update Active screen
    active_screen = SCREEN::MAIN; // First value of the screen array is the main screen

    // Grab initial setting data
    for (int i = 0; i < NUM_DISPLAY_SCREENS; ++i) {
        // Get setting for source
        getSettings(displayed_screens[i]);
    }

    // Initialise LCD
    startup();
}

void MULTIFUEL_LCD::reset_lcd() {
    // Set active screen back to main screen
    active_screen = SCREEN::MAIN;

    // Reset LCD board
    #ifdef UI_BOARD_V1
        digitalWrite(reset_pin, HIGH);
        _lcd.begin(*_i2c, DOGM204);
    #elif defined(UI_BOARD_V2)
        clear();
        startup();
    #endif
}

void MULTIFUEL_LCD::sendCommand(Command cmd) {
    _i2c->beginTransmission(i2c_address);
    _i2c->write((uint8_t *)&cmd, COMMAND_LEN);
    _i2c->endTransmission ();
}

void MULTIFUEL_LCD::sendCommand(Command cmd, uint8_t *buffer, size_t size) {
    // Send the command
    sendCommand(cmd);

    // Get Response
    _i2c->requestFrom(i2c_address, size);
    
    size_t idx = 0;
    while (_i2c->available()) {
        if (idx < size) {
            buffer[idx] = _i2c->read();
        }
        ++idx;
    };
}

void MULTIFUEL_LCD::getConfig() {
    // Create command structure
    Command cmd(COMMANDS::GET_CONFIG, active_screen, 0, 0, 0);

    // Send command
    sendCommand(cmd, (uint8_t *)&ui_data, sizeof((uint8_t *)&ui_data));
}

void MULTIFUEL_LCD::getSettings(uint8_t num) {
    // Create command structure
    Command cmd(COMMANDS::GET_SETTINGS, active_screen, 0, 0, 0);

    // Send command
    sendCommand(cmd, (uint8_t *)&data_vector[num], sizeof((uint8_t *)&data_vector[num]));
}

void MULTIFUEL_LCD::getScreen(uint8_t num) {
    // Create command structure
    Command cmd(COMMANDS::GET_SCREEN, active_screen, 0, 0, 0);

    // Send command
    sendCommand(cmd, (uint8_t *)&lcd_str, sizeof(lcd_str));
}

void MULTIFUEL_LCD::sendInputData(input_data data) {
    // Create command structure
    Command cmd(COMMANDS::SEND_INPUT_DATA, data.screen_number, data.lcd_mode, data.backlight_on, data.backlight_color);

    // Send command
    sendCommand(cmd);
}

void MULTIFUEL_LCD::sendSettings(settings_data data) {
    // Create command structure
    Command cmd(COMMANDS::SEND_INPUT_DATA, data.screen_number, data.max_voltage, data.max_current, data.max_temperature);

    // Send command
    sendCommand(cmd);
}

SCREEN MULTIFUEL_LCD::getScreenID(int &i) {
    // Ensure integer is within the bounds of the screen ids
    if (i >= NUM_DISPLAY_SCREENS) {i = 0;};
    if (i < 0) {i = NUM_DISPLAY_SCREENS - 1;};
    // return screen from integer
    return static_cast<SCREEN>(displayed_screens[i]);
}

uint8_t MULTIFUEL_LCD::get_settings(int line) {
  // Return the corrrect setting given a line
  uint8_t src_setting = 0;

  // get setting from data vector
  if (current_line == 1) {
    src_setting = data_vector[active_screen].max_voltage;
  } else if (current_line == 2 ) {
    src_setting = data_vector[active_screen].max_current;
  } else if (current_line == 3 ) {
    src_setting = data_vector[active_screen].max_temperature;
  }

  // return setting
  return src_setting;
}
void MULTIFUEL_LCD::update_cursor_position(int change) {
    // Erase cursor from every line
    write_array(cursor_off, 1, 0);
    write_array(cursor_off, 2, 0);
    write_array(cursor_off, 3, 0);

    // Compute new cursor line
    int dir;
    if (change < 0) dir = -1; // 
    else dir = 1;

    if (change != 0) current_line += dir * (change / change); // makes change +- 1

    // Ensure current line is within boundaries
    int max_line = 3;
    if (active_screen == SCREEN::MAIN) max_line = 2; // if on the main settings page there are only 2 settings

    // Update current line
    if (current_line < 1) {current_line = max_line;}
    if (current_line > max_line) {current_line = 1;}

    // Write cursor to line
    write_array(cursor_off, current_line, 0);
}

void MULTIFUEL_LCD::save_menu_name(int change) {
    // Update menu num
    int dir;
    if (change < 0) dir = -1; // keep the change negative
    else dir = 1;

    if (change != 0) menu_num += dir * (change / change); // make usre change is always +- 1

    // Make sure the menu number is within boundaries
    if (menu_num >= NUM_DISPLAY_SCREENS) {menu_num = 1;}
    if (menu_num < 1) {menu_num = NUM_DISPLAY_SCREENS-1;}

    // Get the screen number
    SCREEN screen_num = displayed_screens[menu_num];

    // Format the screen
    char num_txt[3];
    snprintf(num_txt, sizeof(num_txt), "%2d", menu_num-1); // Update the menu number
    write_array(num_txt, 2, 19);
    write_array(menu_names[screen_num], 3, 0);
}

void MULTIFUEL_LCD::save_setting_data(int idx, uint8_t new_setting, SCREEN lcd_screen, bool autoprint) {
    // Get information from screen_number data structure
    char update_str[5];
    double disp_setting;
    disp_setting = PosZero(new_setting, lcd_precision);

    // Save screen data
    if (current_line == 1) {
      data_vector[lcd_screen].max_voltage = new_setting;
    } else if (current_line == 2 ) {
      data_vector[lcd_screen].max_current = new_setting;
    } else if (current_line == 3 ) {
      data_vector[lcd_screen].max_temperature = new_setting;
    }
    
    // Print
    snprintf(update_str, sizeof(update_str), "%04.1f", disp_setting);
    if (autoprint) write_array(update_str, current_line, 15);
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
    } else if (current_line == 3) {
        if (val) {
            // Save screen data
            write_array(on_str, current_line,17);
        }
        else {
            // Save screen data
            write_array(off_str, current_line,17);
        }
        data_vector[SCREEN::MAIN].max_temperature = val; // store protection value in max tempertature setting
    }
}

void MULTIFUEL_LCD::clear() {
    // Cleat the LCD
    _lcd->clear();
}

void MULTIFUEL_LCD::write_array(const char *str, uint8_t row, uint8_t col) {
	// size_t size = strlen(str);
    // // _lcd.write(str);

    // _i2c->beginTransmission(i2c_address); // transmit to device
	// buffer_size = _i2c->write((const uint8_t *)str, int(size));          //while
    // _i2c->endTransmission(); //Stop transmission
    // Locate
    locate(row, col);
    _lcd->write(str);
}

void MULTIFUEL_LCD::locate(uint8_t row, uint8_t column) {
    _lcd->setCursor(column, row);
}

void MULTIFUEL_LCD::full_screen_refresh(bool FORCE)
{
  if (FORCE) {
    // Get current screent
    getScreen(active_screen);

    // Print the screen
    _lcd->write(lcd_str);
  }

  // Update cursor
  if (settings_mode && blinking_cursor) {blink_cursor();}
}

void MULTIFUEL_LCD::refresh(bool FORCE)
{
    if ((millis() - last_update) > LCD_UPDATE_RATE)
    {
        full_screen_refresh(FORCE);  
        last_update = millis();
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

void MULTIFUEL_LCD::set_backlight_color(int backlight_idx) {
    backlight_color = backlight_idx;
    if (en_backlight) {set_backlight_color(lcd_colors[backlight_color]);}
}

void MULTIFUEL_LCD::set_backlight_color(byte r, byte g, byte b) {
    if (en_backlight) {
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
    }
}

void MULTIFUEL_LCD::set_backlight_color(unsigned long color) {
    // convert from hex triplet to byte values
    byte r = (color >> 16) & 0x0000FF;
    byte g = (color >> 8) & 0x0000FF;
    byte b = color & 0x0000FF;
    if (en_backlight) {set_backlight_color(r,g,b);}
}

//**************PRIVATE FUNCTIONS**************/
void MULTIFUEL_LCD::startup() {
    // Setup LCD
    setupLCD(); //Initialize the LCD

    setupContrast(); //Set contrast

    setupBacklight(); //Turn on any backlights

    setupPower(); //Power down peripherals that we won't be using
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