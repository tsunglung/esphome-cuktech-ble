#include "c2_pps_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.c1_pps";

void C2PPSSwitch::setup() {

}
void C2PPSSwitch::dump_config() {

}
void C2PPSSwitch::write_state(bool state) {
  this->parent_->set_c2_pd(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
