#include "a_countdown_number.h"

namespace esphome::cuktech_ble {

void ACountdownNumber::control(float value) {
  this->parent_->set_a_countdown(value);
  this->publish_state(value);
}

}  // namespace esphome::cuktech_ble
