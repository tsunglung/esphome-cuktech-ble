#include "c2_scp_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.c2_scp";

void C2SCPSwitch::setup() {

}
void C2SCPSwitch::dump_config() {

}
void C2SCPSwitch::write_state(bool state) {
  this->parent_->set_c3_scp(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
