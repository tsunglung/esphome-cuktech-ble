#ifdef USE_ESP32

#include "cuktech_ble.h"
#include "miot_protocol.h"
#include "miot_auth.h"
#include "queue_msg.h"
#include "miot_protocol_shared.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/application.h"

extern "C" {
#include "esp_bt.h"
#include "esp_log.h"
#include "esp_random.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "nimble/hci_common.h"  // BLE_HCI_ADV_FILT_* advertising filter policies
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
// NVS-backed bond store (provided by NimBLE's ble_store_config helper)
void ble_store_config_init(void);
}

namespace esphome {
namespace cuktech_ble {

static const char *const TAG = "cuktech_ble";

// ============================================================
// Queues
// ============================================================
QueueHandle_t cmd_queue = NULL;
QueueHandle_t urgent_queue = NULL;  // higher priority: port commands, user SETs
QueueHandle_t result_queue = NULL;

// Singleton pointer so static C-callbacks can reach the instance.
static CuktechBle *g_instance = nullptr;

// ---- Forward decls for static callbacks -------------------------------------
static void do_nimble_host_task(void *param);
static void do_stack_reset(int reason);
static void do_nimble_on_sync();
static void start_discover(void);
static void do_handle_cmd_recv(void);
static void do_handle_cmd_send(void);
static void do_pending_check_timeouts(void);
static void start_keepalive(void);
static void start_disconnect(void);
static void start_reconnect(void);
static void do_disable_all_notifications(void);
static int on_gap_event(struct ble_gap_event *event, void *arg);
static int do_disc_svc_cb(uint16_t conn_handle, const struct ble_gatt_error *error,
                         const struct ble_gatt_svc *service, void *arg);
static int do_disc_chr_cb(uint16_t conn_handle, const struct ble_gatt_error *error,
                         const struct ble_gatt_chr *chr, void *arg);

static void do_set_state(BLEState s);

// Definition of the cache declared above.
std::string g_device_name_cache;

static BLEState g_state = BLE_IDLE;
static uint16_t g_conn_handle = 0xFFFF;
static bool g_connected = false;
static uint16_t g_auth_ctrl_handle = 0, g_auth_data_handle = 0, g_cmd_send_handle = 0, g_cmd_recv_handle = 0, g_ver_read_handle = 0;
static QueueHandle_t g_q_auth_ctrl = NULL, g_q_auth_data = NULL, g_q_cmd_send = NULL, g_q_cmd_recv = NULL, g_q_ver_read = NULL;
static SessionKeys g_keys = {};
static uint32_t g_send_it = 0;
static uint8_t g_seq = 1;
static PortData g_ports[4] = {};
static uint32_t g_settings[32] = {};
static bool g_settings_valid[32] = {};
static portMUX_TYPE _settings_spinlock = portMUX_INITIALIZER_UNLOCKED;
#define LOCK_SETTINGS()   portENTER_CRITICAL(&_settings_spinlock)
#define UNLOCK_SETTINGS() portEXIT_CRITICAL(&_settings_spinlock)
static uint32_t g_ra = 0;
static uint64_t g_ra_ts = 0;
static uint64_t g_last_keepalive = 0;
static SemaphoreHandle_t g_disc_sem = NULL, g_connected_sem = NULL, g_disconnect_sem = NULL;
static uint16_t g_disc_service_start = 0, g_disc_service_end = 0;
static uint8_t g_target_addr[6] = {}, g_token[12] = {};
static char g_mac_str[18] = {};
static char g_ver_str[20] = {};
static volatile bool g_nimble_ready = false;  /* written by NimBLE host task, read by ble_task */
static volatile bool g_enabled = true;

typedef struct { uint16_t uuid; uint16_t *handle; const char *name; } CharCtx;
static CharCtx g_char_ctx[5];
static int g_char_ctx_n = 0;

static bool g_setings_dirty = false;
static bool g_data_dirty = false;
#define PIID21_ALL_ON  0x03030F0F
static uint32_t g_protocol_extend_val = PIID21_ALL_ON;
static bool g_protocol_extend_valid = true;
static uint32_t g_port_ctrl_val = 0xFF;  // assume all ports on (can't GET to read actual)
static bool g_port_ctrl_valid = true;    // track SET updates
static uint64_t last_set16_time = 0;
static uint64_t last_set_time = 0;   // any SET command (used for push/GET debounce during transitions)
static uint8_t last_set_piid = 0;    // piid of last SET — distinguish port control from protocol change

// ---- Helpers ----------------------------------------------------------------
__attribute__((unused)) static void log_hex(const char *prefix, const uint8_t *buf, size_t len) {
  // Log up to 64 bytes inline to keep the log readable.
  char hex[3 * 64 + 1];
  size_t to_print = len < 64 ? len : 64;
  for (size_t i = 0; i < to_print; i++) {
    snprintf(&hex[i * 3], 4, "%02x ", buf[i]);
  }
  hex[to_print > 0 ? (to_print * 3 - 1) : 0] = '\0';
  ESP_LOGV(TAG, "%s len=%u: %s%s", prefix, (unsigned) len, hex, len > 64 ? " …" : "");
}

/* ============================================================
 * Async pending command table
 * ============================================================ */
#define MAX_PENDING 12
#define CIPHER_BUF_SIZE 128

typedef enum { PENDING_GET, PENDING_SET } PendingType;
typedef enum { SEND_IDLE, SEND_AWAIT_RDY, SEND_AWAIT_OK, SEND_AWAIT_RESP } SendPhase;

typedef struct {
  bool      in_use;
  uint8_t   seq;
  uint8_t   piid;
  PendingType type;
  uint32_t  send_value;
  uint32_t  poll_seq;
  uint64_t  deadline;
  bool      acked;
  bool      no_result;
  SendPhase phase;
  uint8_t   ciphertext[CIPHER_BUF_SIZE];  // [1,0] + encrypted data
  size_t    ct_len;
} PendingEntry;

static PendingEntry g_pending[MAX_PENDING] = {};

static void do_pending_check_timeouts(void) {
  uint64_t now = esp_timer_get_time() / 1000;
  for (int i = 0; i < MAX_PENDING; i++) {
    if (!g_pending[i].in_use || now < g_pending[i].deadline) continue;
    ESP_LOGW(TAG, "Pending %s seq=%d piid=%d timed out",
              g_pending[i].type == PENDING_GET ? "GET" : "SET",
              g_pending[i].seq, g_pending[i].piid);
    BleResult r;
    if (g_pending[i].type == PENDING_GET) {
        r = (BleResult){RES_GET, false, g_pending[i].piid, 0, g_pending[i].poll_seq};
    } else {
        r = (BleResult){RES_SET, false, g_pending[i].piid, g_pending[i].send_value, 0};
    }
    xQueueSend(result_queue, &r, pdMS_TO_TICKS(50));
    g_pending[i].in_use = false;
  }
}

static void do_pending_match(const uint8_t *resp, size_t rl) {
  if (rl < 6) return;
  uint8_t seq = resp[2];
  uint8_t op  = resp[4];
  for (int i = 0; i < MAX_PENDING; i++) {
    if (!g_pending[i].in_use || g_pending[i].seq != seq) continue;
    if (g_pending[i].type == PENDING_GET && op == 0x03) {
        uint32_t val = 0;
        uint8_t vlen = (rl >= 12) ? resp[11] : 0;
        if (vlen >= 4 && rl >= 17)
            val = resp[13] | (resp[14] << 8) | (resp[15] << 16) | (resp[16] << 24);
        else if (vlen >= 1 && rl >= 14)
            val = resp[13];
        BleResult r = {RES_GET, true, g_pending[i].piid, val, g_pending[i].poll_seq};
        xQueueSend(result_queue, &r, pdMS_TO_TICKS(50));
        g_pending[i].in_use = false;
        return;
    }
    if (g_pending[i].type == PENDING_SET && op == 0x01) {
        g_pending[i].acked = true;
        return;
    }
    if (g_pending[i].type == PENDING_SET && op == 0x04) {
        if (!g_pending[i].no_result) {
            bool piid_ok = (rl >= 8 && resp[7] == g_pending[i].piid);
            BleResult r = {RES_SET, piid_ok, g_pending[i].piid, g_pending[i].send_value, 0};
            xQueueSend(result_queue, &r, pdMS_TO_TICKS(50));
        }
        g_pending[i].in_use = false;
        return;
    }
  }
}

/* ============================================================
 * NimBLE sync / state machine
 * ============================================================ */
static void do_nimble_on_sync(void) {
  ESP_LOGV(TAG, "NimBLE host synced");
  int rc = ble_hs_util_ensure_addr(0);
  if (rc != 0) {
    ESP_LOGE(TAG, "ensure_addr failed: %d", rc);
    return;
  }
  ble_svc_gap_device_name_set(g_device_name_cache.c_str());
  g_nimble_ready = true;
  ESP_LOGV(TAG, "NimBLE ready");
}

static void do_nimble_host_task(void *param) {
  ESP_LOGV(TAG, "NimBLE host task started");
  nimble_port_run();
  ESP_LOGV(TAG, "NimBLE host task done");
  nimble_port_freertos_deinit();
}

 // ---- Stack lifecycle --------------------------------------------------------
static void do_stack_reset(int reason) {
  ESP_LOGW(TAG, "BLE stack reset, reason=%d", reason);
}

static void do_dispatch_notif(uint16_t attr_handle, const uint8_t *data, size_t len) {
  NotifItem item;
  item.conn_handle = g_conn_handle;
  item.attr_handle = attr_handle;
  item.len = (len > NOTIF_ITEM_SIZE) ? NOTIF_ITEM_SIZE : len;
  memcpy(item.data, data, item.len);

  QueueHandle_t q = NULL;
  if (attr_handle == g_auth_ctrl_handle)  q = g_q_auth_ctrl;
  else if (attr_handle == g_auth_data_handle) q = g_q_auth_data;
  else if (attr_handle == g_cmd_send_handle)  q = g_q_cmd_send;
  else if (attr_handle == g_cmd_recv_handle)  q = g_q_cmd_recv;
  else if (attr_handle == g_ver_read_handle)  q = g_q_ver_read;

  if (q) {
    if (xQueueSend(q, &item, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Queue full for handle 0x%04X, dropping", attr_handle);
    }
  }
}

/* ============================================================
 * Queue helpers (non-blocking)
 * ============================================================ */
static bool do_wait_queue(QueueHandle_t q, uint8_t *buf, size_t *len, uint32_t ms) {
  NotifItem item;
  App.feed_wdt();
  if (xQueueReceive(q, &item, pdMS_TO_TICKS(ms)) != pdTRUE) return false;
  size_t n = (item.len > 256) ? 256 : item.len;
  memcpy(buf, item.data, n);
  *len = n;
  return true;
}

static bool do_pop_cmd_recv(uint8_t *buf, size_t *len) {
  return do_wait_queue(g_q_cmd_recv, buf, len, 0);
}

static bool do_pop_cmd_send(uint8_t *buf, size_t *len) {
  return do_wait_queue(g_q_cmd_send, buf, len, 0);
}

static void do_drain_all_queues(void) {
  NotifItem item;
  QueueHandle_t qs[] = {g_q_auth_ctrl, g_q_auth_data, g_q_cmd_send, g_q_cmd_recv};
  for (int i = 0; i < 4; i++) while (xQueueReceive(qs[i], &item, 0) == pdTRUE) {}
}

/* ============================================================
 * BLE read/write helpers
 * ============================================================ */
static bool on_wru_nr(uint16_t handle, const uint8_t *data, size_t len) {
  if (!g_connected || handle == 0) return false;
  struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
  if (!om) return false;
  return ble_gattc_write_no_rsp(g_conn_handle, handle, om) == 0;
}

static int on_read_cb(uint16_t conn_handle,
                       const struct ble_gatt_error *error,
                       struct ble_gatt_attr *attr,
                       void *arg) {
  uint8_t *buf = (uint8_t *)arg;

  ESP_LOGV(TAG, "Read complete for the sub char; "
                "s=%d h=%d, len=%d", error->status, conn_handle, attr->om->om_len);
  log_hex(TAG, attr->om->om_data, attr->om->om_len);
  memcpy(buf, attr->om->om_data, attr->om->om_len);

  return 0;
}

static bool on_rdu_rsp(uint16_t handle, const uint8_t *data, size_t len, uint8_t *buf) {
  if (!g_connected || handle == 0) return false;
  return ble_gattc_read(g_conn_handle, handle, on_read_cb, buf) == 0;
}

static SemaphoreHandle_t g_op_sem = NULL;
static int g_op_rc = 0;
static uint32_t g_op_seq = 0, g_op_expected_seq = 0;

static int on_gatt_write_cb(uint16_t conn_handle, const struct ble_gatt_error *error,
                           struct ble_gatt_attr *attr, void *arg) {
  uint32_t cb_seq = (uint32_t)(uintptr_t)arg;
  if (cb_seq != g_op_expected_seq) return 0;
  g_op_rc = (error != NULL) ? error->status : 0;
  if (g_op_sem) xSemaphoreGive(g_op_sem);
  return 0;
}

static bool on_wru(uint16_t handle, const uint8_t *data, size_t len) {
  if (!g_connected || handle == 0) return false;
  struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
  if (!om) return false;
  g_op_rc = -1;
  g_op_expected_seq = ++g_op_seq;
  xSemaphoreTake(g_op_sem, 0);
  int rc = ble_gattc_write(g_conn_handle, handle, om, on_gatt_write_cb,
                            (void*)(uintptr_t)g_op_expected_seq);
  if (rc != 0) return false;
  if (xSemaphoreTake(g_op_sem, pdMS_TO_TICKS(3000)) != pdTRUE) return false;
  return g_op_rc == 0;
}

// ---- GATT discovery callbacks ----------------------------------------------
static int do_disc_svc_cb(uint16_t conn_handle, const struct ble_gatt_error *error,
                         const struct ble_gatt_svc *service, void *arg) {
  if (error && error->status != 0) { if (g_disc_sem) xSemaphoreGive(g_disc_sem); return 0; }
  if (service == NULL) { ESP_LOGV(TAG, "Svc discovery complete"); if (g_disc_sem) xSemaphoreGive(g_disc_sem); return 0; }
  if (service->uuid.u16.value == 0xFE95) { g_disc_service_start = service->start_handle; g_disc_service_end = service->end_handle; ESP_LOGV(TAG, "MiOT service: 0x%04X-0x%04X", service->start_handle, service->end_handle); }
  return 0;
}

static int do_disc_chr_cb(uint16_t conn_handle, const struct ble_gatt_error *error,
                         const struct ble_gatt_chr *chr, void *arg) {
  if (error && error->status != 0) { if (g_disc_sem) xSemaphoreGive(g_disc_sem); return 0; }
  if (chr == NULL) { if (g_disc_sem) xSemaphoreGive(g_disc_sem); return 0; }
  for (int i = 0; i < g_char_ctx_n; i++) {
      if (chr->uuid.u16.value == g_char_ctx[i].uuid) { *g_char_ctx[i].handle = chr->val_handle; ESP_LOGV(TAG, "%s handle=0x%04X", g_char_ctx[i].name, chr->val_handle); break; }
  }
  return 0;
}

static void do_disable_all_notifications(void) {
  if (!g_connected) return;
  uint8_t val[] = {0x00, 0x00};
  uint16_t handles[] = {g_auth_ctrl_handle, g_auth_data_handle, g_cmd_send_handle, g_cmd_recv_handle, g_ver_read_handle};
  for (int i = 0; i < 4; i++) { if (handles[i]) { on_wru_nr(handles[i] + 1, val, 2); vTaskDelay(pdMS_TO_TICKS(30)); } }
  vTaskDelay(pdMS_TO_TICKS(100));
}

static void do_set_state(BLEState s) {
  if (s == BLE_RECONNECT && !g_enabled) s = BLE_IDLE;
  if (s == BLE_SCANNING && !g_enabled) s = BLE_IDLE;
  BLEState old = g_state;
  g_state = s;
  ESP_LOGV(TAG, "State: %d -> %d", (int)old, (int)s);
  if (g_state == BLE_READY) {
      // Fetch settings on initial BLE connect so frontend shows
      // actual values instead of hardcoded defaults, and protocol
      // detection uses real hardware protocol codes (PIID 17/18).
      static const uint8_t INIT_PIIDS[] = {5, 6, 15, 21, 17, 18};
      for (int i = 0; i < sizeof(INIT_PIIDS); i++) {
          BleCommand c = {CMD_GET, INIT_PIIDS[i], 0, 0};
          xQueueSend(cmd_queue, &c, 0);
      }
  }
}

/* ============================================================
 * Port data parsing + protocol estimation
 * ============================================================ */
// Hardware protocol code from PIID 17/18 (byte positions aligned with Python ble_manager.py):
// PIID 17: byte[3] = C1, byte[1] = C2; PIID 18: byte[3] = C3, byte[1] = A
static uint8_t do_get_hw_proto(uint8_t piid) {
  LOCK_SETTINGS();
  uint8_t result = 0;
  switch (piid) {
    case 1: result = (g_settings_valid[17] && ((g_settings[17] >> 24) & 0xFF) > 0)
                    ? (g_settings[17] >> 24) & 0xFF : 0; break;
    case 2: result = (g_settings_valid[17] && ((g_settings[17] >> 8) & 0xFF) > 0)
                    ? (g_settings[17] >> 8) & 0xFF : 0; break;
    case 3: result = (g_settings_valid[18] && ((g_settings[18] >> 24) & 0xFF) > 0)
                    ? (g_settings[18] >> 24) & 0xFF : 0; break;
    case 4: result = (g_settings_valid[18] && ((g_settings[18] >> 8) & 0xFF) > 0)
                    ? (g_settings[18] >> 8) & 0xFF : 0; break;
  }
  UNLOCK_SETTINGS();
  return result;
}

static uint8_t do_estimate_proto(uint8_t piid, float voltage, uint8_t code, uint8_t hw_protocol) {
  if (hw_protocol > 0) return hw_protocol;
  LOCK_SETTINGS();
  int pd_bit = (piid == 1) ? 0 : 8;
  int pps_bit = (piid == 1) ? 1 : 9;
  bool pd_enabled = (g_settings_valid[21]) ? ((g_settings[21] >> pd_bit) & 1) : true;
  bool pps_enabled = (g_settings_valid[21]) ? ((g_settings[21] >> pps_bit) & 1) : true;
  uint8_t pdo_kind = 0;
  if ((piid == 1 || piid == 2) && g_settings_valid[17]) {
    uint16_t port_word = (piid == 1) ? (g_settings[17] & 0xFFFF) : ((g_settings[17] >> 16) & 0xFFFF);
    pdo_kind = (port_word >> 8) & 0xFF;
  } else if ((piid == 3 || piid == 4) && g_settings_valid[18]) {
    /* C3 (piid=3) = lower 16 bits; A (piid=4) = upper 16 bits (same layout as PIID 17) */
    uint16_t port_word = (piid == 3) ? (g_settings[18] & 0xFFFF) : ((g_settings[18] >> 16) & 0xFFFF);
    pdo_kind = (port_word >> 8) & 0xFF;
  }
  UNLOCK_SETTINGS();
  return estimate_protocol_shared(piid, voltage, code, pd_enabled, pps_enabled, pdo_kind, hw_protocol);
}

static void do_parse_port(uint8_t piid, const uint8_t *pt, size_t pt_len) {
  if (piid < 1 || piid > 4 || pt_len < 12) return;
  uint32_t val = pt[pt_len-4] | (pt[pt_len-3] << 8) |
                  (pt[pt_len-2] << 16) | (pt[pt_len-1] << 24);
  uint8_t idx = piid - 1;
  g_ports[idx].voltage = ((val >> 24) & 0xFF) / 10.0f;
  g_ports[idx].current = ((val >> 16) & 0xFF) / 10.0f;
  g_ports[idx].protocol = do_estimate_proto(piid, g_ports[idx].voltage, (val >> 8) & 0xFF, do_get_hw_proto(piid));
  g_ports[idx].status = val & 0xFF;
  g_ports[idx].power = g_ports[idx].voltage * g_ports[idx].current;
  g_ports[idx].active = (g_ports[idx].status != 0) || (g_ports[idx].voltage > 0.5f);
  ESP_LOGV(TAG, "Port%d: V=%.1f I=%.1f P=%.1f proto=%d st=0x%02X",
            piid, g_ports[idx].voltage, g_ports[idx].current,
            g_ports[idx].power, g_ports[idx].protocol, g_ports[idx].status);
  if (result_queue) {
    BleResult r = {RES_PORT_PUSH, true, piid, 0, 0,
                    g_ports[idx].voltage, g_ports[idx].current, g_ports[idx].power,
                    g_ports[idx].protocol, g_ports[idx].status, false};
    if (xQueueSend(result_queue, &r, 0) != pdTRUE)
      ESP_LOGW(TAG, "result_queue full, dropping push");
  }
}

/* ============================================================
 * Auth helpers (still synchronous — only used during auth flow)
 * ============================================================ */
static bool do_recv_auth(uint8_t *out, size_t *out_len, uint32_t ms) {
  uint64_t start = esp_timer_get_time() / 1000;
  uint8_t buf[256]; size_t blen = 0;
  while ((esp_timer_get_time() / 1000 - start) < ms) {
      uint32_t rem = ms - (uint32_t)(esp_timer_get_time() / 1000 - start);
      if (!do_wait_queue(g_q_auth_data, buf, &blen, (rem > 3000) ? 3000 : rem)) break;
      if (blen < 4) continue;
      uint8_t atype = buf[2];
      if (atype == 0x01) continue;
      if ((atype == 0x02 || atype == 0x04) && blen >= 4) {
          size_t pl = blen - 4;
          if (pl > 256) pl = 256;
          memcpy(out, buf + 4, pl); *out_len = pl;
          on_wru_nr(g_auth_data_handle, (uint8_t[]){0,0,3,0}, 4);
          return true;
      }
      if (atype == 0x00 && blen >= 6) {
          uint16_t cnt = buf[4] | (buf[5] << 8);
          if (cnt > 100) cnt = 100;
          on_wru_nr(g_auth_data_handle, (uint8_t[]){0,0,1,1}, 4);
          size_t total = 0;
          for (uint16_t i = 0; i < cnt && total < 508; i++) {
              NotifItem item;
              if (xQueueReceive(g_q_auth_data, &item, pdMS_TO_TICKS(3000)) != pdTRUE) break;
              if (item.len > 2) {
                  size_t cp = (item.len - 2 < 508 - total) ? (item.len - 2) : (508 - total);
                  memcpy(out + total, item.data + 2, cp); total += cp;
              }
          }
          on_wru_nr(g_auth_data_handle, (uint8_t[]){0,0,1,0}, 4);
          *out_len = total; return total > 0;
      }
  }
  *out_len = 0; return false;
}

static bool do_recv_auth_raw(uint8_t *out, size_t *out_len, uint32_t ms) {
  uint64_t start = esp_timer_get_time() / 1000;
  while ((esp_timer_get_time() / 1000 - start) < ms) {
    uint32_t rem = ms - (uint32_t)(esp_timer_get_time() / 1000 - start);
    if (!do_wait_queue(g_q_auth_data, out, out_len, (rem > 3000) ? 3000 : rem)) break;
    if (*out_len < 4) continue;
    if (out[2] == 0x01) continue;
    if (out[2] == 0x02 || out[2] == 0x04) return true;
  }
  *out_len = 0; return false;
}

static bool do_wait_notif_auth(const uint8_t *expected, size_t elen, uint32_t ms) {
  uint8_t buf[256]; size_t blen;
  uint64_t deadline = esp_timer_get_time() / 1000 + ms;
  while ((esp_timer_get_time() / 1000) < deadline) {
    uint32_t rem = deadline - (esp_timer_get_time() / 1000);
    App.feed_wdt();
    if (!do_wait_queue(g_q_auth_data, buf, &blen, (rem > 3000) ? 3000 : rem)) break;
    if (blen == elen && memcmp(buf, expected, elen) == 0) return true;
  }
  return false;
}

static void do_drain_auth_queue(void) {
    NotifItem item;
    while (xQueueReceive(g_q_auth_data, &item, 0) == pdTRUE) {}
}

/* ============================================================
 * Auth flow (synchronous — used only during initial auth)
 * ============================================================ */
static bool do_auth_sync_send_ctrl(const uint8_t *data, size_t len) {
  return on_wru_nr(g_auth_ctrl_handle, data, len);
}

static bool do_auth_sync_send_data(const uint8_t *data, size_t len) {
  return on_wru_nr(g_auth_data_handle, data, len);
}

static int start_auth(void) {
  ESP_LOGV(TAG, "Auth start");
  uint8_t buf[512]; size_t blen;
  uint8_t rand_key[16], dev_random[16], dev_hmac[32], our_hmac[32];
  SessionKeys keys = {};

  App.feed_wdt();
  ESP_LOGV(TAG, "Phase A: init (0xA4)");
  do_drain_auth_queue();
  if (!do_auth_sync_send_ctrl((uint8_t[]){0xA4}, 1)) return -1;
  if (!do_recv_auth_raw(buf, &blen, 3000) || blen < 4) { ESP_LOGE(TAG, "Phase A: no init response"); return -1; }
  ESP_LOGV(TAG, "Phase A: type=0x%02X len=%d", buf[2], (int)blen);
  buf[2]++; do_auth_sync_send_data(buf, blen);

  uint64_t pa_deadline = esp_timer_get_time() / 1000 + 8000;
  bool got_key_data = false;
  while ((esp_timer_get_time() / 1000) < pa_deadline) {
    uint32_t rem = pa_deadline - (esp_timer_get_time() / 1000);
    if (!do_recv_auth_raw(buf, &blen, (rem > 3000) ? 3000 : rem)) break;
    if (blen < 4) continue;
    if (buf[2] == 0x04 && blen >= 20) { got_key_data = true; break; }
  }
  if (!got_key_data) { ESP_LOGE(TAG, "Phase A: no key exchange data"); return -1; }

  size_t pad_len = (blen > 4) ? (blen - 4) : 0;
  uint8_t placeholder[512] = {0, 0, 5, 1};
  memset(placeholder + 4, 0xF2, pad_len);
  do_auth_sync_send_data(placeholder, 4 + pad_len);
  vTaskDelay(pdMS_TO_TICKS(600)); do_drain_auth_queue();

  ESP_LOGV(TAG, "Phase B: key exchange");
  vTaskDelay(pdMS_TO_TICKS(50));
  do_auth_sync_send_ctrl((uint8_t[]){0x24, 0, 0, 0}, 4);

  esp_fill_random(rand_key, 16);
  do_auth_sync_send_data((uint8_t[]){0, 0, 0, 0x0B, 1, 0}, 6);

  uint8_t rcv_rdy[] = {0, 0, 1, 1};
  bool got_rdy = false;
  for (int r = 0; r < 5; r++) { if (do_wait_notif_auth(rcv_rdy, 4, 3000)) { got_rdy = true; break; } }
  if (!got_rdy) { ESP_LOGE(TAG, "Phase B: no RCV_RDY"); return -1; }

  uint8_t key_frame[18] = {1, 0};
  memcpy(key_frame + 2, rand_key, 16);
  do_auth_sync_send_data(key_frame, 18);

  uint8_t rcv_ok[] = {0, 0, 1, 0};
  if (!do_wait_notif_auth(rcv_ok, 4, 3000)) { ESP_LOGE(TAG, "Phase B: no RCV_OK"); return -1; }
  if (!do_recv_auth(buf, &blen, 3000) || blen < 16) { ESP_LOGE(TAG, "Phase B: no dev key"); return -1; }
  memcpy(dev_random, buf, 16);
  if (!do_recv_auth(buf, &blen, 3000) || blen < 32) { ESP_LOGE(TAG, "Phase B: no dev HMAC"); return -1; }
  memcpy(dev_hmac, buf, 32);

  uint8_t salt[32]; memcpy(salt, rand_key, 16); memcpy(salt + 16, dev_random, 16);
  uint8_t salt_inv[32]; memcpy(salt_inv, dev_random, 16); memcpy(salt_inv + 16, rand_key, 16);
  if (!derive_session_keys(g_token, 12, rand_key, dev_random, &keys)) return -1;

  uint8_t expected_hmac[32];
  if (!hmac_sha256(keys.dev_key, 16, salt_inv, 32, expected_hmac)) return -1;

  if (memcmp(expected_hmac, dev_hmac, 32) != 0) { ESP_LOGE(TAG, "HMAC mismatch"); return -1; }
  ESP_LOGV(TAG, "HMAC verified OK");

  ESP_LOGV(TAG, "Phase C: send HMAC");
  if (!hmac_sha256(keys.app_key, 16, salt, 32, our_hmac)) return -1;

  do_auth_sync_send_data((uint8_t[]){0, 0, 0, 0x0A, 1, 0}, 6);
  got_rdy = false;
  for (int r = 0; r < 5; r++) { if (do_wait_notif_auth(rcv_rdy, 4, 3000)) { got_rdy = true; break; } }
  if (!got_rdy) { ESP_LOGE(TAG, "Phase C: no RCV_RDY"); return -1; }

  uint8_t hmac_frame[34] = {1, 0};
  memcpy(hmac_frame + 2, our_hmac, 32);
  do_auth_sync_send_data(hmac_frame, 34);
  do_wait_notif_auth(rcv_ok, 4, 3000);

  ESP_LOGV(TAG, "Waiting auth result...");
  uint64_t deadline = esp_timer_get_time() / 1000 + 8000;
  while ((esp_timer_get_time() / 1000) < deadline) {
    uint32_t rem = deadline - (esp_timer_get_time() / 1000);
    if (!do_wait_queue(g_q_auth_ctrl, buf, &blen, (rem > 3000) ? 3000 : rem)) break;
    if (blen >= 1) {
      uint8_t code = buf[0];
      ESP_LOGV(TAG, "Auth result: 0x%02X", code);
      if (code == AUTH_SUCCESS || code == AUTH_ACTIVATE) {
        g_keys = keys; g_send_it = 0; g_seq = 1; g_ra = 0;
        do_set_state(BLE_READY); g_last_keepalive = esp_timer_get_time() / 1000;
        ESP_LOGI(TAG, "Auth OK!");
        return 0;
      }
      ESP_LOGW(TAG, "Auth failed: 0x%02X", code); return -1;
    }
  }
  ESP_LOGE(TAG, "Auth timeout");
  return -1;
}

/* ============================================================
 * Handle CMD_RECV notifications (push + async response matching)
 * ============================================================ */
static void do_process_decrypted(const uint8_t *pt, size_t pt_len) {
  if (pt_len >= 12 && pt[4] == 0x04 && pt[7] >= 1 && pt[7] <= 4)
    do_parse_port(pt[7], pt, pt_len);   // port push
  else
    do_pending_match(pt, pt_len);        // command response
}

static void do_handle_cmd_recv(void) {
  uint8_t buf[256]; size_t blen;
  // Process ALL available items (not just one)
  while (do_pop_cmd_recv(buf, &blen)) {
    if (!g_connected) return;

    if (buf[2] == 0x00 && blen >= 6) {
      // Multi-frame
      uint16_t cnt = buf[4] + (buf[5] << 8);
      if (cnt > 100) cnt = 100;
      on_wru_nr(g_cmd_recv_handle, (uint8_t[]){0,0,1,1}, 4);
      uint8_t tmp[512]; size_t t = 0;
      for (uint16_t i = 0; i < cnt; i++) {
          NotifItem item;
          if (xQueueReceive(g_q_cmd_recv, &item, pdMS_TO_TICKS(3000)) != pdTRUE) break;
          if (item.data[2] == 0x02 && item.len >= 3) {
              size_t cp = (item.len - 2 < 508 - t) ? (item.len - 2) : (508 - t);
              memcpy(tmp + t, item.data + 2, cp); t += cp;
          }
      }
      on_wru_nr(g_cmd_recv_handle, (uint8_t[]){0,0,1,0}, 4);
      if (t > 0) {
          size_t pt_len;
          if (decrypt_response(&g_keys, tmp, t, tmp, &pt_len))
              do_process_decrypted(tmp, pt_len);
      }
    } else if (buf[2] == 0x02 && blen >= 4) {
      size_t pt_len;
      if (decrypt_response(&g_keys, buf + 4, blen - 4, buf + 4, &pt_len)) {
          on_wru_nr(g_cmd_recv_handle, (uint8_t[]){0,0,3,0}, 4);
          do_process_decrypted(buf + 4, pt_len);
      }
    }
  }
}

/* ============================================================
 * Handle CMD_SEND notifications (async send state machine)
 * ============================================================ */
static void do_handle_cmd_send(void) {
  uint8_t buf[256]; size_t blen;
  while (do_pop_cmd_send(buf, &blen)) {
    if (blen != 4) continue;

    // RCV_RDY [0,0,1,1] → write ciphertext from first pending SEND_AWAIT_RDY
    if (buf[2] == 0x01 && buf[3] == 0x01) {
      for (int i = 0; i < MAX_PENDING; i++) {
        if (g_pending[i].in_use && g_pending[i].phase == SEND_AWAIT_RDY) {
          on_wru_nr(g_cmd_send_handle, g_pending[i].ciphertext, g_pending[i].ct_len);
          g_pending[i].phase = SEND_AWAIT_OK;
          break;
        }
      }
    }
    // RCV_OK [0,0,1,0] → command fully sent
    else if (buf[2] == 0x01 && buf[3] == 0x00) {
      for (int i = 0; i < MAX_PENDING; i++) {
        if (g_pending[i].in_use && g_pending[i].phase == SEND_AWAIT_OK) {
          g_pending[i].phase = SEND_AWAIT_RESP;
          break;
        }
      }
    }
  }
}

/* ============================================================
 * Sync wrappers (for CMD_PORT and auth flow)
 * ============================================================ */
static void do_drain_cmd_recv_pushes(void) {
    NotifItem item;
    while (xQueueReceive(g_q_cmd_recv, &item, 0) == pdTRUE) {
        if (item.data[2] == 0x02 && item.len >= 4) {
            size_t tlen = 0; uint8_t pt[256];
            if (decrypt_response(&g_keys, item.data + 4, item.len - 4, pt, &tlen)) {
                if (tlen >= 12 && pt[4] == 0x04 && pt[7] >= 1 && pt[7] <= 4) {
                    on_wru_nr(g_cmd_recv_handle, (uint8_t[]){0,0,3,0}, 4);
                    do_parse_port(pt[7], pt, tlen);
                } else {
                    on_wru_nr(g_cmd_recv_handle, (uint8_t[]){0,0,3,0}, 4);
                }
            }
        }
    }
}

static bool do_wait_cmd_send(uint8_t *buf, size_t *blen, uint32_t ms) {
    uint64_t deadline = esp_timer_get_time() / 1000 + ms;
    while ((esp_timer_get_time() / 1000) < deadline) {
        if (do_pop_cmd_send(buf, blen)) return true;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return false;
}

static bool do_send_enc_sync(const uint8_t *pt, size_t pt_len) {
    uint8_t enc[512]; size_t el;
    if (!encrypt_command(&g_keys, &g_send_it, pt, pt_len, enc, &el)) return false;
    if (!on_wru_nr(g_cmd_send_handle, (uint8_t[]){0,0,0,0,1,0}, 6)) return false;
    uint8_t buf[256]; size_t blen;
    if (!do_wait_cmd_send(buf, &blen, 3000)) { ESP_LOGW(TAG, "CMD_SEND: no RCV_RDY"); return false; }
    if (blen != 4 || buf[2] != 1 || buf[3] != 1) { ESP_LOGW(TAG, "RCV_RDY mismatch"); return false; }
    uint8_t f[2+512] = {1,0}; memcpy(f + 2, enc, el);
    if (!on_wru_nr(g_cmd_send_handle, f, 2 + el)) return false;
    if (!do_wait_cmd_send(buf, &blen, 3000)) { ESP_LOGW(TAG, "CMD_SEND: no RCV_OK"); return false; }
    if (blen != 4 || buf[2] != 1 || buf[3] != 0) { ESP_LOGW(TAG, "RCV_OK mismatch"); return false; }
    return true;
}

static bool do_recv_cmd(uint8_t *out, size_t *ol, uint32_t ms) {
    uint64_t start = esp_timer_get_time() / 1000;
    uint8_t buf[256]; size_t blen;
    while ((esp_timer_get_time() / 1000 - start) < ms) {
        uint32_t rem = ms - (uint32_t)(esp_timer_get_time() / 1000 - start);
        if (!do_wait_queue(g_q_cmd_recv, buf, &blen, (rem > 2000) ? 2000 : rem)) break;
        if (blen >= 4 && buf[2] == 0x01) continue;
        if (buf[2] == 0x02 && blen >= 4) {
            uint8_t tmp[256]; size_t tlen = 0;
            if (!decrypt_response(&g_keys, buf + 4, blen - 4, tmp, &tlen)) continue;
            if (tlen >= 12 && tmp[4] == 0x04 && tmp[7] >= 1 && tmp[7] <= 4) {
                on_wru_nr(g_cmd_recv_handle, (uint8_t[]){0,0,3,0}, 4);
                do_parse_port(tmp[7], tmp, tlen); continue;
            }
            on_wru_nr(g_cmd_recv_handle, (uint8_t[]){0,0,3,0}, 4);
            *ol = (tlen > 256) ? 256 : tlen; memcpy(out, tmp, *ol); return true;
        }
        if (buf[2] == 0x00 && blen >= 6) {
            uint16_t cnt = buf[4] + (buf[5] << 8);
            on_wru_nr(g_cmd_recv_handle, (uint8_t[]){0,0,1,1}, 4);
            uint8_t tmp[512]; size_t t = 0;
            for (uint16_t i = 0; i < cnt && t < 508; i++) {
                if (!do_wait_queue(g_q_cmd_recv, buf, &blen, 3000)) break;
                if (buf[2] == 0x02) { size_t cp = (blen - 2 < 508 - t) ? (blen - 2) : (508 - t); memcpy(tmp + t, buf + 2, cp); t += cp; }
            }
            on_wru_nr(g_cmd_recv_handle, (uint8_t[]){0,0,1,0}, 4);
            if (t > 0) { size_t pl; if (decrypt_response(&g_keys, tmp, t, tmp, &pl)) { *ol = (pl > 256) ? 256 : pl; memcpy(out, tmp, *ol); return true; } }
        }
    }
    return false;
}

bool do_miot_get(uint8_t piid, uint32_t* value) {
    do_drain_cmd_recv_pushes();
    uint8_t buf[16] = {0};
    buf[0] = 12; buf[1] = 0x20; buf[2] = g_seq; buf[3] = 0;
    buf[4] = 0x02; buf[5] = 0x01; buf[6] = SIID_CHARGER;
    buf[7] = piid; buf[8] = 0; buf[9] = 0x01; buf[10] = 0x10; buf[11] = 0;
    g_seq = (g_seq + 1) & 0xFF;
    if (!do_send_enc_sync(buf, 12)) return false;
    uint8_t resp[256]; size_t rl = 0;
    if (!do_recv_cmd(resp, &rl, 8000)) return false;
    if (rl < 14 || resp[4] != 0x03 || resp[7] != piid) return false;
    uint8_t vlen = resp[11];
    if (vlen >= 4 && rl >= 17) *value = resp[13] | (resp[14] << 8) | (resp[15] << 16) | (resp[16] << 24);
    else if (vlen >= 1 && rl >= 14) *value = resp[13];
    else *value = 0;
    return true;
}

bool do_miot_set(uint8_t piid, uint32_t value) {
    do_drain_cmd_recv_pushes();
    uint8_t buf[16] = {0};
    uint8_t byte_len, tl_lo, tl_hi;
    if (value <= 0xFF) { byte_len = 1; tl_lo = 0x01; tl_hi = 0x10; }
    else { byte_len = 4; tl_lo = 0x04; tl_hi = 0x50; }
    int total_len = 11 + byte_len;
    buf[0] = total_len; buf[1] = 0x20; buf[2] = g_seq; buf[3] = 0;
    buf[4] = 0x00; buf[5] = 0x01; buf[6] = SIID_CHARGER;
    buf[7] = piid; buf[8] = 0; buf[9] = tl_lo; buf[10] = tl_hi;
    buf[11] = value & 0xFF;
    if (byte_len >= 4) { buf[12] = (value >> 8) & 0xFF; buf[13] = (value >> 16) & 0xFF; buf[14] = (value >> 24) & 0xFF; }
    g_seq = (g_seq + 1) & 0xFF;
    if (!do_send_enc_sync(buf, total_len)) return false;
    uint8_t resp[256]; size_t rl = 0;
    if (!do_recv_cmd(resp, &rl, 3000)) return false;
    bool ok = false;
    if (rl >= 6 && resp[4] == 0x01) {
        ok = true;
        if (do_recv_cmd(resp, &rl, 2000)) { if (rl >= 8 && resp[4] == 0x04) ok = (resp[7] == piid); }
    } else if (rl >= 8 && resp[4] == 0x04) { ok = (resp[7] == piid); }
    NotifItem item;
    QueueHandle_t qs[] = {g_q_auth_ctrl, g_q_auth_data, g_q_cmd_send};
    for (int i = 0; i < 3; i++) while (xQueueReceive(qs[i], &item, 0) == pdTRUE) {}
    return ok;
}

/* ============================================================
 * BLE lifecycle: scan, discover, disconnect, reconnect, keepalive
 * ============================================================ */
static void start_scanning() {
  xSemaphoreTake(g_connected_sem, 0);
  ble_gap_disc_cancel();
  vTaskDelay(pdMS_TO_TICKS(100));
  struct ble_gap_disc_params dp = {}; dp.itvl = 0x60; dp.window = 0x60;
  ESP_LOGI(TAG, "Scanning for %s...", g_mac_str);
  int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, 10 * 100, &dp, on_gap_event, NULL);
  if (rc != 0) { ESP_LOGE(TAG, "ble_gap_disc failed: %d", rc); vTaskDelay(pdMS_TO_TICKS(1000)); }
  else do_set_state(BLE_CONNECTING);
}

static void start_discover(void) {
  ESP_LOGV(TAG, "Discovering...");

  g_disc_service_start = 0; g_disc_service_end = 0;
  xSemaphoreTake(g_disc_sem, 0);

  int rc = ble_gattc_disc_all_svcs(g_conn_handle, do_disc_svc_cb, NULL);
  App.feed_wdt();
  if (xSemaphoreTake(g_disc_sem, pdMS_TO_TICKS(3600)) != pdTRUE || g_disc_service_start == 0) {
    g_ra_ts = esp_timer_get_time() / 1000;
    ESP_LOGE(TAG, "Service discovery failed"); do_set_state(BLE_RECONNECT); return;
  }

  g_char_ctx[0] = (CharCtx){CHAR_UUID_AUTH_CTRL, &g_auth_ctrl_handle, "auth_ctrl"};
  g_char_ctx[1] = (CharCtx){CHAR_UUID_AUTH_DATA, &g_auth_data_handle, "auth_data"};
  g_char_ctx[2] = (CharCtx){CHAR_UUID_CMD_SEND,  &g_cmd_send_handle,  "cmd_send"};
  g_char_ctx[3] = (CharCtx){CHAR_UUID_CMD_RECV,  &g_cmd_recv_handle,  "cmd_recv"};
  g_char_ctx[4] = (CharCtx){CHAR_UUID_VERSION_RD,&g_ver_read_handle,  "ver_read"};
  g_char_ctx_n = 5;
  xSemaphoreTake(g_disc_sem, 0);
  ble_gattc_disc_all_chrs(g_conn_handle, g_disc_service_start, g_disc_service_end, do_disc_chr_cb, NULL);
  if (xSemaphoreTake(g_disc_sem, pdMS_TO_TICKS(3600)) != pdTRUE) ESP_LOGW(TAG, "Char disc timeout");
  ESP_LOGV(TAG, "handles: ctrl=0x%04X data=0x%04X send=0x%04X recv=0x%04X",
            g_auth_ctrl_handle, g_auth_data_handle, g_cmd_send_handle, g_cmd_recv_handle);
  if (g_auth_ctrl_handle) { uint8_t v[] = {0x01,0x00}; on_wru(g_auth_ctrl_handle+1, v, 2); vTaskDelay(pdMS_TO_TICKS(100)); }
  if (g_auth_data_handle) { uint8_t v[] = {0x01,0x00}; on_wru(g_auth_data_handle+1, v, 2); vTaskDelay(pdMS_TO_TICKS(100)); }
  if (g_cmd_send_handle)  { uint8_t v[] = {0x01,0x00}; on_wru(g_cmd_send_handle+1,  v, 2); vTaskDelay(pdMS_TO_TICKS(100)); }
  if (g_cmd_recv_handle)  { uint8_t v[] = {0x01,0x00}; on_wru(g_cmd_recv_handle+1,  v, 2); vTaskDelay(pdMS_TO_TICKS(100)); }
  if (g_ver_read_handle)  { uint8_t v[] = {0x01,0x00}; on_wru(g_ver_read_handle+1,  v, 2); vTaskDelay(pdMS_TO_TICKS(100)); }
  vTaskDelay(pdMS_TO_TICKS(500));

  if (g_auth_ctrl_handle && g_auth_data_handle) {
    do_set_state(BLE_AUTHENTICATING);
    if (start_auth() == -1){
      start_disconnect();
      do_set_state(BLE_RECONNECT);
      App.feed_wdt();
      ESP_LOGI(TAG, "Auth failed, disconnect + wait 2s");
      g_ra_ts = esp_timer_get_time() / 1000 - 2000;
    }
  } else {
    g_ra_ts = esp_timer_get_time() / 1000;
    ESP_LOGE(TAG, "Missing auth handles"); do_set_state(BLE_RECONNECT);
  }

  // read version
  on_rdu_rsp(g_ver_read_handle, (uint8_t[]){0x0, 0, 0, 0}, 4, (uint8_t *)g_ver_str);
}

static void start_reconnect(void) {
  uint32_t base = (g_ra == 0) ? 3000 : 5000;
  uint32_t d = base * (1 << (g_ra < 4 ? g_ra : 3));
  uint64_t now = esp_timer_get_time() / 1000;
  if (d > 40000) d = 40000;
  if (g_ra_ts + d > now) {
    g_ra++;
    do_set_state(BLE_SCANNING);
    g_ra_ts = now;
    ESP_LOGI(TAG, "Reconnect in %ums (attempt %d)", (unsigned)d, (int)(g_ra + 1));
  }
}

static void start_keepalive(void) {
  // Write Command (no response) to cmd_recv — most reliable keepalive
  // No device response needed, won't block, won't trigger disconnect
  if (g_connected) {
    on_wru_nr(g_cmd_recv_handle, (uint8_t[]){0,0,0,0}, 4);
  }
  g_last_keepalive = esp_timer_get_time() / 1000;
}

static void start_disconnect(void) {
  ble_gap_disc_cancel();
  /* Non-reentrant guard: prevent overlapping disconnect sequences */
  static bool _disconnecting = false;
  if (_disconnecting) { do_drain_all_queues(); return; }
  if (!g_connected) { do_drain_all_queues(); return; }
  _disconnecting = true;
  do_disable_all_notifications();
  xSemaphoreTake(g_disconnect_sem, 0);
  ble_gap_terminate(g_conn_handle, 0x13);
  if (xSemaphoreTake(g_disconnect_sem, pdMS_TO_TICKS(3000)) != pdTRUE) ESP_LOGW(TAG, "Disconnect timeout");
  g_connected = false; g_conn_handle = 0xFFFF; do_drain_all_queues();
  _disconnecting = false;
}

/* ============================================================
 * GAP + GATT callbacks (NimBLE host task context)
 * ============================================================ */
static int on_gap_event(struct ble_gap_event *event, void *arg) {
  switch (event->type) {
    case BLE_GAP_EVENT_DISC:
        if (memcmp(event->disc.addr.val, g_target_addr, 6) == 0) {
            ESP_LOGI(TAG, "Found target, RSSI=%d", event->disc.rssi);
            ble_gap_disc_cancel();
            do_set_state(BLE_CONNECTING);
            if (ble_gap_connect(BLE_OWN_ADDR_PUBLIC, &event->disc.addr, 30000, NULL, on_gap_event, NULL) != 0) {
                g_ra_ts = esp_timer_get_time() / 1000;
                do_set_state(BLE_RECONNECT);
            }
        }
        break;
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status != 0) {
          ESP_LOGE(TAG, "Connect fail: %d", event->connect.status);
          g_connected = false;
          g_ra_ts = esp_timer_get_time() / 1000;
          do_set_state(BLE_RECONNECT);
          break;
        }
        g_conn_handle = event->connect.conn_handle; g_connected = true;
        ESP_LOGV(TAG, "Connected, handle=%d", g_conn_handle);
        ble_gattc_exchange_mtu(g_conn_handle, NULL, NULL);
        if (g_connected_sem) xSemaphoreGive(g_connected_sem);
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGV(TAG, "Disconnected, reason=%d", event->disconnect.reason);
        g_connected = false; g_conn_handle = 0xFFFF; do_drain_all_queues();
        if (g_disconnect_sem) xSemaphoreGive(g_disconnect_sem);
        do_set_state(BLE_RECONNECT);
        break;
    case BLE_GAP_EVENT_MTU:
        ESP_LOGV(TAG, "MTU: %d", event->mtu.value);
        break;
    case BLE_GAP_EVENT_NOTIFY_RX:
        do_dispatch_notif(event->notify_rx.attr_handle,
                        OS_MBUF_PKTLEN(event->notify_rx.om) > 0 ? event->notify_rx.om->om_data : NULL,
                        OS_MBUF_PKTLEN(event->notify_rx.om));
        break;
    case BLE_GAP_EVENT_DISC_COMPLETE:
        ESP_LOGV(TAG, "Scan complete, reason=%d", event->disc_complete.reason);
        do_set_state(BLE_SCANNING);
        break;
    default:
      ESP_LOGV(TAG, "GAP event type=%d", event->type);
      break;
  }
  return 0;
}

void ble_manager_loop(void) {
  switch (g_state) {
  case BLE_SCANNING:
      start_scanning();
      break;
  case BLE_CONNECTING:
      start_discover();
      break;
  case BLE_READY:
      do_handle_cmd_recv();
      do_handle_cmd_send();
      do_pending_check_timeouts();
      if ((esp_timer_get_time() / 1000) - g_last_keepalive >= KEEPALIVE_INTERVAL_MS) start_keepalive();
      break;
  case BLE_RECONNECT:
      start_reconnect();
      break;
  default:
      break;
  }
}

void do_store_setting(uint8_t piid, uint32_t val) {
  if (piid < 32) {
    LOCK_SETTINGS();
    g_settings[piid] = val; g_settings_valid[piid] = true;
    UNLOCK_SETTINGS();
  }
}

void ble_res_loop(void) {

  if (g_state != BLE_READY)
    return;

  // Process BLE results
  BleResult res;
  while (xQueueReceive(result_queue, &res, 0) == pdTRUE) {
    switch (res.type) {
      case RES_PORT_PUSH: {
          break;
      }
      case RES_GET: {
          break;
      }
      case RES_SET: {
          break;
      }
      case RES_BLE_STATUS: {
          break;
      }
      default: break;
      }
  }
}

void ble_cmd_queue_loop(void) {
  bool did_work = false;
  int drain_cnt = 0;

  if (g_state != BLE_READY)
    return;

  BleCommand cmd;
  BleResult res;
  do {
      cmd.type = CMD_NOP;
      if (urgent_queue && xQueueReceive(urgent_queue, &cmd, 0) == pdTRUE) {
          did_work = true;
      } else if (xQueueReceive(cmd_queue, &cmd, 0) == pdTRUE) {
          did_work = true;
      }
      if (cmd.type != CMD_NOP) {
          drain_cnt++;
          switch (cmd.type) {
            case CMD_GET: {
                uint32_t val = 0;
                bool ok = do_miot_get(cmd.piid, &val);
                if (ok) {
                    if (cmd.piid < 32) { g_settings[cmd.piid] = val; g_settings_valid[cmd.piid] = true; }
                    if (cmd.piid == 16) { g_port_ctrl_val = val; g_port_ctrl_valid = true; }
                    if (cmd.piid == 21) { g_protocol_extend_val = val; do_store_setting(21, val); }
                    g_setings_dirty = true;
                    ESP_LOGV(TAG, "GET piid=%d value=%lu", cmd.piid, (unsigned long)val);
                    g_data_dirty = true;
                } else {
                    ESP_LOGW(TAG, "GET piid=%d FAILED", cmd.piid);
                }
                break;
            }
            case CMD_SET: {
                ESP_LOGV(TAG, "SET piid=%d val=%lu", cmd.piid, (unsigned long)cmd.value);
                bool ok = do_miot_set(cmd.piid, cmd.value);
                if (ok) {
                    if (cmd.piid == 16) { g_port_ctrl_val = cmd.value; g_port_ctrl_valid = true; }
                    else if (cmd.piid < 32) { g_settings[cmd.piid] = cmd.value; g_settings_valid[cmd.piid] = true; }
                    if (cmd.piid == 21) { g_protocol_extend_val = cmd.value; do_store_setting(21, cmd.value); }
                    g_setings_dirty = true;
                    ESP_LOGV(TAG, "SET piid=%d val=%lu OK", cmd.piid, (unsigned long)cmd.value);
                } else {
                    ESP_LOGW(TAG, "SET piid=%d val=%lu FAILED", cmd.piid, (unsigned long)cmd.value);
                }
                res = (BleResult){RES_SET, ok, cmd.piid, cmd.value, 0};
                xQueueSend(result_queue, &res, 0);
                break;
            }
            case CMD_PORT: {
                uint32_t current = g_port_ctrl_val;
                ESP_LOGV(TAG, "CMD_PORT bit=%d %s: current=0x%02lX", cmd.piid, cmd.value ? "ON" : "OFF", (unsigned long)current);
                if (cmd.value) current |= (1 << cmd.piid);
                else current &= ~(1 << cmd.piid);
                g_port_ctrl_val = current;
                g_port_ctrl_valid = true;
                if (do_miot_set(16, current)) {
                    ESP_LOGV(TAG, "CMD_PORT: SET16=0x%02lX sent", (unsigned long)current);
                } else {
                    ESP_LOGW(TAG, "CMD_PORT: SET16=0x%02lX FAILED", (unsigned long)current);
                    if (g_state != BLE_READY) {
                        ESP_LOGW(TAG, "BLE not connected, aborting port control");
                        cmd.type = CMD_NOP;  // exit command drain loop
                        break;
                    }
                }
                res = (BleResult){RES_SET, true, 16, current, 0};
                xQueueSend(result_queue, &res, 0);
                last_set16_time = esp_timer_get_time() / 1000;
                break;
            }
            case CMD_ACTION: {
                ESP_LOGV(TAG, "ACTION aiid=%d val=%lu", cmd.piid, (unsigned long)cmd.value);
                bool ok = do_miot_set(cmd.piid, cmd.value);
                break;
            }
            case CMD_RECONNECT:
            case CMD_DISCONNECT:
                start_disconnect();
                break;
            default: break;
          }
      }
  } while (cmd.type != CMD_NOP && drain_cnt < 8);

  // Only yield if truly idle
  if (!did_work) vTaskDelay(pdMS_TO_TICKS(20));
}

static const char* PROTO_NAMES[] = {"idle","5V","5V","QC","AFC","FCP","SCP","PD","PPS","PPS","UFCS"};
static const int PROTO_NAMES_LEN = sizeof(PROTO_NAMES)/sizeof(PROTO_NAMES[0]);
static const char* get_proto_name(uint8_t code) {
    return (code < PROTO_NAMES_LEN) ? PROTO_NAMES[code] : "?";
}

static bool handle_port_control(const char *port, const char *action) {
  if (g_state != BLE_READY) return false;
  if (!port || !action) return false;
  int bit = -1;
  if (strcmp(port, "c1") == 0) bit = 0;
  else if (strcmp(port, "c2") == 0) bit = 1;
  else if (strcmp(port, "c3") == 0) bit = 2;
  else if (strcmp(port, "a") == 0) bit = 3;
  if (bit < 0) return false;
  bool on = (strcmp(action, "on") == 0);
  BleCommand cmd = {CMD_PORT, (uint8_t)bit, uint32_t(on ? 1 : 0), 0};
  if (xQueueSend(urgent_queue, &cmd, pdMS_TO_TICKS(2000)) != pdTRUE) {
      ESP_LOGW(TAG, "PORT %s %s dropped (queue full)", port, action);
      return false;
  }
  ESP_LOGV(TAG, "PORT %s %s (bit=%d)", port, action, bit);

  //if (on)
  //  g_port_ctrl_val |= 0x1 << bit;
  //else
  //  g_port_ctrl_val &= ~(0x1 << bit);

  return true;
}

static bool handle_setting_set(int piid, int value) {
  if (g_state != BLE_READY) return false;
  if (piid <= 0 || piid >= 32) return false;
  BleCommand cmd = {CMD_SET, (uint8_t)piid, (uint32_t)value, 0};
  if (xQueueSend(urgent_queue, &cmd, pdMS_TO_TICKS(2000)) != pdTRUE) {
      ESP_LOGW(TAG, "SET piid=%d dropped (queue full)", piid);
      return false;
  }
  ESP_LOGV(TAG, "SET piid=%d value=%d", piid, value);
  return true;
}

static bool handle_protocol_toggle(const char *port, const char *protocol, bool on) {
  if (g_state != BLE_READY) return false;
  if (!port || !protocol) return false;
  static const struct { const char *port; const char *proto; int bit; } map[] = {
      {"c1", "pd", 0}, {"c1", "pps", 1}, {"c1", "ufcs", 2},
      {"c2", "pd", 8}, {"c2", "pps", 9}, {"c2", "ufcs", 10},
      {"c3", "ufcs", 16}, {"c3", "scp", 17},
      {"a", "ufcs", 24}, {"a", "scp", 25},
  };
  for (int i = 0; i < sizeof(map)/sizeof(map[0]); i++) {
      if (strcmp(port, map[i].port) == 0 && strcasecmp(protocol, map[i].proto) == 0) {
          uint32_t val = g_protocol_extend_val;
          if (on) val |= (1 << map[i].bit);
          else val &= ~(1 << map[i].bit);
          BleCommand cmd = {CMD_SET, 21, val, 0};
          if (xQueueSend(urgent_queue, &cmd, pdMS_TO_TICKS(2000)) != pdTRUE) {
              ESP_LOGW(TAG, "PROTO %s %s dropped (queue full)", port, protocol);
              return false;
          }
          ESP_LOGV(TAG, "PROTO %s %s %s (PIID21=0x%lX)", port, protocol, on?"ON":"OFF", (unsigned long)val);
          return true;
      }
  }
  return false;
}

// ---- ESPHome Component lifecycle -------------------------------------------
void CuktechBle::setup() {
  g_instance = this;
  g_device_name_cache = this->device_name_;

  ESP_LOGI(TAG, "Initializing Cuktech BLE bridge (v0.1)");

  g_q_auth_ctrl = xQueueCreate(NOTIF_QUEUE_LEN_AUTH, sizeof(NotifItem));
  g_q_auth_data = xQueueCreate(NOTIF_QUEUE_LEN_AUTH, sizeof(NotifItem));
  g_q_cmd_send  = xQueueCreate(NOTIF_QUEUE_LEN, sizeof(NotifItem));
  g_q_cmd_recv  = xQueueCreate(NOTIF_QUEUE_LEN, sizeof(NotifItem));
  g_q_ver_read  = xQueueCreate(NOTIF_QUEUE_LEN, sizeof(NotifItem));
  g_op_sem = xSemaphoreCreateBinary();
  g_connected_sem = xSemaphoreCreateBinary();
  g_disc_sem = xSemaphoreCreateBinary();
  g_disconnect_sem = xSemaphoreCreateBinary();

  // NVS is already initialized by ESPHome before our setup() runs.
  esp_err_t ret = nimble_port_init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "nimble_port_init failed: %d", ret);
    this->mark_failed();
    return;
  }

  ble_hs_cfg.reset_cb = do_stack_reset;
  ble_hs_cfg.sync_cb = do_nimble_on_sync;
  ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
  ble_hs_cfg.sm_bonding = 1;
  ble_hs_cfg.sm_sc = 1;
  ble_hs_cfg.sm_mitm = 0;
  ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
  ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
  ble_hs_cfg.gatts_register_cb = NULL;

  ble_svc_gap_init();
  ble_svc_gatt_init();
  ble_store_config_init();  // NVS-backed bond store

  nimble_port_freertos_init(do_nimble_host_task);

  cmd_queue = xQueueCreate(20, sizeof(BleCommand));
  urgent_queue = xQueueCreate(4, sizeof(BleCommand));
  result_queue = xQueueCreate(48, sizeof(BleResult));
}

void CuktechBle::loop() {
  uint64_t now = esp_timer_get_time() / 1000;

  ble_manager_loop();
  ble_cmd_queue_loop();
  ble_res_loop();

  // Slow poll (90s): scene mode, screen time, countdown, trickle
  if ((g_state == BLE_READY) && (now - this->last_cd_fetch_slow_ >= 90000)) {
      this->last_cd_fetch_slow_ = now;
      static const uint8_t SLOW_PIIDS[] = {5, 6, 9, 10, 11, 12, 13, 15, 19, 20};
      for (int i = 0; i < sizeof(SLOW_PIIDS); i++) {
          BleCommand c = {CMD_GET, SLOW_PIIDS[i], 0, 0};
          xQueueSend(cmd_queue, &c, 0);
      }
  }
  // Fast poll (20s): port control (16) and protocol extends (21) change via SET
  if ((g_state == BLE_READY) && (now - this->last_cd_fetch_fast_ >= 20000)) {
      this->last_cd_fetch_fast_ = now;
      static const uint8_t FAST_PIIDS[] = {16, 21};
      for (int i = 0; i < sizeof(FAST_PIIDS); i++) {
          BleCommand c = {CMD_GET, FAST_PIIDS[i], 0, 0};
          xQueueSend(cmd_queue, &c, 0);
      }
  }

  if (g_data_dirty) {
    g_data_dirty = false;
    this->publish_portdata_();
  }

  if (g_setings_dirty) {
    g_setings_dirty = false;
    this->publish_settings_();
  }
  return;
}

void CuktechBle::publish_settings_(void) {
  if (g_connected) {
    if (this->scene_mode_select_) {
      this->scene_mode_select_->publish_state(g_settings[5] - 1);
    }
    if (this->screen_saver_timeout_select_) {
      this->screen_saver_timeout_select_->publish_state(g_settings[6] - 1);
    }
    if (this->c1_countdown_number_) {
      this->c1_countdown_number_->publish_state(g_settings[9]);
    }
    if (this->c2_countdown_number_) {
      this->c2_countdown_number_->publish_state(g_settings[10]);
    }
    if (this->c3_countdown_number_) {
      this->c3_countdown_number_->publish_state(g_settings[11]);
    }
    if (this->a_countdown_number_) {
      this->a_countdown_number_->publish_state(g_settings[12]);
    }
    if (this->language_select_) {
      this->language_select_->publish_state(g_settings[13]);
    }
    if (this->a_always_on_switch_) {
      this->a_always_on_switch_->publish_state(g_settings[15]);
    }
    if (this->screen_saver_switch_) {
      this->screen_saver_switch_->publish_state(g_settings[19]);
    }
    if (this->screen_dir_lock_switch_) {
      this->screen_dir_lock_switch_->publish_state(g_settings[20]);
    }
    if (this->c1_port_switch_) {
      this->c1_port_switch_->publish_state(g_settings[16] & (0x1 << 0));
    }
    if (this->c2_port_switch_) {
      this->c2_port_switch_->publish_state(g_settings[16] & (0x1 << 1));
    }
    if (this->c3_port_switch_) {
      this->c3_port_switch_->publish_state(g_settings[16] & (0x1 << 2));
    }
    if (this->a_port_switch_) {
      this->a_port_switch_->publish_state(g_settings[16] & (0x1 << 3));
    }
    if (this->c1_pd_switch_) {
      this->c1_pd_switch_->publish_state(g_settings[21] & (0x1 << 0));
    }
    if (this->c1_pps_switch_) {
      this->c1_pps_switch_->publish_state(g_settings[21] & (0x1 << 1));
    }
    if (this->c1_ufcs_switch_) {
      this->c1_ufcs_switch_->publish_state(g_settings[21] & (0x1 << 2));
    }
    if (this->c2_pd_switch_) {
      this->c2_pd_switch_->publish_state(g_settings[21] & (0x1 << 8));
    }
    if (this->c2_pps_switch_) {
      this->c2_pps_switch_->publish_state(g_settings[21] & (0x1 << 9));
    }
    if (this->c2_ufcs_switch_) {
      this->c2_ufcs_switch_->publish_state(g_settings[21] & (0x1 << 10));
    }
    if (this->c3_ufcs_switch_) {
      this->c3_ufcs_switch_->publish_state(g_settings[21] & (0x1 << 16));
    }
    if (this->c3_scp_switch_) {
      this->c3_scp_switch_->publish_state(g_settings[21] & (0x1 << 17));
    }
    if (this->a_ufcs_switch_) {
      this->a_ufcs_switch_->publish_state(g_settings[21] & (0x1 << 24));
    }
    if (this->a_scp_switch_) {
      this->a_scp_switch_->publish_state(g_settings[21] & (0x1 << 25));
    }
  }
}

void CuktechBle::publish_portdata_() {
  if (this->connected_binary_sensor_) {
    this->connected_binary_sensor_->publish_state(g_connected);
  }
  if (g_connected) {
    if (this->c1_active_binary_sensor_) {
      this->c1_active_binary_sensor_->publish_state(g_ports[0].status);
    }
    if (this->c2_active_binary_sensor_) {
      this->c2_active_binary_sensor_->publish_state(g_ports[1].status);
    }
    if (this->c3_active_binary_sensor_) {
      this->c3_active_binary_sensor_->publish_state(g_ports[2].status);
    }
    if (this->a_active_binary_sensor_) {
      this->a_active_binary_sensor_->publish_state(g_ports[3].status);
    }
    if (this->total_power_sensor_) {
      this->total_power_sensor_->publish_state(
        g_ports[0].power + g_ports[1].power + g_ports[2].power + g_ports[3].power);
    }
    if (this->c1_power_sensor_) {
      this->c1_power_sensor_->publish_state(g_ports[0].power);
    }
    if (this->c2_power_sensor_) {
      this->c2_power_sensor_->publish_state(g_ports[1].power);
    }
    if (this->c3_power_sensor_) {
      this->c3_power_sensor_->publish_state(g_ports[2].power);
    }
    if (this->a_power_sensor_) {
      this->a_power_sensor_->publish_state(g_ports[3].power);
    }
    if (this->c1_current_sensor_) {
      this->c1_current_sensor_->publish_state(g_ports[0].current);
    }
    if (this->c2_current_sensor_) {
      this->c2_current_sensor_->publish_state(g_ports[1].current);
    }
    if (this->c3_current_sensor_) {
      this->c3_current_sensor_->publish_state(g_ports[2].current);
    }
    if (this->a_current_sensor_) {
      this->a_current_sensor_->publish_state(g_ports[3].current);
    }
    if (this->c1_voltage_sensor_) {
      this->c1_voltage_sensor_->publish_state(g_ports[0].voltage);
    }
    if (this->c2_voltage_sensor_) {
      this->c2_voltage_sensor_->publish_state(g_ports[1].voltage);
    }
    if (this->c3_voltage_sensor_) {
      this->c3_voltage_sensor_->publish_state(g_ports[2].voltage);
    }
    if (this->a_voltage_sensor_) {
      this->a_voltage_sensor_->publish_state(g_ports[3].voltage);
    }
    if (this->version_text_sensor_) {
      this->version_text_sensor_->publish_state(g_ver_str);
    }
  }

  if (this->c1_protocol_text_sensor_) {
    const char *proto = g_ports[0].active ? get_proto_name(g_ports[0].protocol) : "idle";
    this->c1_protocol_text_sensor_->publish_state(proto);
  }
  if (this->c2_protocol_text_sensor_) {
    const char *proto = g_ports[1].active ? get_proto_name(g_ports[1].protocol) : "idle";
    this->c2_protocol_text_sensor_->publish_state(proto);
  }
  if (this->c3_protocol_text_sensor_) {
    const char *proto = g_ports[2].active ? get_proto_name(g_ports[2].protocol) : "idle";
    this->c3_protocol_text_sensor_->publish_state(proto);
  }
  if (this->a_protocol_text_sensor_) {
    const char *proto = g_ports[3].active ? get_proto_name(g_ports[3].protocol) : "idle";
    this->a_protocol_text_sensor_->publish_state(proto);
  }

  //for (int i=0; i< 32; i++) {
  //  ESP_LOGE(TAG, "g_settings %d %x", i, g_settings[i]);
  //}
}

void CuktechBle::dump_config() {
  ESP_LOGCONFIG(TAG, "Cuktech BLE Bridge:");
  ESP_LOGCONFIG(TAG, "  Device name: %s", this->device_name_.c_str());
  ESP_LOGCONFIG(TAG, "  BLE Mac: %s", this->ble_mac_.c_str());
  ESP_LOGCONFIG(TAG, "  BLE Token: %s", this->ble_token_.c_str());
  ESP_LOGCONFIG(TAG, "  BLE Key: %s", this->ble_key_.c_str());
  this->check_config();
}

float CuktechBle::get_setup_priority() const {
  return setup_priority::AFTER_WIFI;
}

void CuktechBle::on_shutdown() {
  start_disconnect();
}

void CuktechBle::on_connect_state_change(bool connected) {
  this->pending_connected_state_ = connected;
  this->connection_dirty_ = true;
  if (!connected) {
    // Reset "present" flags so stale values don't get re-published on reconnect.
    this->latest_ = PortData{};
  }
}

void CuktechBle::clear_bonding() {
  ESP_LOGW(TAG, "Clearing all bonded peers from NVS");
  ble_store_clear();
}

void CuktechBle::check_config() {
  this->ble_config_ok_ = true;
  /* Validate BLE config before initializing NimBLE. If MAC/token/key
    are missing or ill-formed, skip BLE entirely — no scanning, no
    repeated connection attempts. */
  uint8_t mac[6];
  bool mac_ok = (this->ble_mac_.c_str() && strlen(this->ble_mac_.c_str()) == 17 &&
                sscanf(this->ble_mac_.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                        &mac[5], &mac[4], &mac[3], &mac[2], &mac[1], &mac[0]) == 6);
  /* Token must be exactly 24 hex chars → 12 bytes.  We also accept
    bare 12-byte raw token strings (rare config edge-case). */
  size_t tlen = this->ble_token_.c_str()? strlen(this->ble_token_.c_str()) : 0;
  bool token_ok = (tlen >= 24);

  if (!mac_ok || !token_ok) {
      ESP_LOGW(TAG, "BLE config incomplete (mac=%s token=%s len=%u) — BLE disabled",
              this->ble_mac_.c_str() ? "set" : "null",
              this->ble_token_.c_str() ? (tlen > 8 ? "set" : "short") : "null",
              (unsigned)tlen);
      g_enabled = false;
      this->ble_config_ok_ = false;
  }

  memcpy(g_target_addr, mac, 6);

  snprintf(g_mac_str, sizeof(g_mac_str), "%s", this->ble_mac_.c_str());
  /* Parse 12-byte token from hex string. Loop guard ensures we never
      read past the NUL terminator (already validated >= 24 chars). */
  for (int i = 0; i < 12; i++) {
      char hex[3] = {this->ble_token_.c_str()[i*2], this->ble_token_.c_str()[i*2+1], 0};
      g_token[i] = (uint8_t)strtol(hex, NULL, 16);
  }
}

void CuktechBle::set_enable_controlling(bool enable) {
  this->check_config();
  if (!this->ble_config_ok_) {
      ESP_LOGE(TAG, "BLE config is not ok!");
    return;
  }
  g_enabled = enable;

  ESP_LOGV(TAG, "Controlling switch -> %s", enable ? "ON" : "OFF");
  if (g_nimble_ready && !this->last_published_connected_) {
    ble_gap_adv_stop();
    if (enable) {
        if (g_state == BLE_IDLE) do_set_state(BLE_SCANNING);
    } else {
        if (g_connected) start_disconnect();
        if (g_state != BLE_IDLE) do_set_state(BLE_IDLE);
        //memset(g_ports, 0x0, sizeof(g_ports));
        this->publish_portdata_();
    }
  }
}

bool CuktechBle::controlling_enabled() { return g_enabled; }

void CuktechBle::factory_reset(void) {
  uint8_t piid = 1;
  BleCommand cmd = {CMD_ACTION, piid, 0, 0};
  if (xQueueSend(urgent_queue, &cmd, pdMS_TO_TICKS(2000)) != pdTRUE) {
      ESP_LOGW(TAG, "ACTION piid=%d dropped (queue full)", piid);
  }
}

void CuktechBle::set_screen_dir_lock(bool enable) {
  handle_setting_set(20, enable? 1: 0);
}

void CuktechBle::set_screen_saver(bool enable) {
  handle_setting_set(19, enable? 1: 0);
}

void CuktechBle::set_scene_mode(const char *state) {
  int value = 0;
  if (!strcmp(state, "AI Mode"))
    value = 1;
  else if (!strcmp(state, "Digital Mode"))
    value = 2;
  else if (!strcmp(state, "Single Port Mode"))
    value = 3;
  else if (!strcmp(state, "Balance Mode"))
    value = 4;
  handle_setting_set(5, (uint16_t)value);
}

void CuktechBle::set_language(const char *state) {
  int value = 0;
  if (!strcmp(state, "Simplified Chinese"))
    value = 1;
  else if (!strcmp(state, "English"))
    value = 0;
  handle_setting_set(13, (uint16_t)value);
}

void CuktechBle::set_c1_port(bool enable) {
  handle_port_control("c1", enable? "on": "off");
}

void CuktechBle::set_c2_port(bool enable) {
  handle_port_control("c2", enable? "on": "off");
}

void CuktechBle::set_c3_port(bool enable) {
  handle_port_control("c3", enable? "on": "off");
}

void CuktechBle::set_a_port(bool enable) {
  handle_port_control("a", enable? "on": "off");
}

void CuktechBle::set_c1_pd(bool enable) {
  handle_protocol_toggle("c1", "pd", enable);
}

void CuktechBle::set_c1_pps(bool enable) {
  handle_protocol_toggle("c1", "pps", enable);
}

void CuktechBle::set_c1_ufcs(bool enable) {
  handle_protocol_toggle("c1", "ufcs", enable);
}

void CuktechBle::set_c2_pd(bool enable) {
  handle_protocol_toggle("c2", "pd", enable);
}

void CuktechBle::set_c2_pps(bool enable) {
  handle_protocol_toggle("c2", "pps", enable);
}

void CuktechBle::set_c2_ufcs(bool enable) {
  handle_protocol_toggle("c2", "ufcs", enable);
}

void CuktechBle::set_c3_scp(bool enable) {
  handle_protocol_toggle("c3", "scp", enable);
}

void CuktechBle::set_c3_ufcs(bool enable) {
  handle_protocol_toggle("c3", "ufcs", enable);
}

void CuktechBle::set_a_scp(bool enable) {
  handle_protocol_toggle("a", "scp", enable);
}

void CuktechBle::set_a_ufcs(bool enable) {
  handle_protocol_toggle("a", "ufcs", enable);
}

void CuktechBle::set_a_always_on(bool enable) {
  handle_setting_set(15, enable? 1: 0);
}

void CuktechBle::set_c1_countdown(float value) {
  handle_setting_set(9, (uint16_t)value);
}

void CuktechBle::set_c2_countdown(float value) {
  handle_setting_set(10, (uint16_t)value);
}

void CuktechBle::set_c3_countdown(float value) {
  handle_setting_set(11, (uint16_t)value);
}

void CuktechBle::set_a_countdown(float value) {
  handle_setting_set(12, (uint16_t)value);
}

void CuktechBle::set_screen_saver_timeout(const char *state) {
  int value = 0;
  if (!strcmp(state, "1 Min"))
    value = 4;
  else if (!strcmp(state, "OFF"))
    value = 3;
  else if (!strcmp(state, "30 Min"))
    value = 2;
  else if (!strcmp(state, "10 Min"))
    value = 1;
  else if (!strcmp(state, "5 Min"))
    value = 0;
  handle_setting_set(6, (uint16_t)value + 1);
}

}  // namespace cuktech_ble
}  // namespace esphome

#endif  // USE_ESP32
