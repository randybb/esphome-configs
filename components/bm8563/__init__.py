import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID, CONF_SLEEP_DURATION

DEPENDENCIES = ['i2c']

CONF_I2C_ADDR = 0x51

bm8563 = cg.esphome_ns.namespace('bm8563')
BM8563 = bm8563.class_('BM8563', cg.Component, i2c.I2CDevice)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(BM8563),
    cv.Optional(CONF_SLEEP_DURATION): cv.positive_time_period_seconds,
}).extend(cv.COMPONENT_SCHEMA).extend(i2c.i2c_device_schema(CONF_I2C_ADDR))

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    if CONF_SLEEP_DURATION in config:
        cg.add(var.set_sleep_duration(config[CONF_SLEEP_DURATION]))
    yield cg.register_component(var, config)
    yield i2c.register_i2c_device(var, config)
    