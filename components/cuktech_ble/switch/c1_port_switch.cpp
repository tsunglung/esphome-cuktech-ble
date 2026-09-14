#include "c1_port_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.c1_port";

void C1PortSwitch::setup() {

}
void C1PortSwitch::dump_config() {

}
void C1PortSwitch::write_state(bool state) {
  this->parent_->set_c1_port(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
