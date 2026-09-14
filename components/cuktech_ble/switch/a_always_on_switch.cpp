#include "a_always_on_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.a_always_on";

void AAlwaysOnSwitch::setup() {

}
void AAlwaysOnSwitch::dump_config() {

}
void AAlwaysOnSwitch::write_state(bool state) {
  this->parent_->set_a_always_on(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
