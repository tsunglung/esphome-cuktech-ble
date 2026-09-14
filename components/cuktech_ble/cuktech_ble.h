#pragma once

#ifdef USE_ESP32

#include "esphome/core/component.h"
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#include <string>
#include <cstdint>

// --- Timing ---
#define KEEPALIVE_INTERVAL_MS    10000

typedef enum {
    BLE_IDLE, BLE_SCANNING, BLE_CONNECTING, BLE_AUTHENTICATING,
    BLE_READY, BLE_RECONNECT
} BLEState;

typedef void (*StateCallback)(BLEState old_state, BLEState new_state);
typedef void (*PortDataCallback)(int piid);

// Port data from notifications
typedef struct {
    float voltage, current, power;
    uint8_t protocol, status;
    bool active;
} PortInfo;

#define NOTIF_ITEM_SIZE  256
#define NOTIF_QUEUE_LEN  8
#define NOTIF_QUEUE_LEN_AUTH  2  // auth queues only used briefly during startup

typedef struct { uint8_t data[NOTIF_ITEM_SIZE]; size_t len; uint16_t conn_handle, attr_handle; } NotifItem;
typedef struct { float voltage, current, power; uint8_t protocol, status; bool active; } PortData;

namespace esphome {
namespace cuktech_ble {

static PortDataCallback g_port_data_cb = NULL;

class CuktechBle : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void on_shutdown() override;

  void set_device_name(const std::string &name) { this->device_name_ = name; }
  void set_ble_mac(const std::string &data) { this->ble_mac_ = data; }
  void set_ble_token(const std::string &data) { this->ble_token_ = data; }
  void set_ble_key(const std::string &data) { this->ble_key_ = data; }

  // ---- Called from NimBLE callback context ----
  void on_connect_state_change(bool connected);
  void on_live_data_notify(const uint8_t *data, size_t len);

  void clear_bonding();
  void check_config();

  void factory_reset();

  void set_enable_controlling(bool enable);
  void set_screen_dir_lock(bool enable);
  void set_screen_saver(bool enable);
  void set_c1_port(bool enable);
  void set_c2_port(bool enable);
  void set_c3_port(bool enable);
  void set_a_port(bool enable);
  void set_c1_pd(bool enable);
  void set_c1_pps(bool enable);
  void set_c1_ufcs(bool enable);
  void set_c2_pd(bool enable);
  void set_c2_pps(bool enable);
  void set_c2_ufcs(bool enable);
  void set_c3_scp(bool enable);
  void set_c3_ufcs(bool enable);
  void set_a_scp(bool enable);
  void set_a_ufcs(bool enable);
  void set_a_always_on(bool enable);

  void set_scene_mode(const char *state);
  void set_language(const char *state);
  void set_screen_saver_timeout(const char *state);
  void set_c1_countdown(float value);
  void set_c2_countdown(float value);
  void set_c3_countdown(float value);
  void set_a_countdown(float value);
  bool controlling_enabled();

 protected:
  std::string device_name_{"ESPHome Cuktech BLE Bridge"};
  std::string ble_mac_{""};
  std::string ble_token_{""};
  std::string ble_key_{""};
  PortData ble_ports[4];
  uint32_t ble_settings[32];
  bool ble_settings_valid[32];
  StateCallback state_cb_{NULL};

  // Connection state plumbing
  bool pending_connected_state_{false};
  bool last_published_connected_{false};
  bool connection_dirty_{false};

  uint64_t last_cd_fetch_slow_{0};
  uint64_t last_cd_fetch_fast_{0};

  // Last decoded values (held across notifications since fields may be sparse)
  PortData latest_;
  bool data_dirty_{false};

#ifdef USE_BINARY_SENSOR
  SUB_BINARY_SENSOR(connected)
  SUB_BINARY_SENSOR(c1_active)
  SUB_BINARY_SENSOR(c2_active)
  SUB_BINARY_SENSOR(c3_active)
  SUB_BINARY_SENSOR(a_active)
#endif

#ifdef USE_BUTTON
  SUB_BUTTON(factory_reset)
#endif

#ifdef USE_NUMBER
  SUB_NUMBER(c1_countdown)
  SUB_NUMBER(c2_countdown)
  SUB_NUMBER(c3_countdown)
  SUB_NUMBER(a_countdown)
#endif

#ifdef USE_SELECT
  SUB_SELECT(scene_mode)
  SUB_SELECT(language)
  SUB_SELECT(screen_saver_timeout)
#endif

#ifdef USE_SENSOR
  SUB_SENSOR(total_power)
  SUB_SENSOR(c1_power)
  SUB_SENSOR(c2_power)
  SUB_SENSOR(c3_power)
  SUB_SENSOR(a_power)
  SUB_SENSOR(c1_current)
  SUB_SENSOR(c2_current)
  SUB_SENSOR(c3_current)
  SUB_SENSOR(a_current)
  SUB_SENSOR(c1_voltage)
  SUB_SENSOR(c2_voltage)
  SUB_SENSOR(c3_voltage)
  SUB_SENSOR(a_voltage)
#endif

#ifdef USE_SWITCH
  SUB_SWITCH(enable_controlling)
  SUB_SWITCH(screen_dir_lock)
  SUB_SWITCH(screen_saver)
  SUB_SWITCH(c1_port)
  SUB_SWITCH(c2_port)
  SUB_SWITCH(c3_port)
  SUB_SWITCH(a_port)
  SUB_SWITCH(c1_pd)
  SUB_SWITCH(c1_pps)
  SUB_SWITCH(c1_ufcs)
  SUB_SWITCH(c2_pd)
  SUB_SWITCH(c2_pps)
  SUB_SWITCH(c2_ufcs)
  SUB_SWITCH(c3_scp)
  SUB_SWITCH(c3_ufcs)
  SUB_SWITCH(a_scp)
  SUB_SWITCH(a_ufcs)
  SUB_SWITCH(a_always_on)
#endif

#ifdef USE_TEXT_SENSOR
  SUB_TEXT_SENSOR(c1_protocol)
  SUB_TEXT_SENSOR(c2_protocol)
  SUB_TEXT_SENSOR(c3_protocol)
  SUB_TEXT_SENSOR(a_protocol)
#endif

  void publish_portdata_();
  void publish_settings_();
};

}  // namespace cuktech_ble
}  // namespace esphome

#endif
