#include "c1_pps_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.c1_pps";

void C1PPSSwitch::setup() {

}
void C1PPSSwitch::dump_config() {

}
void C1PPSSwitch::write_state(bool state) {
  this->parent_->set_c1_pd(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
