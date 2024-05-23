import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import i2c
from esphome.const import CONF_ID, CONF_NUMBER, CONF_MODE, CONF_INVERTED

DEPENDENCIES = ['i2c']
MULTI_CONF = True

pca9554_ns = cg.esphome_ns.namespace('pca9554')
PCA9554GPIOMode = pca9554_ns.enum('PCA9554GPIOMode')
PCF8674_GPIO_MODES = {
    'INPUT': PCA9554GPIOMode.PCA9554_INPUT,
    'OUTPUT': PCA9554GPIOMode.PCA9554_OUTPUT,
}

PCA9554Component = pca9554_ns.class_('PCA9554Component', cg.Component, i2c.I2CDevice)
PCA9554GPIOPin = pca9554_ns.class_('PCA9554GPIOPin', cg.GPIOPin)

CONF_PCA9554 = 'pca9554'
CONF_PCF8575 = 'pcf8575'
CONFIG_SCHEMA = cv.Schema({
    cv.Required(CONF_ID): cv.declare_id(PCA9554Component),
    cv.Optional(CONF_PCF8575, default=False): cv.boolean,
}).extend(cv.COMPONENT_SCHEMA).extend(i2c.i2c_device_schema(0x20))


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield i2c.register_i2c_device(var, config)
    cg.add(var.set_pcf8575(config[CONF_PCF8575]))


def validate_pca9554_gpio_mode(value):
    value = cv.string(value)
    return cv.enum(PCF8674_GPIO_MODES, upper=True)(value)


PCA9554_OUTPUT_PIN_SCHEMA = cv.Schema({
    cv.Required(CONF_PCA9554): cv.use_id(PCA9554Component),
    cv.Required(CONF_NUMBER): cv.int_,
    cv.Optional(CONF_MODE, default="OUTPUT"): validate_pca9554_gpio_mode,
    cv.Optional(CONF_INVERTED, default=False): cv.boolean,
})
PCA9554_INPUT_PIN_SCHEMA = cv.Schema({
    cv.Required(CONF_PCA9554): cv.use_id(PCA9554Component),
    cv.Required(CONF_NUMBER): cv.int_,
    cv.Optional(CONF_MODE, default="INPUT"): validate_pca9554_gpio_mode,
    cv.Optional(CONF_INVERTED, default=False): cv.boolean,
})


@pins.PIN_SCHEMA_REGISTRY.register('pca9554', (PCA9554_OUTPUT_PIN_SCHEMA, PCA9554_INPUT_PIN_SCHEMA))
def pca9554_pin_to_code(config):
    parent = yield cg.get_variable(config[CONF_PCA9554])
    yield PCA9554GPIOPin.new(parent, config[CONF_NUMBER], config[CONF_MODE], config[CONF_INVERTED])
