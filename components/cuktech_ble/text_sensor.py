"""Text Sensors for Cuktech BLE bridge."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import (
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import CONF_CUKTECH_BLE_ID, CuktechBle

CONF_C1_PROTOCOL = "c1_protocol"
CONF_C2_PROTOCOL = "c2_protocol"
CONF_C3_PROTOCOL = "c3_protocol"
CONF_A_PROTOCOL = "a_protocol"

ICON_USB_C_PORT = "mdi:usb-c-port"
ICON_USB_PORT = "mdi:usb-port"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_CUKTECH_BLE_ID): cv.use_id(CuktechBle),
        cv.Optional(CONF_C1_PROTOCOL): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon=ICON_USB_C_PORT
        ),
        cv.Optional(CONF_C2_PROTOCOL): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon=ICON_USB_C_PORT
        ),
        cv.Optional(CONF_C3_PROTOCOL): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon=ICON_USB_C_PORT
        ),
        cv.Optional(CONF_A_PROTOCOL): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon=ICON_USB_PORT
        ),
    }
)

_SENSORS = {
    CONF_C1_PROTOCOL: "set_c1_protocol_text_sensor",
    CONF_C2_PROTOCOL: "set_c2_protocol_text_sensor",
    CONF_C3_PROTOCOL: "set_c3_protocol_text_sensor",
    CONF_A_PROTOCOL: "set_a_protocol_text_sensor",
}

async def to_code(config):
    parent = await cg.get_variable(config[CONF_CUKTECH_BLE_ID])
    for key, setter in _SENSORS.items():
        if key in config:
            sens = await text_sensor.new_text_sensor(config[key])
            cg.add(getattr(parent, setter)(sens))
