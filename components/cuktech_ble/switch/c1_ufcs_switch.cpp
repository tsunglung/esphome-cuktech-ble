#include "c1_ufcs_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.c1_ufcs";

void C1UFCSSwitch::setup() {

}
void C1UFCSSwitch::dump_config() {

}
void C1UFCSSwitch::write_state(bool state) {
  this->parent_->set_c1_ufcs(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
