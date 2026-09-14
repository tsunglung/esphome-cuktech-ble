#include "language_select.h"

namespace esphome::cuktech_ble {

void LanguageSelect::control(size_t index) {
  this->publish_state(index);
  this->parent_->set_language(this->option_at(index));
}

}  // namespace esphome::cuktech_ble
