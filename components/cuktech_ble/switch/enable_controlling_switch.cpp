#include "enable_controlling_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.enable_controlling";

void EnableControllingSwitch::setup() {

}
void EnableControllingSwitch::dump_config() {

}
void EnableControllingSwitch::write_state(bool state) {
  this->parent_->set_enable_controlling(state);
  state = this->parent_->controlling_enabled();
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
