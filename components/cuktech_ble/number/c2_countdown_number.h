#pragma once

#include "esphome/components/number/number.h"
#include "../cuktech_ble.h"

namespace esphome::cuktech_ble {

class C2CountdownNumber final : public number::Number, public Parented<CuktechBle> {
 protected:
  void control(float value) override;
};

}  // namespace esphome::cuktech_ble
