#include "screen_brightness_number.h"

namespace esphome::cuktech_ble {

void ScreenBrightnessNumber::control(float value) {
  this->parent_->set_brightness(value);
  this->publish_state(value);
}

}  // namespace esphome::cuktech_ble
