import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import (
    CONF_FACTORY_RESET,
    CONF_ID,
    DEVICE_CLASS_RESTART,
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_RESTART_ALERT,
)
from esphome.types import ConfigType

from .. import CONF_CUKTECH_BLE_ID, CuktechBle, cuktech_ble_ns

FactoryResetButton = cuktech_ble_ns.class_("FactoryResetButton", button.Button)

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_CUKTECH_BLE_ID): cv.use_id(CuktechBle),
    cv.Optional(CONF_FACTORY_RESET): button.button_schema(
        FactoryResetButton,
        device_class=DEVICE_CLASS_RESTART,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_RESTART_ALERT,
    ),
}


async def to_code(config: ConfigType) -> None:
    CuktechBle = await cg.get_variable(config[CONF_CUKTECH_BLE_ID])
    if factory_reset_config := config.get(CONF_FACTORY_RESET):
        b = await button.new_button(factory_reset_config)
        await cg.register_parented(b, config[CONF_CUKTECH_BLE_ID])
        cg.add(CuktechBle.set_factory_reset_button(b))
