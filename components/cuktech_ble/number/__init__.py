import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_MAX_VALUE,
    CONF_MIN_VALUE,
    ENTITY_CATEGORY_CONFIG,
    UNIT_MINUTE
)
from esphome.types import ConfigType

from .. import CONF_CUKTECH_BLE_ID, CuktechBle, cuktech_ble_ns

C1CountdownNumber = cuktech_ble_ns.class_("C1CountdownNumber", number.Number)
C2CountdownNumber = cuktech_ble_ns.class_("C2CountdownNumber", number.Number)
C3CountdownNumber = cuktech_ble_ns.class_("C3CountdownNumber", number.Number)
ACountdownNumber = cuktech_ble_ns.class_("ACountdownNumber", number.Number)

CONF_C1_COUNTDOWN = "c1_countdown"
CONF_C2_COUNTDOWN = "c2_countdown"
CONF_C3_COUNTDOWN = "c3_countdown"
CONF_A_COUNTDOWN = "a_countdown"

ICON_TIMER = "mdi:timer-cog-outline"

MAX_VALUE = 1440
MIN_VALUE = 0

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_CUKTECH_BLE_ID): cv.use_id(CuktechBle),
    cv.Optional(CONF_C1_COUNTDOWN): number.number_schema(
        C1CountdownNumber,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_TIMER,
        unit_of_measurement=UNIT_MINUTE
    ),
    cv.Optional(CONF_C2_COUNTDOWN): number.number_schema(
        C2CountdownNumber,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_TIMER,
        unit_of_measurement=UNIT_MINUTE
    ),
    cv.Optional(CONF_C3_COUNTDOWN): number.number_schema(
        C3CountdownNumber,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_TIMER,
        unit_of_measurement=UNIT_MINUTE
    ),
    cv.Optional(CONF_A_COUNTDOWN): number.number_schema(
        ACountdownNumber,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_TIMER,
        unit_of_measurement=UNIT_MINUTE
    ),
}

async def to_code(config: ConfigType) -> None:
    CuktechBle = await cg.get_variable(config[CONF_CUKTECH_BLE_ID])
    if c1_countdown_config := config.get(CONF_C1_COUNTDOWN):
        s = await number.new_number(
            c1_countdown_config,
            max_value=MAX_VALUE,
            min_value=MIN_VALUE,
            step=1
        )
        await cg.register_parented(s, config[CONF_CUKTECH_BLE_ID])
        cg.add(CuktechBle.set_c1_countdown_number(s))
    if c2_countdown_config := config.get(CONF_C2_COUNTDOWN):
        s = await number.new_number(
            c2_countdown_config,
            max_value=MAX_VALUE,
            min_value=MIN_VALUE,
            step=1
        )
        await cg.register_parented(s, config[CONF_CUKTECH_BLE_ID])
        cg.add(CuktechBle.set_c2_countdown_number(s))

    if c3_countdown_config := config.get(CONF_C3_COUNTDOWN):
        s = await number.new_number(
            c3_countdown_config,
            max_value=MAX_VALUE,
            min_value=MIN_VALUE,
            step=1
        )
        await cg.register_parented(s, config[CONF_CUKTECH_BLE_ID])
        cg.add(CuktechBle.set_c3_countdown_number(s))

    if a_countdown_config := config.get(CONF_A_COUNTDOWN):
        s = await number.new_number(
            a_countdown_config,
            max_value=MAX_VALUE,
            min_value=MIN_VALUE,
            step=1
        )
        await cg.register_parented(s, config[CONF_CUKTECH_BLE_ID])
        cg.add(CuktechBle.set_a_countdown_number(s))

