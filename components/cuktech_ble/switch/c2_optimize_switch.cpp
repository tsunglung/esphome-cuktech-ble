#include "c2_optimize_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.c2_optimize";

void C2OptimizeSwitch::setup() {

}
void C2OptimizeSwitch::dump_config() {

}
void C2OptimizeSwitch::write_state(bool state) {
  this->parent_->set_c2_optimize(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
