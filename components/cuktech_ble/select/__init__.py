import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    ENTITY_CATEGORY_CONFIG,
)
from esphome.types import ConfigType

from .. import CONF_CUKTECH_BLE_ID, CuktechBle, cuktech_ble_ns

SceneModeSelect = cuktech_ble_ns.class_("SceneModeSelect", select.Select)
LanguageSelect = cuktech_ble_ns.class_("LanguageSelect", select.Select)
ScreenSaverTimeoutSelect = cuktech_ble_ns.class_("ScreenSaverTimeoutSelect", select.Select)

CONF_SCENE_MODE = "scene_mode"
CONF_LANGUAGE = "language"
CONF_SCREEN_SAVER_TIMEOUT = "screen_saver_timeout"

ICON_SCENE_MODE = "mdi:cog"
ICON_LANGUAGE = "mdi:translate"
ICON_SCREEN_SAVER_TIMEOUT = "mdi:monitor-off"

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_CUKTECH_BLE_ID): cv.use_id(CuktechBle),
    cv.Optional(CONF_SCENE_MODE): select.select_schema(
        SceneModeSelect,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_SCENE_MODE,
    ),
    cv.Optional(CONF_LANGUAGE): select.select_schema(
        LanguageSelect,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_LANGUAGE,
    ),
    cv.Optional(CONF_SCREEN_SAVER_TIMEOUT): select.select_schema(
        ScreenSaverTimeoutSelect,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_SCREEN_SAVER_TIMEOUT,
    ),
}


async def to_code(config: ConfigType) -> None:
    CuktechBle = await cg.get_variable(config[CONF_CUKTECH_BLE_ID])
    if scene_mode_config := config.get(CONF_SCENE_MODE):
        s = await select.new_select(
            scene_mode_config,
            options=[
                "AI Mode",
                "Digital Mode",
                "Single Port Mode",
                "Balance Mode",
            ],
        )
        await cg.register_parented(s, config[CONF_CUKTECH_BLE_ID])
        cg.add(CuktechBle.set_scene_mode_select(s))
    if language_config := config.get(CONF_LANGUAGE):
        s = await select.new_select(
            language_config, options=["English", "Simplified Chinese"]
        )
        await cg.register_parented(s, config[CONF_CUKTECH_BLE_ID])
        cg.add(CuktechBle.set_language_select(s))
    if screen_saver_timeout_config := config.get(CONF_SCREEN_SAVER_TIMEOUT):
        s = await select.new_select(
            screen_saver_timeout_config, options=["5 Min", "10 Min", "30 Min", "OFF", "1 Min"]
        )
        await cg.register_parented(s, config[CONF_CUKTECH_BLE_ID])
        cg.add(CuktechBle.set_screen_saver_timeout_select(s))
