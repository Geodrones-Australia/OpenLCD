#ifndef _MULTIFUEL_LCD_H_
#define _MULTIFUEL_LCD_H_

#include "Arduino.h"
#include "Wire.h"
#include "utils.h"
#include <hd44780.h>
#include <hd44780ioClass/hd44780_pinIO.h> // Arduino pin i/o class header
#include <EEPROM.h>  //Brightness, Baud rate, and I2C address are stored in EEPROM

#include <avr/sleep.h> //Needed for sleep_mode
#include <avr/power.h> //Needed for powering down perihperals such as the ADC/TWI and Timers
#include <SoftPWM.h> //Software PWM for Blue backlight: From https://github.com/bhagman/SoftPWM
/**
 * LCD config
 * 
 * @author Albert (06=8/2024)
 * 
 * @param _lcd         pointer to lcd class
 * @param _i2c         pointer to the i2c bus the input is on
 * @param i2c_address  i2c address for the MCU
 * 
 */
struct multifuel_lcd_config {
    hd44780_pinIO _lcd;
    TwoWire &_i2c;
    uint8_t i2c_address;
};

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
// LiquidCrystalFast SerLCD(LCD_RS, LCD_RW, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7)

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

/*
Screens: enumerator for screens for each source and LCD specific screens (Order indicates the index of the source data)
*/
#define NUM_SCREENS 15 // NUmber of screens saved (total)
#define NUM_DISPLAY_SCREENS 11 // Number of screens displayed
typedef enum{
    // Sources
    CORELLA_IN_DETAILS, 
    CORELLA_EXT_DETAILS,
    LEAD_DETAILS,       
    LIPO_DETAILS,       
    GENERATOR_DETAILS,  
    ACDC_DETAILS,       
    SOLAR_DETAILS,      
    CHARGER_DETAILS,    
    DCDC_DETAILS, 
    LEAD_STEPUP_DETAILS,                
    VARDC_DETAILS,       
    INVERTER_DETAILS,
    // LCD specific screens 
    SUMMARY,
    MAIN,    
    OTHER,               
} SCREEN;

typedef enum {
    // Send Commands
    GET_CONFIG      = 123, // get configuration data Tx: N/A, Rx: num_screens, lcd_mode, backlight_on, backlight_color (array of screens)
    GET_SETTINGS    = 142, // get setting data for source Tx: screen_idx, Rx: screen_idx, max_voltage, max_current, max_temprature
    GET_SCREEN      = 150, // get current screen    Tx: screen_idx, screen_mode(settings/data). Rx: lcd_str[81]
    SEND_INPUT_DATA = 169, // data from buttons and encoder Tx: screen_idx, encoder_change, lcd_mode, current_line Rx: N/A
    SEND_SETTINGS   = 172, // send setting data for source  Tx: screen_idx, max_voltage, max_current, max_temprature Rx: N/A
} COMMANDS;

#define COMMAND_LEN 4
/**
 * Command data: Structure for the communicating with STM32
 * 
 * @author Albert (06=8/2024)
 * 
 * @param screen_number     screen number of source
 * @param max_voltage       max voltage for source
 * @param max_current       max current for source
 * @param max_temperature   max temperature of the sensor
 * 
 */
typedef struct Command{
  uint8_t cmd;
  uint8_t payload[4] = {0,0,0,0};

  Command() {};
  Command(COMMANDS command, uint8_t screen_num, uint8_t d1, uint8_t d2, uint8_t d3) :
  cmd(command) {
    payload[0] = screen_num;
    payload[1] = d1;
    payload[2] = d2;
    payload[3] = d3;
  };
}Command;

/**
 * Settings data: Structure for the settings of each source
 * 
 * @author Albert (06=8/2024)
 * 
 * @param screen_number     screen number of source
 * @param max_voltage       max voltage for source
 * @param max_current       max current for source
 * @param max_temperature   max temperature of the sensor
 * 
 */
struct settings_data {
    uint8_t screen_number;
    uint8_t max_voltage;
    uint8_t max_current;
    uint8_t max_temperature;
    settings_data() : screen_number(0), max_voltage(0), max_current(0), max_temperature(0) {};
    settings_data(SCREEN num) : screen_number(num), max_voltage(0), max_current(0), max_temperature(0) {};
    settings_data(uint8_t num, uint8_t v, uint8_t a, uint8_t t) :
    screen_number(num), max_voltage(v), max_current(a), max_temperature(t) {};
};

/**
 * LCD Settings data: Structure for the settings of each source
 * 
 * @author Albert (06=8/2024)
 * 
 * @param screen_number     current screen number
 * @param lcd_mode          settings/data mode for lcd
 * @param backlight_on      backlight on/off
 * @param backlight_color   backlight color
 * 
 */
struct input_data {
    uint8_t screen_number;
    uint8_t lcd_mode;
    uint8_t backlight_on;
    uint8_t backlight_color;
    input_data(): screen_number(0), lcd_mode(0), backlight_on(0), backlight_color(0) {};
    input_data(uint8_t num, uint8_t enc_change, uint8_t mode, uint8_t line) :
    screen_number(num), lcd_mode(enc_change), backlight_on(mode), backlight_color(line){};
};

// Helper functions
void petSafeDelay(int delayAmount);
void setPwmFrequency(int pin, int divisor);

class MULTIFUEL_LCD {
    public:
        // Classes
        hd44780_pinIO *_lcd;
        TwoWire *_i2c;

        // LCD properties
        uint8_t i2c_address  = 0x72;
        uint8_t LCD_DELAY    = 50;          // delay to wait for LCD to update (ms)
        SCREEN active_screen = SCREEN::MAIN;
        bool lcd_present = false;
        bool lcd_mode = 0b00000100;  // data mode, cursor off, backlight on, white color
        bool settings_mode = false;
        int lcd_precision = 1;  
        int current_line = 1;
        uint8_t screen_array[NUM_SCREENS+1]; // array of all the screens first entry is how many screens, the rest is the screen numbers in order

        // Update rate
        unsigned long  LCD_UPDATE_RATE = 25;   // 25ms/40Hz
        unsigned long last_update = 0;
        unsigned long CURSOR_BLINK_RATE = 100; //ms
        unsigned long last_cursor_update = 0;
        bool show_cursor = false;
        bool blinking_cursor = false;
        int buffer_size = 0;

        // Update triggers
        bool status_update, voltage_update, current_update, power_update;

        // LCD Properties
        bool en_backlight = true;
        int backlight_color = 0;
        int NUM_COLORS = 8;
        unsigned long lcd_colors[8] {
            0xFFFFFF, // White
            0xFF0000, // Red
            0x0000FF, // Blue
            0x00FF00, // Green
            0xFFFF00, // Yellow
            0x00FFFF, // Teal
            0x7F00FF, // Purple
            0xFF8C00, // Orange
        };
        const char* lcd_color_str[8] {
            " White", // White
            "   Red", // Red
            "  Blue", // Blue
            " Green", // Green
            "Yellow", // Yellow
            "  Teal", // Teal
            "Purple", // Purple
            "Orange", // Orange
        };
        
        // Default character arrays
        uint8_t i2c_buffer[BUFFER_LENGTH];
        char lcd_str[81]; 
        char on_str[4]      = " ON"; 
        char off_str[4]     = "OFF";   
        char cursor_on[2]   = ">";
        char cursor_off[2]  = " ";
        /*************************************Settings/Input Data********************************/
        settings_data data_vector[NUM_SCREENS];
        input_data ui_data = {
            SCREEN::MAIN,
            0,
            0,
            0,
        };

        /********************************Character formatting Arrays*****************************/
        int menu_num = 1;
        const char* menu_names[NUM_SCREENS];
        SCREEN displayed_screens[NUM_DISPLAY_SCREENS];

        

        /*************************************Methods********************************************/
        // Main Menu/Input screen formatting arrays
        MULTIFUEL_LCD();
        MULTIFUEL_LCD(multifuel_lcd_config config);
        ~MULTIFUEL_LCD();

        // Init
        void init(multifuel_lcd_config config);
        void setupLCD();
        void setupContrast(); //Set contrast
        void setupBacklight();
        void setupPower();
        void startup();
        void configure();
        void reset_lcd();

        // Low level Command functions
        void sendCommand(Command cmd); // Send command without response
        void sendCommand(Command cmd, uint8_t *buffer, size_t size); // Send command with response

        // High level Command functions
        void getConfig();
        void getSettings(uint8_t num);
        void getScreen(uint8_t num);
        void sendInputData(input_data data);
        void sendSettings(settings_data data);

        // Refresh screen at a constant rate
        void refresh(bool FORCE = false);
        void full_screen_refresh(bool FORCE = false);

        // Update setting menu screens
        uint8_t get_settings(int line);
        void save_menu_name(int change);
        void save_setting_data(int idx, uint8_t new_setting, SCREEN lcd_screen, bool autoprint = false);
        void save_setting_data(bool val = false, bool autoprint = false);

        // Helper function
        SCREEN getScreenID(int &i);
        
        // LCD Configuration Function
        void update_cursor_position(int direction);
        void blink_cursor();
        void backlight_on();
        void backlight_off();
        void set_backlight_color(int backlight_idx);
        void set_backlight_color(byte r, byte g, byte b);
        void set_backlight_color(unsigned long color);

        
        /**
         * @brief Set the cursor to a given position.
         * @brief You have to set the row number first !!!
         * 
         * @param row 
         * @param column 
         */
        void locate(uint8_t row, uint8_t column);

        // Printing methods
        /**
         * @brief Write a character array into the lcd
         * @brief You can't continuously use write, with Arduino mbed for portentaH7, so write array sends each letter one at a time
         * 
         * @brief NB : When the array exceeds 20 characters it wraps to the next line
         * 
         * @param str
         */
        void write_array(const char *str, uint8_t row = 0, uint8_t col = 0);

        /**
         * @brief Clear lcd and return to 0,0
         * 
         */
        void clear();

    private:
        // Collomun of voltage/power data
        const int V_COLLUMN = 9;
        const int A_COLLUMN = 15;
        const int POWER_COLLUMN = 15;

        // LCD Properties
        const int LCD_LINES = 4;
        const int LCD_COLLUMNS = 20;
        size_t lcd_str_width;
};

#endif