#include "c2_port_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.c2_port";

void C2PortSwitch::setup() {

}
void C2PortSwitch::dump_config() {

}
void C2PortSwitch::write_state(bool state) {
  this->parent_->set_c2_port(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
