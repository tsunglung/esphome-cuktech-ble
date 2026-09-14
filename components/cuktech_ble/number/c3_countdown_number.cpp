#include "c3_countdown_number.h"

namespace esphome::cuktech_ble {

void C3CountdownNumber::control(float value) {
  this->parent_->set_c3_countdown(value);
  this->publish_state(value);
}

}  // namespace esphome::cuktech_ble
