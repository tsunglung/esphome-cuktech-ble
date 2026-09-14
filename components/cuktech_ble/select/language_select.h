#pragma once

#include "esphome/components/select/select.h"
#include "../cuktech_ble.h"

namespace esphome::cuktech_ble {

class LanguageSelect final : public select::Select, public Parented<CuktechBle> {
 public:
  LanguageSelect() = default;

 protected:
  void control(size_t index) override;
};

}  // namespace esphome::cuktech_ble
