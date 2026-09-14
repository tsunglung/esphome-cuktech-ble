#include "a_ufcs_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.a_ufcs";

void AUFCSSwitch::setup() {

}
void AUFCSSwitch::dump_config() {

}
void AUFCSSwitch::write_state(bool state) {
  this->parent_->set_a_ufcs(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
