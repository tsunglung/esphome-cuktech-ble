"""Cuktech 10 Live Data Interface (LDI) BLE Bridge - ESPHome External Component."""
from esphome.core import CORE
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components.esp32 import add_idf_sdkconfig_option
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
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_device_name(config[CONF_DEVICE_NAME]))
    if cv.Version.parse(ESPHOME_VERSION) >= cv.Version.parse("2026.9.0"):
        request_bluetooth()
