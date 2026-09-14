#include "c2_countdown_number.h"

namespace esphome::cuktech_ble {

void C2CountdownNumber::control(float value) {
  this->parent_->set_c2_countdown(value);
  this->publish_state(value);
}

}  // namespace esphome::cuktech_ble
