#include "button.h"

void Button::init(uint32_t btn_pin) {
    pin = btn_pin;

    // Set pin mode
    pinMode(pin, INPUT);

    // INitialise values
    press_timer = 0;
    hold_timer = 0;
    last_val = false;
}

bool Button::update_btn() {
  // Read button pin
  bool val = !digitalRead(pin);
  if (val != last_val) {
    last_val = val;
    if (val) { 
      // Button has been pressed
      button_pressed = true;
      hold_timer = 0;
      last_time = millis();
    }
    else if (hold_timer > DEBOUNCE_TIME) {
      // Button has been releaed, reset hold timer and return true
      hold_timer = 0;
      button_pressed = false;
      button_held = false;
      return true;
    }
  } else if (val && button_pressed) {
    // Button is being held
    hold_timer = millis() - last_time;// update hold timer
    if (hold_timer > HOLD_TIME) button_held = true;
    else button_held = false;
  }
  return false;
}