#include "screen_saver_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.screen_saver";

void ScreenSaverSwitch::setup() {

}
void ScreenSaverSwitch::dump_config() {

}
void ScreenSaverSwitch::write_state(bool state) {
  this->parent_->set_screen_saver(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
