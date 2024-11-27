#ifndef BUTTON_H
#define BUTTON_H

#include "Arduino.h"

class Button {
    public:
        uint32_t pin;

        void init(uint32_t btn_pin);
        bool update_btn();

        // Button pressed status
        bool button_held;
        bool button_pressed;
    private:
        unsigned long DEBOUNCE_TIME = 50; // 100 ms
        unsigned long HOLD_TIME = 1000; // 1000ms (1s)
        bool last_val = false;
        bool has_changed = false;
        unsigned long last_time = 0;  // the last time the output pin was toggled
        uint32_t press_timer = 0;
        uint32_t hold_timer = 0;
        
};

#endif