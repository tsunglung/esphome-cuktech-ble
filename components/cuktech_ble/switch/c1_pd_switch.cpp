#include "c1_pd_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.c1_pd";

void C1PDSwitch::setup() {

}
void C1PDSwitch::dump_config() {

}
void C1PDSwitch::write_state(bool state) {
  this->parent_->set_c1_pd(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
