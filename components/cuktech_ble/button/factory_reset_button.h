#pragma once

#include "esphome/components/button/button.h"
#include "../cuktech_ble.h"

namespace esphome::cuktech_ble {

class FactoryResetButton final : public button::Button, public Parented<CuktechBle> {
 public:
  FactoryResetButton() = default;

 protected:
  void press_action() override;
};

}  // namespace esphome::cuktech_ble
