#include "c3_port_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.c3_port";

void C3PortSwitch::setup() {

}
void C3PortSwitch::dump_config() {

}
void C3PortSwitch::write_state(bool state) {
  this->parent_->set_c3_port(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
