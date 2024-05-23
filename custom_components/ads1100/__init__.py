import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["sensor", "voltage_sampler"]
MULTI_CONF = True

ads1100_ns = cg.esphome_ns.namespace("ads1100")
ADS1100Component = ads1100_ns.class_("ADS1100Component", cg.Component, i2c.I2CDevice)

CONF_CONTINUOUS_MODE = "continuous_mode"
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ADS1100Component),
            cv.Optional(CONF_CONTINUOUS_MODE, default=False): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(None))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.set_continuous_mode(config[CONF_CONTINUOUS_MODE]))
