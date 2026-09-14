#include "screen_saver_timeout_select.h"

namespace esphome::cuktech_ble {

void ScreenSaverTimeoutSelect::control(size_t index) {
  this->publish_state(index);
  this->parent_->set_screen_saver_timeout(this->option_at(index));
}

}  // namespace esphome::cuktech_ble
