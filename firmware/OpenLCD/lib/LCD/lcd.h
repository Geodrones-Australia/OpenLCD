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
    hd44780_pinIO &_lcd;
    TwoWire &_i2c;
    uint8_t i2c_address;
};
int offset = 0;
#define NUM_SCREENS 15 // NUmber of screens saved (total)
#define NUM_DISPLAY_SCREENS 11 // Number of screens displayed

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
// LiquidCrystalFast SerLCD(LCD_RS, LCD_RW, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
hd44780_pinIO SerLCD(LCD_RS, LCD_RW, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

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


/*
Screens: enumerator for screens for each source and LCD specific screens (Order indicates the index of the source data)
*/
enum SCREEN {
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
};

/*
Status: enumerator for the different multifuel status, index indicates which error line to display
*/
enum SOURCE_STATUS {
    NOTFOUND = 0,
    DISABLED = 1,
    ENABLED = 2,
    DISABLED_OV = 3,
    DISABLED_UV = 4,
    DISABLED_OC = 5,
    DISABLED_OT = 6,
    RESET_RQ    = 7,
    UNIT_OK     = 8,
    NO_SOURCES  = 9,
};
#define NUM_STATUS 10

/*
Line Type: enumerator for the different types of lines
*/
enum LINE_TYPE {
    TITLE,  
    TITLE_TEMP,
    TITLE_SOC,  
    STATUS,   
    DC_VA,  
    VARDC_VA,
    AC, 
    PROPERTIES_MAIN,     
    PROPERTIES,  
    FIXED,
    CUSTOM,  // justs writes whatever text you add as a line
};

// Useful Macros
#define LINE1 0
#define LINE2 1
#define LINE3 2
#define LINE4 3


/**
 * Sensor data
 * 
 * @author Albert (06=8/2024)
 * 
 * @param screen_number     Screen to update
 * @param status            sensor status
 * @param voltage           Voltage (V)
 * @param current           current (A)
 * @param power_source      total power being used by sensor
 * @param power_total       total power being used (only used by main screen)
 * @param percentage        total power being used (only used by main screen)
 * @param temperature       total power being used (only used by main screen)
 * 
 */
struct sensor_data {
    SCREEN screen_number;
    SOURCE_STATUS status;
    float voltage;
    float current;
    float power_source;
    float power_total;
    float percentage = 0.0f;
    float temperature = 0.0f;
    bool  has_temp_sensor = false;
    bool  has_soc_estimation = false;

    sensor_data() {};
    sensor_data(SCREEN num, SOURCE_STATUS stat, float v, float a, float ps, float pt) :
    screen_number(num), status(stat), voltage(v), current(a), power_source(ps), power_total(pt) {};
};

// Line update details
/**
 * Line update
 * 
 * @author Albert (06=8/2024)
 * 
 * @param line_number       pointer to the i2c bus the input is on
 * @param line_type         Line type (0 = title, 1 = status, 2 = power, 3 = dc, 4 = custom)
 * @param voltage           (float) Voltage (Volts)
 * @param current           (float) Current (Amps)
 * @param power             (float) power (Watts)
 * @param soc               (float) State of Charge (%)
 * @param temp              (float) temperature (Celsius)
 * @param status            Status number (0 = not found, 1 = disabled, 2 = enabled)
 * 
 */
struct line_update {
    int line_number;
    int line_type;
    float voltage;
    float current;
    float power;
    float soc;
    float temp;
    int status;

    line_update() {};
    line_update(int line_num, float v, float a, float w, float bsoc, float t, int stat) : 
                line_number(line_num), voltage(v), current(a), power(w), soc(bsoc), temp(t), status(stat) {}; 
    
};

/**
 * Screen update
 * 
 * @author Albert
 * 
 * @param screen_number     Screen to update
 * @param line1             line1 update
 * @param line2             line2 update
 * @param line3             line3 update
 * @param line4             line4 update
 * 
 */
struct screen_update {
    SCREEN screen_number;
    line_update line1;
    line_update line2;
    line_update line3;
    line_update line4;

    screen_update() {};
    screen_update(SCREEN num, line_update l1, line_update l2, line_update l3, line_update l4 ) :
                  screen_number(num), line1(l1), line2(l2), line3(l3), line4(l4) {};
};

// structure for line data
/**
 * line data
 * 
 * @author Albert
 * 
 * @param line_number     line number
 * @param txt             line txt
 * 
 */
struct line_data {
    int line_number;
    char txt[21];
};

/**
 * Screen data
 * 
 * @author Albert (06=8/2024)
 * 
 * @param screen_number     Screen to update
 * @param line1             line1 data
 * @param line2             line2 data
 * @param line3             line3 data
 * @param line4             line4 data
 * 
 */
struct screen_data {
    SCREEN   screen_number;
    line_data line1;
    line_data line2;
    line_data line3;
    line_data line4;
    line_data *lines[4] = {&line1, &line2, &line3, &line4};

    screen_data() {};
    screen_data(SCREEN num, line_data l1, line_data l2, line_data l3, line_data l4 ) :
                screen_number(num), line1(l1), line2(l2), line3(l3), line4(l4) {};
};

// Helper functions
SOURCE_STATUS getStatus(int &i);
char * formatNumber(float val, signed char width, unsigned char prec, char *sout);

class MULTIFUEL_LCD {
    public:
        // Classes
        hd44780_pinIO *_lcd;
        TwoWire *_i2c;

        // LCD properties
        uint8_t i2c_address = 0x72;
        uint8_t LCD_DELAY = 50;          // delay to wait for LCD to update (ms)
        SCREEN active_screen = SCREEN::MAIN;
        bool lcd_present = false;
        bool lcd_mode = true;
        bool settings_mode = false;
        int current_line = 1;

        // Update rate
        uint32_t LCD_UPDATE_RATE = 25;   // 25ms/40Hz
        uint32_t last_update;
        uint32_t CURSOR_BLINK_RATE = 100; //ms
        uint32_t last_cursor_update;
        int timer_increment = 1;
        int update_timer = 0;
        int blink_timer = 0;
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

        // Screen names
        int menu_num = 1;
        int source_num = 2;
        const char *menu_names[NUM_SCREENS];
        const char *source_names[NUM_SCREENS];

        // Default character arrays
        char lcd_str[81]; 
        char on_str[4]           = " ON"; 
        char off_str[4]          = "OFF";    
        int lcd_precision = 1;

        // Status strings
        const char *off_line      = "Status: not found   ";
        const char *enabled_line  = "Status: enabled     ";
        const char *disabled_line = "Status: disabled    ";
        const char *OV_line       = "Status: disabled(OV)";
        const char *UV_line       = "Status: disabled(UV)";
        const char *OC_line       = "Status: disabled(OC)";
        const char *OT_line       = "Status: disabled(OT)";
        const char *RESET_line    = "Status: RST REQUIRED";
        const char *UNIT_OK_line  = "Status: Unit OK     ";
        const char *NO_SRCS_line  = "Status: No Sources  ";
        const char * status_vector[NUM_STATUS];
        
        /*****************************Character formatting Arrays*****************************/

        // Main Menu/Input screen formatting arrays
        const char *dc_format         = "Input:   %04.1fV/%04.1fA";
        const char *properties_format = "Board Temp:    %3d\337C";
        const char *vardc_format      = "VARDC:   %04.1fV/%04.1fA";
        const char *ac_format         = "Inverter:      %04dW";
        const char *vaw_format        = " %04.1fV/%02dA    %04dW ";
        const char *summary_format    = "%2dV/%2dA";
        const char *menu_name_format  = " \176%s";
        const char *menu_num_format   = " Go to page       %2d";
        const char *source_num_format = " Go to input     %2d";

        // Integer formatting
        const char *temp_format       = "%sC";
        const char *soc_format        = "%3d";
        const char *str_float_format  = "%s";

        // Symbols
        const char *cursor_on         = ">";
        const char *cursor_off        = " ";
        const char *percent_symbol    = "%";

        // Summary page formatting arrays
        const char *summary_line2_format = "Active Inputs:    %2d";
        const char *summary_line3_format = "Temp    (Max): %3d\337C";
        const char *summary_line4_format = "Current (Max):   %2dA";

        // Title lines
        line_data main_title_line        = {0, " Multifuel Unit     "};
        line_data lead_title_line        = {0, "  ) Lead Acid       "};
        line_data lipo_title_line        = {0, "  ) 6S Lipo         "};
        line_data corella_in_title_line  = {0, "  ) BATT (INT)      "};
        line_data corella_ext_title_line = {0, "  ) BATT (EXT)      "};
        line_data solar_title_line       = {0, "  ) Solar Charger   "};
        line_data generator_title_line   = {0, "  ) Generator       "};
        line_data acdc_title_line        = {0, "  ) ACDC            "};
        line_data dcdc_title_line        = {0, "  ) DCDC            "};
        line_data charger_title_line     = {0, "  ) Battery Charger "};
        line_data vardc_title_line       = {0, "  ) Variable DC     "};
        line_data inverter_title_line    = {0, "  ) Inverter        "};
        line_data lead_stepup_title_line = {0, "  ) 12V Stepup      "};
        line_data other_title_line       = {0, "  ) Default Title   "};

        // Default lines for startup screen
        line_data default_startup1       = {1, "Setting up unit     "};
        line_data default_startup2       = {2, "Ensure no loads are "};
        line_data default_startup3       = {3, "connected           "};

        // Default lines for main screen
        line_data default_status_main    = {1, "Status: No Sources  "};
        line_data default_dc_main        = {2, "   VARDC   Inverter "};
        line_data default_ac_main        = {3, " 00.0V/00A   0000W  "};

        // Default lines for inputs/internal sources
        line_data default_status         = {1, "Status: not found   "};
        line_data default_dc             = {2, "Input:   00.0V/00.0A"};
        line_data default_properties     = {3, "Board Temp:    N/A\337C"};  

        // Default lines for settings menu
        line_data settings_line1         = {0, "   Settings Menu    "};
        line_data settings_line2         = {1, ">Go back to main    "};
        line_data settings_line3         = {2, " Go to page        0"};
        line_data settings_line4         = {3, " \176Module Settings   "};

        // Default module settings lines
        line_data module_line1           = {0, "00) Module Settings "};
        line_data module_line2           = {1, ">Backlight        ON"};
        line_data module_line3           = {2, " LCD Color     White"};
        line_data module_line4           = {3, " Soft Protection  ON"};

        // Default input settings menu lines
        line_data default_min_v          = {1, ">Min Voltage   00.0V"};
        line_data default_max_v          = {2, " Max Voltage   55.0V"};
        line_data default_max_a          = {3, " Max Current   55.0A"};

        // Default Summary Lines
        line_data summary_line1         = {0, "00) Unit Summary    "};
        line_data summary_line2         = {1, "Active Inputs:    00"};
        line_data summary_line3         = {2, "Temp    (Max): 000\337C"};
        line_data summary_line4         = {3, "Current (Max):   00A"};

        // Default empty lines
        line_data empty_line1 = {0, "                    "};
        line_data empty_line2 = {1, "                    "};
        line_data empty_line3 = {2, "                    "};
        line_data empty_line4 = {3, "                    "};

        /**********Default Screen Data (special)************/
        screen_data empty_screen = {
            SCREEN::OTHER, 
            empty_line1,
            empty_line2,
            empty_line3,
            empty_line4,
        };

        /**********Default Screen Data (main mode)**********/
        screen_data start_screen = {
            SCREEN::MAIN, 
            main_title_line,
            default_startup1,
            default_startup2,
            default_startup3
        };
        screen_data main_screen = {
            SCREEN::MAIN, 
            main_title_line,
            default_status_main,
            default_dc_main,
            default_ac_main
        };
        screen_data summary_screen  = {
            SCREEN::SUMMARY, 
            summary_line1,
            summary_line2,
            summary_line3,
            summary_line4,
        };
        screen_data lead_screen = {
            SCREEN::LEAD_DETAILS, 
            lead_title_line,
            default_status,
            default_dc,
            default_properties,
        };
        screen_data lipo_screen = {
            SCREEN::LIPO_DETAILS, 
            lipo_title_line,
            default_status,
            default_dc,
            default_properties,
        };
        screen_data corella_in_screen   = {
            SCREEN::CORELLA_IN_DETAILS, 
            corella_in_title_line,
            default_status,
            default_dc,
            default_properties,
        };
        screen_data corella_ext_screen  = {
            SCREEN::CORELLA_EXT_DETAILS, 
            corella_ext_title_line,
            default_status,
            default_dc,
            default_properties,
        };
        screen_data solar_screen        = {
            SCREEN::SOLAR_DETAILS, 
            solar_title_line,
            default_status,
            default_dc,
            default_properties,
        };
        screen_data generator_screen    = {
            SCREEN::GENERATOR_DETAILS, 
            generator_title_line,
            default_status,
            default_dc,
            default_properties,

        };
        screen_data acdc_screen         = {
            SCREEN::ACDC_DETAILS, 
            acdc_title_line,
            default_status,
            default_dc,
            default_properties,

        };
        screen_data dcdc_screen         = {
            SCREEN::DCDC_DETAILS, 
            dcdc_title_line,
            default_status,
            default_dc,
            default_properties,

        };
        screen_data charger_screen      = {
            SCREEN::CHARGER_DETAILS, 
            charger_title_line,
            default_status,
            default_dc,
            default_properties,

        };
        screen_data vardc_screen        = {
            SCREEN::VARDC_DETAILS, 
            vardc_title_line,
            default_status,
            default_dc,
            default_properties,

        };
        screen_data inverter_screen     = {
            SCREEN::INVERTER_DETAILS, 
            inverter_title_line,
            default_status,
            default_dc,
            default_properties,
        };
        screen_data lead_stepup_screen  = {
            SCREEN::LEAD_STEPUP_DETAILS, 
            lead_stepup_title_line,
            default_status,
            default_dc,
            default_properties,

        };
        screen_data other_screen  = {
            SCREEN::OTHER, 
            other_title_line,
            default_status,
            default_dc,
            default_properties,
        };

        // Create vector for the screens
        // std::vector<screen_data *> lcd_data = {&lead_screen, &lipo_screen, &corella_in_screen, &corella_ext_screen, &solar_screen, &generator_screen, &acdc_screen, &dcdc_screen, &charger_screen, &vardc_screen, &inverter_screen, &lead_stepup_screen, &main_screen};
        screen_data * lcd_data[NUM_SCREENS];
        /**********Default Screen Data (settings)**********/
        screen_data main_settings_screen = {
            SCREEN::MAIN, 
            settings_line1,
            settings_line2,
            settings_line3,
            settings_line4
        };
        screen_data module_settings_screen = {
            SCREEN::SUMMARY, 
            module_line1,
            module_line2,
            module_line3,
            module_line4
        };
        screen_data lead_settings_screen = {
            SCREEN::LEAD_DETAILS, 
            lead_title_line,
            default_min_v,
            default_max_v,
            default_max_a
        };
        screen_data lipo_settings_screen = {
            SCREEN::LIPO_DETAILS, 
            lipo_title_line,
            default_min_v,
            default_max_v,
            default_max_a
        };
        screen_data corella_in_settings_screen   = {
            SCREEN::CORELLA_IN_DETAILS, 
            corella_in_title_line,
            default_min_v,
            default_max_v,
            default_max_a          
        };
        screen_data corella_ext_settings_screen  = {
            SCREEN::CORELLA_EXT_DETAILS, 
            corella_ext_title_line,
            default_min_v,
            default_max_v,
            default_max_a         
        };
        screen_data solar_settings_screen        = {
            SCREEN::SOLAR_DETAILS, 
            solar_title_line,
            default_min_v,
            default_max_v,
            default_max_a           
        };
        screen_data generator_settings_screen    = {
            SCREEN::GENERATOR_DETAILS, 
            generator_title_line,
            default_min_v,
            default_max_v,
            default_max_a         
        };
        screen_data acdc_settings_screen         = {
            SCREEN::ACDC_DETAILS, 
            acdc_title_line,
            default_min_v,
            default_max_v,
            default_max_a         
        };
        screen_data dcdc_settings_screen         = {
            SCREEN::DCDC_DETAILS, 
            dcdc_title_line,
            default_min_v,
            default_max_v,
            default_max_a         
        };
        screen_data charger_settings_screen      = {
            SCREEN::CHARGER_DETAILS, 
            charger_title_line,
            default_min_v,
            default_max_v,
            default_max_a         
        };
        screen_data vardc_settings_screen        = {
            SCREEN::VARDC_DETAILS, 
            vardc_title_line,
            default_min_v,
            default_max_v,
            default_max_a         
        };
        screen_data inverter_settings_screen     = {
            SCREEN::INVERTER_DETAILS, 
            inverter_title_line,
            default_min_v,
            default_max_v,
            default_max_a     
        };
        screen_data lead_stepup_settings_screen  = {
            SCREEN::LEAD_STEPUP_DETAILS, 
            lead_stepup_title_line,
            default_min_v,
            default_max_v,
            default_max_a         
        };

        screen_data other_settings_screen  = {
            SCREEN::OTHER, 
            other_title_line,
            default_min_v,
            default_max_v,
            default_max_a         
        };

        // Create vector for the screens
        // std::vector<screen_data *> settings_data = {&lead_settings_screen, &lipo_settings_screen, &corella_in_settings_screen, &corella_ext_settings_screen, &solar_settings_screen, &generator_settings_screen, &acdc_settings_screen, &dcdc_settings_screen, &charger_settings_screen, &vardc_settings_screen, &inverter_settings_screen, &lead_stepup_settings_screen, &main_settings_screen};
        screen_data * settings_data[NUM_SCREENS];
        // Sensor data update structures for lcd
        sensor_data lead         = {SCREEN::LEAD_DETAILS,SOURCE_STATUS::NOTFOUND,0,0,0,0};
        sensor_data lipo         = {SCREEN::LIPO_DETAILS,SOURCE_STATUS::NOTFOUND,0,0,0,0};
        sensor_data corella_in   = {SCREEN::CORELLA_IN_DETAILS,SOURCE_STATUS::NOTFOUND,0,0,0,0};
        sensor_data corella_ext  = {SCREEN::CORELLA_EXT_DETAILS,SOURCE_STATUS::NOTFOUND,0,0,0,0};
        sensor_data solar        = {SCREEN::SOLAR_DETAILS,SOURCE_STATUS::NOTFOUND,0,0,0,0};
        sensor_data generator    = {SCREEN::GENERATOR_DETAILS,SOURCE_STATUS::NOTFOUND,0,0,0,0};
        sensor_data acdc         = {SCREEN::ACDC_DETAILS,SOURCE_STATUS::NOTFOUND,0,0,0,0};
        sensor_data dcdc         = {SCREEN::DCDC_DETAILS,SOURCE_STATUS::NOTFOUND,0,0,0,0};
        sensor_data charger      = {SCREEN::CHARGER_DETAILS,SOURCE_STATUS::NOTFOUND,0,0,0,0};
        sensor_data vardc        = {SCREEN::VARDC_DETAILS,SOURCE_STATUS::NOTFOUND,0,0,0,0};
        sensor_data inverter     = {SCREEN::INVERTER_DETAILS,SOURCE_STATUS::NOTFOUND,0,0,0,0};
        sensor_data leadstepup   = {SCREEN::LEAD_STEPUP_DETAILS,SOURCE_STATUS::NOTFOUND,0,0,0,0};
        sensor_data main_screen_data = {SCREEN::MAIN,SOURCE_STATUS::NOTFOUND,0,0,0,0};
        sensor_data data_vector[NUM_SCREENS] = {lead, lipo, corella_in, corella_ext, solar, generator, acdc, dcdc, charger, vardc, inverter, leadstepup, main_screen_data};

        // Make a seperate vector for storing which screens are active
        SCREEN displayed_screens[NUM_DISPLAY_SCREENS];
        
        // Min difference to update value
        float MIN_DIFF_V = 0.50f; // minimum difference to flag that new data is present
        float MIN_DIFF_A = 0.10f; // minimum difference to flag that new data is present
        float MIN_DIFF_P = 1.0f; // minimum difference to flag that new data is present
        float MIN_DIFF_T = 0.01f; // minimum difference to flag new temperature present (V)

        // Constructor
        MULTIFUEL_LCD();
        MULTIFUEL_LCD(multifuel_lcd_config config);
        ~MULTIFUEL_LCD();

        // Init
        void init(multifuel_lcd_config config);
        void setupSystemMessages();
        void setupLCD();
        void setupContrast(); //Set contrast
        void setupBacklight();
        void setupSplash();
        void setupCustomChars();
        void setupPower();
        void startup();
        void configure();
        void reset_lcd();
        bool lcd_scan();

        // Refresh screen at a constant rate
        void tick();
        void refresh(bool FORCE = false);
        void full_screen_refresh();
        void full_screen_refresh(screen_data new_screen);

        // Display methods
        void save_sensor_data(int i, sensor_data new_data, bool FORCE = false);
        void save_sensor_data(sensor_data new_data, bool autoprint = false);
        void save_setting_data(int idx, float new_setting, SCREEN lcd_screen, bool autoprint = false);
        void save_setting_data(bool val = false, bool autoprint = false);
        void save_menu_name(int change);
        void save_source_name(int change);
        void update_summary_screen(int num_srcs, int avg_temp, int max_current);
        
        // Display methods
        void change_screen(SCREEN screen_number, bool clear_lcd = true);
        void update_screen(screen_update new_data, bool autoprint = true);
        void update_line(SCREEN screen_number, line_update new_data, bool autoprint = true);
        void update_cursor_position(int direction);
        void blink_cursor();
        void no_blink_cursor();

        // LCD Configuration Function
        void backlight_on();
        void backlight_off();
        void set_backlight_color(int backlight_idx);
        void set_backlight_color(byte r, byte g, byte b);
        void set_backlight_color(unsigned long color);

        // Helper function
        SCREEN getScreenID(int &i);

        // Printing methods
        /**
         * @brief Print screen to LCD
         * 
         * @param screen_data    screen data for the screen
         * @param clear          if you wish to clear the screen prior to printing new screen
        */
        void print_screen(screen_data screen, bool clear = true);

        /**
         * @brief Print line to LCD
         * @brief NB : When the array exceeds 20 characters it wraps to the next line
         * 
         * @param line    line_data to print
         * @param clear   if you wish to clear the line prior to printing new line
        */
        void print_line(line_data line, bool clear = true);

        /**
         * @brief Set the cursor to a given position.
         * @brief You have to set the row number first !!!
         * 
         * @param row 
         * @param column 
         */
        void locate(uint8_t row, uint8_t column);

        /**
         * @brief Fast locate (doesn't use blocking delay)
         * @brief You have to set the row number first !!!
         * 
         * @param row 
         * @param column 
         */
        void fast_locate(uint8_t row, uint8_t column);

        /**
         * @brief Write a character array into the lcd
         * @brief You can't continuously use write, with Arduino mbed for portentaH7, so write array sends each letter one at a time
         * 
         * @brief NB : When the array exceeds 20 characters it wraps to the next line
         * 
         * @param str
         */
        void write_array(const char *str);

        /**
         * @brief Write a character array into the lcd without using blocking delay (relies on external delay to ensure lines are note overwritten)
         * 
         * @brief NB : When the array exceeds 20 characters it wraps to the next line
         * 
         * @param str
         */
        void nb_write_array(const char *str);

        /**
         * @brief Clear framebuffer
         * 
         */
        void clearFrameBuffer();
        /**
         * @brief Clear lcd and return to 0,0
         * 
         */
        void clear();

        void print_partial_line(int row, int collumn, const char *txt);

    private:
        // Collomun of voltage/power data
        const int V_COLLUMN = 9;
        const int A_COLLUMN = 15;
        const int POWER_COLLUMN = 15;

        // LCD Properties
        const int LCD_LINES = 4;
        const int LCD_COLLUMNS = 20;
        size_t lcd_str_width;

        // Methods
        /**
         * @brief Clear row and move pointer to the beginning of the row	 * 
         * @brief NB : When the array exceeds 20 characters it wraps to the next line
         * 
         * @param row uint_8t row that needs to be cleared
         */
        void clear_line(int line_number);

        void write_line(line_data line);
        
        // void validate_str(); todo
};

#endif