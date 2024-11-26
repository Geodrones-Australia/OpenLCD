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

    // Add screen data to screen vector
    lcd_data[SCREEN::LEAD_DETAILS]        = &lead_screen;
    lcd_data[SCREEN::LIPO_DETAILS]        = &lipo_screen;
    lcd_data[SCREEN::CORELLA_IN_DETAILS]  = &corella_in_screen;
    lcd_data[SCREEN::CORELLA_EXT_DETAILS] = &corella_ext_screen;
    lcd_data[SCREEN::SOLAR_DETAILS]       = &solar_screen;
    lcd_data[SCREEN::GENERATOR_DETAILS]   = &generator_screen;
    lcd_data[SCREEN::ACDC_DETAILS]        = &acdc_screen;
    lcd_data[SCREEN::DCDC_DETAILS]        = &dcdc_screen;
    lcd_data[SCREEN::CHARGER_DETAILS]     = &charger_screen;
    lcd_data[SCREEN::VARDC_DETAILS]       = &vardc_screen;
    lcd_data[SCREEN::INVERTER_DETAILS]    = &inverter_screen;
    lcd_data[SCREEN::LEAD_STEPUP_DETAILS] = &lead_stepup_screen;
    lcd_data[SCREEN::MAIN]                = &main_screen;
    lcd_data[SCREEN::SUMMARY]             = &summary_screen;

    // Add setting data to screen vector
    settings_data[SCREEN::LEAD_DETAILS]        = &lead_settings_screen;
    settings_data[SCREEN::LIPO_DETAILS]        = &lipo_settings_screen;
    settings_data[SCREEN::CORELLA_IN_DETAILS]  = &corella_in_settings_screen;
    settings_data[SCREEN::CORELLA_EXT_DETAILS] = &corella_ext_settings_screen;
    settings_data[SCREEN::SOLAR_DETAILS]       = &solar_settings_screen;
    settings_data[SCREEN::GENERATOR_DETAILS]   = &generator_settings_screen;
    settings_data[SCREEN::ACDC_DETAILS]        = &acdc_settings_screen;
    settings_data[SCREEN::DCDC_DETAILS]        = &dcdc_settings_screen;
    settings_data[SCREEN::CHARGER_DETAILS]     = &charger_settings_screen;
    settings_data[SCREEN::VARDC_DETAILS]       = &vardc_settings_screen;
    settings_data[SCREEN::INVERTER_DETAILS]    = &inverter_settings_screen;
    settings_data[SCREEN::LEAD_STEPUP_DETAILS] = &lead_stepup_settings_screen;
    settings_data[SCREEN::MAIN]                = &main_settings_screen;
    settings_data[SCREEN::SUMMARY]             = &module_settings_screen;
    
    // Add sensor data to data vector
    data_vector[SCREEN::LEAD_DETAILS]        = lead;
    data_vector[SCREEN::LIPO_DETAILS]        = lipo;
    data_vector[SCREEN::CORELLA_IN_DETAILS]  = corella_in;
    data_vector[SCREEN::CORELLA_EXT_DETAILS] = corella_ext;
    data_vector[SCREEN::SOLAR_DETAILS]       = solar;
    data_vector[SCREEN::GENERATOR_DETAILS]   = generator;
    data_vector[SCREEN::ACDC_DETAILS]        = acdc;
    data_vector[SCREEN::DCDC_DETAILS]        = dcdc;
    data_vector[SCREEN::CHARGER_DETAILS]     = charger;
    data_vector[SCREEN::VARDC_DETAILS]       = vardc;
    data_vector[SCREEN::INVERTER_DETAILS]    = inverter;
    data_vector[SCREEN::LEAD_STEPUP_DETAILS] = leadstepup;
    data_vector[SCREEN::MAIN]                = main_screen_data;
    data_vector[SCREEN::SUMMARY]             = main_screen_data;

    // Add data to status vector
    status_vector[SOURCE_STATUS::NOTFOUND]    = off_line;
    status_vector[SOURCE_STATUS::ENABLED]     = enabled_line;
    status_vector[SOURCE_STATUS::DISABLED]    = disabled_line;
    status_vector[SOURCE_STATUS::DISABLED_OV] = OV_line;
    status_vector[SOURCE_STATUS::DISABLED_OC] = OC_line;
    status_vector[SOURCE_STATUS::DISABLED_UV] = UV_line;
    status_vector[SOURCE_STATUS::DISABLED_OT] = OT_line;
    status_vector[SOURCE_STATUS::RESET_RQ]    = RESET_line;
    status_vector[SOURCE_STATUS::UNIT_OK]     = UNIT_OK_line;
    status_vector[SOURCE_STATUS::NO_SOURCES]  = NO_SRCS_line;

    // Save menu names
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

    // Save source names
    source_names[SCREEN::LEAD_DETAILS]        = "Lead Acid         ";
    source_names[SCREEN::LIPO_DETAILS]        = "6S Lipo           ";
    source_names[SCREEN::CORELLA_IN_DETAILS]  = "BATT (INT)        "; 
    source_names[SCREEN::CORELLA_EXT_DETAILS] = "BATT (EXT)        ";
    source_names[SCREEN::SOLAR_DETAILS]       = "Solar Charger     ";
    source_names[SCREEN::GENERATOR_DETAILS]   = "Generator         ";
    source_names[SCREEN::ACDC_DETAILS]        = "ACDC              "; 
    source_names[SCREEN::DCDC_DETAILS]        = "DCDC              ";
    source_names[SCREEN::CHARGER_DETAILS]     = "Battery Charger   ";
    source_names[SCREEN::VARDC_DETAILS]       = "Variable DC       ";
    source_names[SCREEN::INVERTER_DETAILS]    = "Inverter          ";
    source_names[SCREEN::LEAD_STEPUP_DETAILS] = "12V Stepup        ";
    source_names[SCREEN::MAIN]                = "Multifuel Unit    "; 
    source_names[SCREEN::SUMMARY]             = "Multifuel Unit    ";

    // Add screens you are displaying to vector
    displayed_screens[0] = SCREEN::MAIN;
    displayed_screens[1] = SCREEN::SUMMARY;
    for (int i = 0; i < (NUM_DISPLAY_SCREENS-2); i++) {
        displayed_screens[i+2] = lcd_data[i]->screen_number;
    }

    // Format the title lines wth the right screen numbers
    char num_str[4];
    for (size_t i = 2; i < sizeof(displayed_screens); ++i) {
        // Get the screen number from the vector
        SCREEN tmp_num = static_cast<SCREEN>(displayed_screens[i]);

        // Format the screen number as a string
        int disp_num = tmp_num + 1;
        snprintf(num_str, sizeof(num_str), "%02d)", disp_num);

        // Get the screen data and update the line text
        for (size_t idx = 0; idx != sizeof(num_str) - 1; idx++) {
            lcd_data[tmp_num]->lines[0]->txt[idx] = num_str[idx];
            settings_data[tmp_num]->lines[0]->txt[idx] = num_str[idx];
        }
    }

    // Set LCD status as true
    lcd_present = true;

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

void MULTIFUEL_LCD::clear() {
    // Cleat the LCD
    _lcd->clear();
}

void MULTIFUEL_LCD::write_array(const char *str) {
	// size_t size = strlen(str);
    // // _lcd.write(str);

    // _i2c->beginTransmission(i2c_address); // transmit to device
	// buffer_size = _i2c->write((const uint8_t *)str, int(size));          //while
    // _i2c->endTransmission(); //Stop transmission
    buffer_size = _lcd->write(str);
}

void MULTIFUEL_LCD::nb_write_array(const char *str) {
	_lcd->write(str);
}

void MULTIFUEL_LCD::locate(uint8_t row, uint8_t column) {
    _lcd->setCursor(column, row);
}

void MULTIFUEL_LCD::fast_locate(uint8_t row, uint8_t column) {
    _lcd->setCursor(column, row);
}

void MULTIFUEL_LCD::save_sensor_data(int i, sensor_data new_data, bool FORCE) {
    // Update data in lcd
    data_vector[i].screen_number = new_data.screen_number;
    data_vector[i].status        = new_data.status;
    data_vector[i].voltage       = new_data.voltage; 
    data_vector[i].current       = new_data.current; 
    data_vector[i].power_source  = new_data.power_source; 
    data_vector[i].power_total   = new_data.power_total; 

    // Update Character strings
    save_sensor_data(data_vector[i], FORCE);
}

void MULTIFUEL_LCD::save_sensor_data(sensor_data new_data, bool autoprint) {
    // Get information from screen_number data structure
    SCREEN screen_number = new_data.screen_number;
    SOURCE_STATUS status = new_data.status;
    float voltage = PosZero(abs(new_data.voltage), lcd_precision);
    float current = PosZero(abs(new_data.current), lcd_precision);
    float power_source = PosZero(abs(new_data.power_source));
    float power_total  = PosZero(abs(new_data.power_total));
    float percentage  = PosZero(abs(new_data.percentage));
    float temperature  = PosZero(abs(new_data.temperature));

    // Clip the inputs so they don't overflow
    if (voltage > 100) {
        voltage = 99.9f;
    }
    if (current > 100) {
        current = 99.9f;
    }
    if (power_source > 10000) {
        power_source = 9999.0f;
    }
    if (power_total > 10000) {
        power_total = 9999.0f;
    }
    if (temperature > 999) {
        temperature = 999.9f;
    }

    // Create temporary line updates
    line_update new_line1(LINE1, voltage, current, power_source, percentage, temperature, status);
    line_update new_line2(LINE2, voltage, current, power_source, percentage, temperature, status);
    line_update new_line3(LINE3, voltage, current, power_source, percentage, temperature, status);
    line_update new_line4(LINE4, voltage, current, power_source, percentage, temperature, status);

    // line 2 is always a status line regardless of the screen
    new_line2.line_type = LINE_TYPE::STATUS; 

    // Set the line type
    if (screen_number == SCREEN::MAIN) {
        new_line1.line_type = LINE_TYPE::TITLE_SOC;
        new_line3.line_type = LINE_TYPE::FIXED;
        new_line4.line_type = LINE_TYPE::PROPERTIES_MAIN;
    } else {
        new_line3.line_type = LINE_TYPE::DC_VA;
        if (new_data.has_soc_estimation) {new_line1.line_type = LINE_TYPE::TITLE_SOC;}
        else {new_line1.line_type = LINE_TYPE::FIXED;}
        if (new_data.has_temp_sensor) {new_line4.line_type = LINE_TYPE::PROPERTIES;} 
        else {new_line4.line_type = LINE_TYPE::FIXED;}
    }

    // Save data to line updates
    screen_update new_screen(screen_number, new_line1, new_line2, new_line3, new_line4);

    update_screen(new_screen, autoprint);    
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
    snprintf(settings_data[SCREEN::MAIN]->lines[2]->txt, lcd_str_width, menu_num_format, menu_num-1); // Update the menu number
    snprintf(settings_data[SCREEN::MAIN]->lines[3]->txt, lcd_str_width, menu_name_format, menu_names[screen_num]); // Update the menu name
}

void MULTIFUEL_LCD::save_source_name(int change) {
    // Update menu num
    int dir;
    if (change < 0) dir = -1; // keep the change negative
    else dir = 1;

    if (change != 0) source_num += dir * (change / change); // make usre change is always +- 1

    // Make sure the menu number is within boundaries
    if (source_num >= NUM_DISPLAY_SCREENS) {source_num = 2;}
    if (source_num < 2) {source_num = NUM_DISPLAY_SCREENS-1;}

    // Get the screen number
    SCREEN screen_num = displayed_screens[source_num];

    // Format the screen
    snprintf(settings_data[SCREEN::MAIN]->lines[2]->txt, lcd_str_width, menu_num_format, source_num-2); // Update the menu number
    snprintf(settings_data[SCREEN::MAIN]->lines[3]->txt, lcd_str_width, menu_name_format, menu_names[screen_num]); // Update the menu name
}

void MULTIFUEL_LCD::save_setting_data(int idx, float new_setting, SCREEN lcd_screen, bool autoprint) {
    // Get information from screen_number data structure
    char update_str[5];
    new_setting = PosZero(new_setting, lcd_precision);

    // Save screen data
    dtostrf(new_setting, 4, lcd_precision, update_str);
    
    for (int i = 15; i < 19; i++) {
        settings_data[lcd_screen]->lines[idx]->txt[i] = update_str[i-15];
    }

    // Update LCD screen
    if (autoprint) {
        print_partial_line(idx, 15, update_str);
    }
}

void MULTIFUEL_LCD::save_setting_data(bool val, bool autoprint) {
    // Update the main module screen
    if (current_line == 1) {
        if (en_backlight) {
            // Save screen data
            for (int i = 17; i < 20; i++) {
                settings_data[SCREEN::SUMMARY]->lines[current_line]->txt[i] = on_str[i-17];
            }
            if (autoprint) {print_partial_line(current_line,17,on_str);}
        }
        else {
            // Save screen data
            for (int i = 17; i < 20; i++) {
                settings_data[SCREEN::SUMMARY]->lines[current_line]->txt[i] = off_str[i-17];
            }            
            if (autoprint) {print_partial_line(current_line,17,off_str);}
        }
    } else if (current_line == 2) {
        // Save screen data
        for (int i = 14; i < 20; i++) {
            settings_data[SCREEN::SUMMARY]->lines[current_line]->txt[i] = lcd_color_str[backlight_color][i-14];
        }
            if (autoprint) {print_partial_line(current_line, 14, lcd_color_str[backlight_color]);}
    } else if (current_line == 3) {
        if (val) {
            // Save screen data
            for (int i = 17; i < 20; i++) {
                settings_data[SCREEN::SUMMARY]->lines[current_line]->txt[i] = on_str[i-17];
            }        
            if (autoprint) {print_partial_line(current_line,17,on_str);}
        }
        else {
            // Save screen data
            for (int i = 17; i < 20; i++) {
                settings_data[SCREEN::SUMMARY]->lines[current_line]->txt[i] = off_str[i-17];
            }                    
            if (autoprint) {print_partial_line(current_line,17,off_str);}
        }
    }
}

void MULTIFUEL_LCD::update_summary_screen(int num_srcs, int max_temp, int max_current) {
    // Format the summary screen
    snprintf(lcd_data[SCREEN::SUMMARY]->lines[LINE2]->txt, lcd_str_width, summary_line2_format, num_srcs);
    snprintf(lcd_data[SCREEN::SUMMARY]->lines[LINE3]->txt, lcd_str_width, summary_line3_format, max_temp);
    snprintf(lcd_data[SCREEN::SUMMARY]->lines[LINE4]->txt, lcd_str_width, summary_line4_format, max_current);
}

void MULTIFUEL_LCD::full_screen_refresh()
{
    // print_screen(*lcd_data[active_screen], false);
    if (settings_mode && blinking_cursor) {blink_cursor();}

    // Print the screen
    if (settings_mode) {
        print_screen(*settings_data[active_screen], false);
    } else {
        print_screen(*lcd_data[active_screen], false);
    }
}

void MULTIFUEL_LCD::full_screen_refresh(screen_data new_screen)
{
    // print_screen(*lcd_data[active_screen], false);
    if (settings_mode && blinking_cursor) {blink_cursor();}

    // Print screen
    print_screen(new_screen, false);
}

void MULTIFUEL_LCD::refresh(bool FORCE)
{
    if ((FORCE) || (update_timer > LCD_UPDATE_RATE))
    {
        full_screen_refresh();  
        update_timer = 0;
    }
}

void MULTIFUEL_LCD::change_screen(SCREEN screen_number, bool clear) {
    if (settings_mode) {
        // Print screen
        print_screen(*settings_data[screen_number], clear);
        current_line = 1;
        locate(current_line,0);
    } else {
        // Print screen
        print_screen(*lcd_data[screen_number], clear);
    }


    // update active screen
    active_screen = screen_number;
}

void MULTIFUEL_LCD::print_screen(screen_data screen, bool clear_lcd) {
    if (clear_lcd)
    {
        clear();
        locate(0,0);
    }

    // Print screen
    concatStrings(lcd_str, screen.lines[0]->txt, screen.lines[1]->txt, screen.lines[2]->txt, screen.lines[3]->txt);
    nb_write_array(lcd_str);

    // Update active screen
    active_screen = screen.screen_number;

    // Reset timer so it doesn't immediately refresh
    last_update = millis();
}

void MULTIFUEL_LCD::print_line(line_data line, bool clear) {
    // Clear the line
    if (clear)
    {
        clear_line(line.line_number);
    }

    // Print the line
    write_line(line);
}

void MULTIFUEL_LCD::update_screen(screen_update new_data, bool autoprint) {
    // Update each line 
    update_line(new_data.screen_number, new_data.line1, false);
    update_line(new_data.screen_number, new_data.line2, false);
    update_line(new_data.screen_number, new_data.line3, false);
    update_line(new_data.screen_number, new_data.line4, false);

    // Print screen when complete
    // if (autoprint)
    // {
    //     print_screen(*lcd_data[new_data.screen_number]);
    // }
}

void MULTIFUEL_LCD::update_line(SCREEN screen_number, line_update new_data, bool autoprint) {
    // format new data
    if (new_data.line_type != LINE_TYPE::FIXED) {
        // char volt_str[5];
        // char curr_str[5];
        // char power_str[5];
        // char temp_str[6];
        char soc_str[4];
        // formatNumber(new_data.voltage, 4, lcd_precision, volt_str);
        // formatNumber(new_data.current, 4, lcd_precision, curr_str);
        // formatNumber(new_data.power, 4, 0, power_str);
        snprintf(soc_str, 4, soc_format, int(new_data.soc));
        // dtostrf(new_data.temp, 5, lcd_precision, temp_str);
        // Add leading zeros for the current, voltage and power measurements

        // Format strings
        if (new_data.line_type == LINE_TYPE::TITLE_SOC) 
        {
            for (int i = 16; i < 19; i++) {
                lcd_data[screen_number]->lines[new_data.line_number]->txt[i] = soc_str[i-16];
            }
            lcd_data[screen_number]->lines[new_data.line_number]->txt[19] = percent_symbol[0];
        }
        else if (new_data.line_type == LINE_TYPE::PROPERTIES) 
        {
            snprintf(lcd_data[screen_number]->lines[new_data.line_number]->txt, lcd_str_width,properties_format, int(new_data.temp));
        }
        else if (new_data.line_type == LINE_TYPE::VARDC_VA) 
        {
            snprintf(lcd_data[screen_number]->lines[new_data.line_number]->txt, lcd_str_width,vardc_format, new_data.voltage, new_data.current);
        }
        else if (new_data.line_type == LINE_TYPE::DC_VA) 
        {
            snprintf(lcd_data[screen_number]->lines[new_data.line_number]->txt, lcd_str_width,dc_format, new_data.voltage, new_data.current);
        }
        else if (new_data.line_type == LINE_TYPE::PROPERTIES_MAIN) 
        {
            snprintf(lcd_data[screen_number]->lines[new_data.line_number]->txt, lcd_str_width,vaw_format, new_data.voltage, int(new_data.current), int(new_data.power));
        }
        else if (new_data.line_type == LINE_TYPE::AC) 
        {
            snprintf(lcd_data[screen_number]->lines[new_data.line_number]->txt, lcd_str_width,ac_format, new_data.power);
        }
        else if (new_data.line_type == LINE_TYPE::STATUS) 
        {
            snprintf(lcd_data[screen_number]->lines[new_data.line_number]->txt, lcd_str_width, "%s",status_vector[new_data.status]);
        }
    }

    // // Print line on completion
    // if (autoprint && (active_screen == screen_number))
    // {
    //     print_line(*lcd_data[screen_number]->lines[new_data.line_number]);
    // }
}

void MULTIFUEL_LCD::update_cursor_position(int change) {
    // Erase cursor from current position (I might not where it is so jsut wipe it)
    settings_data[active_screen]->lines[1]->txt[0] = cursor_off[0];
    settings_data[active_screen]->lines[2]->txt[0] = cursor_off[0];
    settings_data[active_screen]->lines[3]->txt[0] = cursor_off[0];

    // Compute new cursor line
    int dir;
    if (change < 0) dir = -1; // 
    else dir = 1;

    if (change != 0) current_line += dir * (change / change); // makes change +- 1

    // Ensure current line is within boundaries
    int max_line = 3;
    if (active_screen == SCREEN::MAIN) max_line = 2; // if on the main settings page there are only 2 settings
    if (current_line < 1) {current_line = max_line;}
    if (current_line > max_line) {current_line = 1;}
    
    // Update cursor position
    settings_data[active_screen]->lines[current_line]->txt[0] = cursor_on[0];
}

void MULTIFUEL_LCD::blink_cursor() {

    // Make cursor blink
    if (blink_timer > CURSOR_BLINK_RATE) {
        if (show_cursor) {
            settings_data[active_screen]->lines[current_line]->txt[0] = cursor_on[0];
            show_cursor = false;
        } else {
            settings_data[active_screen]->lines[current_line]->txt[0] = cursor_off[0];
            show_cursor = true;
        }
        blink_timer = 0;
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
    currentMode = MODE_NORMAL;

    setupSystemMessages(); //Load settings, such as displaySystemMessages
    
    setupLCD(); //Initialize the LCD

    setupContrast(); //Set contrast

    setupBacklight(); //Turn on any backlights

    setupSplash(); //Read and display the user's splash screen

    setupCustomChars(); //Pre-load user's custom chars from EEPROM

    setupPower(); //Power down peripherals that we won't be using
}

//Look up settings like the enabling of system messages
void MULTIFUEL_LCD::setupSystemMessages()
{
  //Look up if we should display messages or not
  settingDisplaySystemMessages = EEPROM.read(LOCATION_DISPLAY_SYSTEM_MESSAGES);
  if (settingDisplaySystemMessages) //True = 1, false = 0
  {
    settingDisplaySystemMessages = DEFAULT_DISPLAY_SYSTEM_MESSAGES;
    EEPROM.update(LOCATION_DISPLAY_SYSTEM_MESSAGES, settingDisplaySystemMessages);
  }
}

void MULTIFUEL_LCD::setupLCD()
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

  _lcd->begin(settingLCDwidth, settingLCDlines); //Setup the width and lines for this LCD
  _lcd->lineWrap(); // enable line wrap

  //Clear any characters in the frame buffer
  _lcd->clear();
}

void MULTIFUEL_LCD::setupContrast() {

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

void MULTIFUEL_LCD::setupSplash()
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
      _lcd->clear();
      _lcd->setCursor(0, 0); //First position, 1st row
      _lcd->print(F("SparkFun OpenLCD"));
      _lcd->setCursor(0, 1); //First position, 2nd row
      _lcd->print(F("Baud:"));

      //Display firmware version
      _lcd->print(F(" v"));
      _lcd->print(firmwareVersionMajor);
      _lcd->print(F("."));
      _lcd->print(firmwareVersionMinor);
    }
    else
    {
      //Pull splash content from EEPROM

      //Copy the EEPROM to the character buffer
      for (byte x = 0 ; x < settingLCDlines * settingLCDwidth ; x++)
        currentFrame[x] = EEPROM.read(LOCATION_SPLASH_CONTENT + x);

      //Now display the splash
      _lcd->write(currentFrame);
    }

    //Now erase it and the buffer
    _lcd->clear();
    _lcd->setCursor(0, 0); //Reset cursor

    //After this function we go back to system baud rate
  }
}

//Look up the 8 custom chars from EEPROM and push them to the LCD
//We have to re-init the LCD after we send the chars
void MULTIFUEL_LCD::setupCustomChars()
{
  for (byte charNumber = 0 ; charNumber < 8 ; charNumber++)
  {
    for (byte charSpot = 0 ; charSpot < 8 ; charSpot++)
      customCharData[charSpot] = EEPROM.read(LOCATION_CUSTOM_CHARACTERS + (charNumber * 8) + charSpot);

    _lcd->createChar(charNumber, customCharData); //Record the array to CGRAM
  }

  //For some reason you need to re-init the LCD after a custom char is loaded
  _lcd->begin(settingLCDwidth, settingLCDlines);
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

void MULTIFUEL_LCD::clear_line(int line_number) {
    // Ensure line_number is between 0 to 3
    if (line_number < 0) {
        line_number = 0;
    }
    if (line_number > 3) {
        line_number = 3;
    }
    // Clear the line_number
    write_line(*empty_screen.lines[line_number]);
}

void MULTIFUEL_LCD::print_partial_line(int row, int collumn, const char *txt) {
    // Move LCD to right row and collumn
    locate(row, collumn);

    // Write it to the lcd
    write_array(txt);
}   

void MULTIFUEL_LCD::write_line(line_data line) {
    // Locate to the row
    locate(line.line_number,0);

    // write the line
    write_array(line.txt);
}

void MULTIFUEL_LCD::tick() {
    // update timers
    update_timer += timer_increment;
    blink_timer += timer_increment;
}

SCREEN MULTIFUEL_LCD::getScreenID(int &i) {
    // Ensure integer is within the bounds of the screen ids
    if (i >= NUM_DISPLAY_SCREENS) {i = 0;};
    if (i < 0) {i = NUM_DISPLAY_SCREENS - 1;};
    // return screen from integer
    return static_cast<SCREEN>(displayed_screens[i]);
}

SOURCE_STATUS getStatus(int &i) {
    // Ensure integer is within the bounds of the screen ids
    if (i > 2) {i = 2;};
    if (i < 0) {i = 0;};
    // return screen from integer
    return static_cast<SOURCE_STATUS>(i);
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