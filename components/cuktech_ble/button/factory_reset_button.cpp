#include "factory_reset_button.h"

namespace esphome::cuktech_ble {

void FactoryResetButton::press_action() { this->parent_->factory_reset(); }

}  // namespace esphome::cuktech_ble
