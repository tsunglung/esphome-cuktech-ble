#include "c3_scp_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.c3_scp";

void C3SCPSwitch::setup() {

}
void C3SCPSwitch::dump_config() {

}
void C3SCPSwitch::write_state(bool state) {
  this->parent_->set_c3_scp(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
