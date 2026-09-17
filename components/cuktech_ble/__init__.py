"""Cuktech 10 Live Data Interface (LDI) BLE Bridge - ESPHome External Component."""
from esphome.core import CORE
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components.const import CONF_USE_PSRAM
from esphome.components.esp32 import (
    add_idf_sdkconfig_option,
    const,
    get_esp32_variant
)
from esphome.const import CONF_ID
from esphome.const import __version__ as ESPHOME_VERSION

if cv.Version.parse(ESPHOME_VERSION) >= cv.Version.parse("2026.9.0"):
  from esphome.components.esp32 import request_bluetooth


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
        cv.Optional(CONF_USE_PSRAM): cv.All(
            cv.only_on_esp32, cv.requires_component("psram"), cv.boolean
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_device_name(config[CONF_DEVICE_NAME]))
    if cv.Version.parse(ESPHOME_VERSION) >= cv.Version.parse("2026.9.0"):
        request_bluetooth()

    if config.get(CONF_USE_PSRAM, False):
        cg.add_define("USE_ESP32_BLE_PSRAM")
        # CONFIG_BT_ALLOCATION_FROM_SPIRAM_FIRST is only available on ESP32
        # (BTDM dual-mode controller). BLE-only SoCs (C3, S3, C2, H2) do not
        # expose this Kconfig symbol; applying it there would cause a build error.
        if get_esp32_variant() == const.VARIANT_ESP32:
            add_idf_sdkconfig_option("CONFIG_BT_ALLOCATION_FROM_SPIRAM_FIRST", True)
        # CONFIG_BT_BLE_DYNAMIC_ENV_MEMORY applies to all Bluedroid-enabled variants.
        add_idf_sdkconfig_option("CONFIG_BT_BLE_DYNAMIC_ENV_MEMORY", True)