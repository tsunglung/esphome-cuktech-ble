"""Cuktech 10 Live Data Interface (LDI) BLE Bridge - ESPHome External Component."""
from esphome.core import CORE
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components.esp32 import add_idf_sdkconfig_option
from esphome.const import CONF_ID

CODEOWNERS = ["@zonglong"]
DEPENDENCIES = ["esp32"]
AUTO_LOAD = ["binary_sensor", "button", "number", "select", "sensor", "switch", "text_sensor"]
CONFLICTS_WITH = ["bluetooth_proxy"]
MULTI_CONF = False

cuktech_ble_ns = cg.esphome_ns.namespace("cuktech_ble")
CuktechBle = cuktech_ble_ns.class_("CuktechBle", cg.Component)

CONF_CUKTECH_BLE_ID = "cuktech_ble_id"

CONF_DEVICE_NAME = "device_name"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(CuktechBle),
        cv.Optional(CONF_DEVICE_NAME, default="HA Cuktech BLE Bridge"): cv.string,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_device_name(config[CONF_DEVICE_NAME]))

    # ESP-IDF only - NimBLE is the host stack we depend on
    cg.add_build_flag("-DCUKTECH_BLE_LDI_NIMBLE")

    # NimBLE sdkconfig requirements (issue #59): declare these ourselves so
    # the bridge builds and runs correctly out of the box, instead of users
    # having to discover and copy this exact block into their own YAML by
    # trial and error - core components like esp32_ble_tracker do the same.
    # Matches the block every example/factory YAML in this repo already
    # sets manually. The three CONFIG_BTDM_CTRL_MODE_* options are
    # deliberately left out here: they only exist on dual-mode (BT Classic +
    # BLE) chips like the classic ESP32, not on BLE-only ones (C3/C6/S3),
    # so those stay in each YAML's own esp32: block instead.
    if CORE.is_esp32:
      add_idf_sdkconfig_option("CONFIG_BT_ENABLED", True)
      add_idf_sdkconfig_option("CONFIG_BT_NIMBLE_ENABLED", True)
      add_idf_sdkconfig_option("CONFIG_BT_BLUEDROID_ENABLED", False)
      add_idf_sdkconfig_option("CONFIG_BT_CONTROLLER_ENABLED", True)
      add_idf_sdkconfig_option("CONFIG_BT_NIMBLE_MAX_CONNECTIONS", 1)
      add_idf_sdkconfig_option("CONFIG_BT_NIMBLE_ROLE_PERIPHERAL", True)
      add_idf_sdkconfig_option("CONFIG_BT_NIMBLE_ROLE_BROADCASTER", True)
      add_idf_sdkconfig_option("CONFIG_BT_NIMBLE_ROLE_CENTRAL", True)
      add_idf_sdkconfig_option("CONFIG_BT_NIMBLE_ROLE_OBSERVER", True)
      add_idf_sdkconfig_option("CONFIG_BT_NIMBLE_SM_LEGACY", False)
      add_idf_sdkconfig_option("CONFIG_BT_NIMBLE_SM_SC", True)
      add_idf_sdkconfig_option("CONFIG_BT_NIMBLE_NVS_PERSIST", True)
      add_idf_sdkconfig_option("CONFIG_BT_NIMBLE_ATT_PREFERRED_MTU", 247)
      add_idf_sdkconfig_option("CONFIG_ESP_TASK_WDT_TIMEOUT_S", 10)
