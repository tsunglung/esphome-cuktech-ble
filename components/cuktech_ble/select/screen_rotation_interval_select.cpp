#include "screen_rotation_interval_select.h"

namespace esphome::cuktech_ble {

void ScreenRotationIntervalSelect::control(size_t index) {
  this->publish_state(index);
  this->parent_->set_screen_rotation_interval(this->option_at(index));
}

}  // namespace esphome::cuktech_ble
