#ifndef _MULTIFUEL_BUTTON_H_
#define _MULTIFUEL_BUTTON_H_

#include "Arduino.h"
#include "GPIO.h"
#include "Button.h"

template <BOARD::pin_t PIN, uint16_t DEBOUNCE = 100>
class Multifuel_Button : public Button<PIN, DEBOUNCE> {
    public:
        // Update button
        bool update_btn() {
            return (this->isReleased());
        }

        /**
         * Return true(1) if a button state change was detected, otherwise
         * false(0). Rising or falling edge is determined by reading the
         * debounced pin state.
         * @return bool.
         */
        bool isReleased()
        {
            // Check if debounce time limit has elapsed
            if (millis() - last_time < DEBOUNCE) return (false);
            last_time = millis();

            // Check for the pin state has changed
            bool state = this->m_pin;
            if (state == this->m_state) return (false);
            this->m_state = state;
            return (this->m_state); // if the button is high we've pressed it
        }

        // Button pressed status
        bool button_held;
        bool button_pressed;
        unsigned long last_time = 0;  // the last time the output pin was toggled
    private:
        uint32_t DEBOUNCE_TIME = 50; // 100 ms
        uint32_t HOLD_TIME = 1000; // 1000ms (1s)
        bool last_val = false;
        bool has_changed = false;
        unsigned long press_timer = 0;
        unsigned long hold_timer = 0;
        
};

#endif