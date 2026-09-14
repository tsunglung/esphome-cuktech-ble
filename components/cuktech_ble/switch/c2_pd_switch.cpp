#include "c2_pd_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.c2_pd";

void C2PDSwitch::setup() {

}
void C2PDSwitch::dump_config() {

}
void C2PDSwitch::write_state(bool state) {
  this->parent_->set_c2_pd(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
