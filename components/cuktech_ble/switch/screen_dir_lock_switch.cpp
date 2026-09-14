#include "screen_dir_lock_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.screen_lock";

void ScreenDirLockSwitch::setup() {

}
void ScreenDirLockSwitch::dump_config() {

}
void ScreenDirLockSwitch::write_state(bool state) {
  this->parent_->set_screen_dir_lock(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
