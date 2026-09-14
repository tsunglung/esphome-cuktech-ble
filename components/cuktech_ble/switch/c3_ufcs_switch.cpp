#include "c3_ufcs_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.c3_ufcs";

void C3UFCSSwitch::setup() {

}
void C3UFCSSwitch::dump_config() {

}
void C3UFCSSwitch::write_state(bool state) {
  this->parent_->set_c3_ufcs(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
