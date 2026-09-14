"""Swich for Cuktech BLE bridge."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import (
    CONF_TYPE,
    DEVICE_CLASS_SWITCH,
    ENTITY_CATEGORY_NONE,
    ENTITY_CATEGORY_CONFIG,
    ICON_BLUETOOTH,
)

from .. import CONF_CUKTECH_BLE_ID, CuktechBle, cuktech_ble_ns

DEPENDENCIES = ["cuktech_ble"]

EnableControllingSwitch = cuktech_ble_ns.class_("EnableControllingSwitch", switch.Switch)
ScreenDirLockSwitch = cuktech_ble_ns.class_("ScreenDirLockSwitch", switch.Switch)
ScreenSaverSwitch = cuktech_ble_ns.class_("ScreenSaverSwitch", switch.Switch)
C1PortSwitch = cuktech_ble_ns.class_("C1PortSwitch", switch.Switch)
C2PortSwitch = cuktech_ble_ns.class_("C2PortSwitch", switch.Switch)
C3PortSwitch = cuktech_ble_ns.class_("C3PortSwitch", switch.Switch)
APortSwitch = cuktech_ble_ns.class_("APortSwitch", switch.Switch)
C1PDSwitch = cuktech_ble_ns.class_("C1PDSwitch", switch.Switch)
C1PPSSwitch = cuktech_ble_ns.class_("C1PPSSwitch", switch.Switch)
C1UFCSSwitch = cuktech_ble_ns.class_("C1UFCSSwitch", switch.Switch)
C2PDSwitch = cuktech_ble_ns.class_("C2PDSwitch", switch.Switch)
C2PPSSwitch = cuktech_ble_ns.class_("C2PPSSwitch", switch.Switch)
C2UFCSSwitch = cuktech_ble_ns.class_("C2UFCSSwitch", switch.Switch)
C3SCPSwitch = cuktech_ble_ns.class_("C3SCPSwitch", switch.Switch)
C3UFCSSwitch = cuktech_ble_ns.class_("C3UFCSSwitch", switch.Switch)
ASCPSwitch = cuktech_ble_ns.class_("ASCPSwitch", switch.Switch)
AUFCSSwitch = cuktech_ble_ns.class_("AUFCSSwitch", switch.Switch)
AAlwaysOnSwitch = cuktech_ble_ns.class_("AAlwaysOnSwitch", switch.Switch)

CONF_ENABLE_CONTROLLING = "enable_controlling"
CONF_SCREEN_DIR_LOCK = "screen_dir_lock"
CONF_SCREEN_SAVER = "screen_saver"
CONF_C1_PORT = "c1_port"
CONF_C2_PORT = "c2_port"
CONF_C3_PORT = "c3_port"
CONF_A_PORT = "a_port"
CONF_C1_PD = "c1_pd"
CONF_C1_PPS = "c1_pps"
CONF_C1_UFCS = "c1_ufcs"
CONF_C2_PD = "c2_pd"
CONF_C2_PPS = "c2_pps"
CONF_C2_UFCS = "c2_ufcs"
CONF_C3_SCP = "c3_scp"
CONF_C3_UFCS = "c3_ufcs"
CONF_A_SCP = "a_scp"
CONF_A_UFCS = "a_ufcs"
CONF_A_ALWAYS_ON = "a_always_on"

DEFAULT_ICON = "mdi:toggle-switch-variant"
ICONS = {
    CONF_ENABLE_CONTROLLING: "mdi:bluetooth-connect",
    CONF_SCREEN_DIR_LOCK: "mdi:screen-rotation-lock",
    CONF_SCREEN_SAVER: "mdi:monitor-off",
    CONF_C1_PORT: "mdi:usb-c-port",
    CONF_C2_PORT: "mdi:usb-c-port",
    CONF_C3_PORT: "mdi:usb-c-port",
    CONF_A_PORT: "mdi:usb-port",
    CONF_C1_PD: "mdi:usb",
    CONF_C1_PPS: "mdi:usb",
    CONF_C1_UFCS: "mdi:usb",
    CONF_C2_PD: "mdi:usb",
    CONF_C2_PPS: "mdi:usb",
    CONF_C2_UFCS: "mdi:usb",
    CONF_C3_SCP: "mdi:usb",
    CONF_C3_UFCS: "mdi:usb",
    CONF_A_SCP: "mdi:usb",
    CONF_A_UFCS: "mdi:usb",
    CONF_A_ALWAYS_ON: "mdi:usb-port"
}

_SWITCHS = {
    CONF_ENABLE_CONTROLLING: EnableControllingSwitch,
    CONF_SCREEN_DIR_LOCK: ScreenDirLockSwitch,
    CONF_SCREEN_SAVER: ScreenSaverSwitch,
    CONF_C1_PORT: C1PortSwitch,
    CONF_C2_PORT: C2PortSwitch,
    CONF_C3_PORT: C3PortSwitch,
    CONF_A_PORT: APortSwitch,
    CONF_C1_PD: C1PDSwitch,
    CONF_C1_PPS: C1PPSSwitch,
    CONF_C1_UFCS: C1UFCSSwitch,
    CONF_C2_PD: C2PDSwitch,
    CONF_C2_PPS: C2PPSSwitch,
    CONF_C2_UFCS: C2UFCSSwitch,
    CONF_C3_SCP: C3SCPSwitch,
    CONF_C3_UFCS: C3UFCSSwitch,
    CONF_A_SCP: ASCPSwitch,
    CONF_A_UFCS: AUFCSSwitch,
    CONF_A_ALWAYS_ON: AAlwaysOnSwitch,
}

COMPONENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(cg.EntityBase),
        cv.GenerateID(CONF_CUKTECH_BLE_ID): cv.use_id(CuktechBle),
    }
)

CONFIG_SCHEMA = COMPONENT_SCHEMA.extend(
    {cv.Optional(s): switch.switch_schema(
        _SWITCHS[s],
        device_class=DEVICE_CLASS_SWITCH,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICONS[s]
    ) for s in _SWITCHS}
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_CUKTECH_BLE_ID])

    for key, sw in _SWITCHS.items():
        if key in config:
            if key is CONF_ENABLE_CONTROLLING:
                config[key]['entity_category'] = ENTITY_CATEGORY_NONE
            config[key]['icon'] = ICONS.get(key, DEFAULT_ICON)
            s = await switch.new_switch(config[key])
            cg.add(s.set_parent(parent))
            cg.add(getattr(parent, f"set_{key}_switch")(s))
            cg.add(s.set_config_name(key))