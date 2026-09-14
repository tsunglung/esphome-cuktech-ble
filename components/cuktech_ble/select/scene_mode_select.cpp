#include "scene_mode_select.h"

namespace esphome::cuktech_ble {

void SceneModeSelect::control(size_t index) {
  this->publish_state(index);
  this->parent_->set_scene_mode(this->option_at(index));
}

}  // namespace esphome::cuktech_ble
