#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/components/switch/switch.h"
#include "../cuktech_ble.h"

namespace esphome::cuktech_ble {

class APortSwitch final : public switch_::Switch, public Component {
 public:
  void set_parent(CuktechBle *parent) { this->parent_ = parent; };
  void set_config_name(const std::string &name) { this->config_name_ = name; }

  void setup() override;
  void dump_config() override;

 protected:
  void write_state(bool state) override;
  std::string config_name_;

  CuktechBle *parent_;
};

}  // namespace esphome::cuktech_ble
