"""Binary sensors for Cuktech BLE bridge."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    DEVICE_CLASS_CONNECTIVITY,
    DEVICE_CLASS_RUNNING,
    ENTITY_CATEGORY_DIAGNOSTIC
)

from . import CONF_CUKTECH_BLE_ID, CuktechBle

CONF_CONNECTED = "connected"
CONF_C1_ACTIVE = "c1_active"
CONF_C2_ACTIVE = "c2_active"
CONF_C3_ACTIVE = "c3_active"
CONF_A_ACTIVE = "a_active"


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_CUKTECH_BLE_ID): cv.use_id(CuktechBle),
        cv.Optional(CONF_CONNECTED): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_CONNECTIVITY,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_C1_ACTIVE): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_RUNNING,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_C2_ACTIVE): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_RUNNING,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_C3_ACTIVE): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_RUNNING,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_A_ACTIVE): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_RUNNING,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)

_SENSORS = {
    CONF_CONNECTED: "set_connected_binary_sensor",
    CONF_C1_ACTIVE: "set_c1_active_binary_sensor",
    CONF_C2_ACTIVE: "set_c2_active_binary_sensor",
    CONF_C3_ACTIVE: "set_c3_active_binary_sensor",
    CONF_A_ACTIVE: "set_a_active_binary_sensor",
}


async def to_code(config):
    parent = await cg.get_variable(config[CONF_CUKTECH_BLE_ID])
    for key, setter in _SENSORS.items():
        if key in config:
            sens = await binary_sensor.new_binary_sensor(config[key])
            cg.add(getattr(parent, setter)(sens))
