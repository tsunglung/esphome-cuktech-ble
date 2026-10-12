#include "screen_onoff_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.screen_onoff";

void ScreenOnOffSwitch::setup() {

}
void ScreenOnOffSwitch::dump_config() {

}
void ScreenOnOffSwitch::write_state(bool state) {
  this->parent_->set_screen_onoff(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
