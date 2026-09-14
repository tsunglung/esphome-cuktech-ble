#include "a_port_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.a_port";

void APortSwitch::setup() {

}
void APortSwitch::dump_config() {

}
void APortSwitch::write_state(bool state) {
  this->parent_->set_a_port(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
