"""Sensors for Cuktech BLE bridge."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.components.const import ICON_CURRENT_DC
from esphome.const import (
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_VOLTAGE,
    ICON_FLASH,
    STATE_CLASS_MEASUREMENT,
    UNIT_AMPERE,
    UNIT_VOLT,
    UNIT_WATT,
)

from . import CONF_CUKTECH_BLE_ID, CuktechBle

CONF_TOTAL_POWER = "total_power"
CONF_C1_POWER = "c1_power"
CONF_C2_POWER = "c2_power"
CONF_C3_POWER = "c3_power"
CONF_A_POWER = "a_power"
CONF_C1_CURRENT = "c1_current"
CONF_C2_CURRENT = "c2_current"
CONF_C3_CURRENT = "c3_current"
CONF_A_CURRENT = "a_current"
CONF_C1_VOLTAGE = "c1_voltage"
CONF_C2_VOLTAGE = "c2_voltage"
CONF_C3_VOLTAGE = "c3_voltage"
CONF_A_VOLTAGE = "a_voltage"
CONF_C1_PROTOCOL = "c1_protocol"
CONF_C2_PROTOCOL = "c2_protocol"
CONF_C3_PROTOCOL = "c3_protocol"
CONF_A_PROTOCOL = "a_protocol"

ICON_USB_C_PORT = "mdi:usb-c-port"
ICON_USB_PORT = "mdi:usb-port"
ICON_VOLTAGE = "mdi:sine-wave"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_CUKTECH_BLE_ID): cv.use_id(CuktechBle),
        cv.Optional(CONF_TOTAL_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_FLASH
        ),
        cv.Optional(CONF_C1_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_FLASH
        ),
        cv.Optional(CONF_C2_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_FLASH
        ),
        cv.Optional(CONF_C3_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_FLASH
        ),
        cv.Optional(CONF_A_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_FLASH
        ),
        cv.Optional(CONF_C1_CURRENT): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_CURRENT_DC
        ),
        cv.Optional(CONF_C2_CURRENT): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_CURRENT_DC
        ),
        cv.Optional(CONF_C3_CURRENT): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_CURRENT_DC
        ),
        cv.Optional(CONF_A_CURRENT): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_CURRENT_DC
        ),
        cv.Optional(CONF_C1_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_VOLTAGE
        ),
        cv.Optional(CONF_C2_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_VOLTAGE
        ),
        cv.Optional(CONF_C3_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_VOLTAGE
        ),
        cv.Optional(CONF_A_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_VOLTAGE
        ),
        cv.Optional(CONF_C1_PROTOCOL): sensor.sensor_schema(
            icon=ICON_USB_C_PORT
        ),
        cv.Optional(CONF_C2_PROTOCOL): sensor.sensor_schema(
            icon=ICON_USB_C_PORT
        ),
        cv.Optional(CONF_C3_PROTOCOL): sensor.sensor_schema(
            icon=ICON_USB_C_PORT
        ),
        cv.Optional(CONF_A_PROTOCOL): sensor.sensor_schema(
            icon=ICON_USB_PORT
        ),
    }
)

_SENSORS = {
    CONF_TOTAL_POWER: "set_total_power_sensor",
    CONF_C1_POWER: "set_c1_power_sensor",
    CONF_C2_POWER: "set_c2_power_sensor",
    CONF_C3_POWER: "set_c3_power_sensor",
    CONF_A_POWER: "set_a_power_sensor",
    CONF_C1_CURRENT: "set_c1_current_sensor",
    CONF_C2_CURRENT: "set_c2_current_sensor",
    CONF_C3_CURRENT: "set_c3_current_sensor",
    CONF_A_CURRENT: "set_a_current_sensor",
    CONF_C1_VOLTAGE: "set_c1_voltage_sensor",
    CONF_C2_VOLTAGE: "set_c2_voltage_sensor",
    CONF_C3_VOLTAGE: "set_c3_voltage_sensor",
    CONF_A_VOLTAGE: "set_a_voltage_sensor",
    CONF_C1_PROTOCOL: "set_c1_protocol_sensor",
    CONF_C2_PROTOCOL: "set_c2_protocol_sensor",
    CONF_C3_PROTOCOL: "set_c3_protocol_sensor",
    CONF_A_PROTOCOL: "set_a_protocol_sensor",
}

async def to_code(config):
    parent = await cg.get_variable(config[CONF_CUKTECH_BLE_ID])
    for key, setter in _SENSORS.items():
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(parent, setter)(sens))
