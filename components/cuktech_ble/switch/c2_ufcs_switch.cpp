#include "c2_ufcs_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.c2_ufcs";

void C2UFCSSwitch::setup() {

}
void C2UFCSSwitch::dump_config() {

}
void C2UFCSSwitch::write_state(bool state) {
  this->parent_->set_c2_ufcs(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
