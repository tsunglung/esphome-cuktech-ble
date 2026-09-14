#include "a_scp_switch.h"
#include "esphome/core/log.h"

namespace esphome::cuktech_ble {

static const char *const TAG = "switch.a_ufcs";

void ASCPSwitch::setup() {

}
void ASCPSwitch::dump_config() {

}
void ASCPSwitch::write_state(bool state) {
  this->parent_->set_c3_scp(state);
  this->publish_state(state);
}

}  // namespace esphome::cuktech_ble
