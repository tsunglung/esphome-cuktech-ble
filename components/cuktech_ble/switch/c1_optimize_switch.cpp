#include "c1_optimize_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.c1_optimize";

void C1OptimizeSwitch::setup() {

}
void C1OptimizeSwitch::dump_config() {

}
void C1OptimizeSwitch::write_state(bool state) {
  this->parent_->set_c1_optimize(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
