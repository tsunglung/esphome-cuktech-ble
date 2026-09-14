#include "c1_countdown_number.h"

namespace esphome::cuktech_ble {

void C1CountdownNumber::control(float value) {
  this->parent_->set_c1_countdown(value);
  this->publish_state(value);
}

}  // namespace esphome::cuktech_ble
