#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

#include <mruby.h>
#include <mruby/array.h>
#include <mruby/value.h>
#include <wiringx.h>

// Set MILKV_DUO_VARIANT in mruby build config.
// Precompiler isn't treating strings in -D flags as strings, so do this nonsense.
#define _milkv_duo      "milkv_duo"
#define _milkv_duo256m  "milkv_duo256m"
#define _milkv_duos     "milkv_duos"

static mrb_value
mrbWX_setup(mrb_state* mrb, mrb_value self) {
  int result = wiringXSetup(MILKV_DUO_VARIANT, NULL);
  if (result == -1) {
    wiringXGC();
    mrb_raise(mrb, E_RUNTIME_ERROR, "Wiring X setup failed");
  }
  return mrb_nil_value();
}

/****************************************************************************/
/*                                 GPIO                                     */
/****************************************************************************/
static mrb_value
mrbWX_valid_gpio(mrb_state* mrb, mrb_value self) {
  mrb_int pin, valid;
  mrb_get_args(mrb, "i", &pin);
  valid = wiringXValidGPIO(pin);
  return mrb_fixnum_value(valid);
}

static mrb_value
mrbWX_pin_mode(mrb_state* mrb, mrb_value self) {
  mrb_int pin, mode;
  mrb_get_args(mrb, "ii", &pin, &mode);
  pinMode(pin, mode);
  return mrb_nil_value();
}

static mrb_value
mrbWX_digital_write(mrb_state* mrb, mrb_value self) {
  mrb_int pin, state;
  mrb_get_args(mrb, "ii", &pin, &state);
  digitalWrite(pin, state);
  return mrb_nil_value();
}

static mrb_value
mrbWX_digital_read(mrb_state* mrb, mrb_value self) {
  mrb_int pin, state;
  mrb_get_args(mrb, "i", &pin);
  state = digitalRead(pin);
  return mrb_fixnum_value(state);
}

/****************************************************************************/
/*                                   PWM                                    */
/****************************************************************************/
static mrb_value
mrbWX_pwm_enable(mrb_state* mrb, mrb_value self) {
  mrb_int pin, enabled;
  mrb_get_args(mrb, "ii", &pin, &enabled);
  wiringXPWMEnable(pin, enabled);
  return mrb_nil_value();
}

static mrb_value
mrbWX_pwm_set_polarity(mrb_state* mrb, mrb_value self) {
  mrb_int pin, polarity;
  mrb_get_args(mrb, "ii", &pin, &polarity);
  wiringXPWMSetPolarity(pin, polarity);
  return mrb_nil_value();
}

static mrb_value
mrbWX_pwm_set_period(mrb_state* mrb, mrb_value self) {
  mrb_int pin, period;
  mrb_get_args(mrb, "ii", &pin, &period);
  wiringXPWMSetPeriod(pin, period);
  return mrb_nil_value();
}

static mrb_value
mrbWX_pwm_set_duty(mrb_state* mrb, mrb_value self) {
  mrb_int pin, duty;
  mrb_get_args(mrb, "ii", &pin, &duty);
  wiringXPWMSetDuty(pin, duty);
  return mrb_nil_value();
}

/****************************************************************************/
/*                                   I2C                                    */
/****************************************************************************/
static mrb_value
mrbWX_i2c_setup(mrb_state* mrb, mrb_value self) {
  // Args are Linux I2C dev index, and peripheral I2C address.
  mrb_int index, address;
  mrb_get_args(mrb, "ii", &index, &address);

  // Full I2C dev path.
  char i2c_dev[12];
  snprintf(i2c_dev, sizeof(i2c_dev), "/dev/i2c-%d", index);

  // Setup and return the file descriptor to mruby.
  int fd = wiringXI2CSetup(i2c_dev, address);
  return mrb_fixnum_value(fd);
}

static mrb_value
mrbWX_i2c_write(mrb_state* mrb, mrb_value self) {
  // Args are I2C file descriptor from i2c_setup, and array of bytes to send.
  mrb_int fd;
  mrb_value txArray;
  mrb_get_args(mrb, "iA", &fd, &txArray);

  // Copy mrb array txArray into C array txBuf.
  mrb_int length = RARRAY_LEN(txArray);
  uint8_t txBuf[length];
  for (int i=0; i<length; i++) {
    mrb_value elem = mrb_ary_ref(mrb, txArray, i);
    if (!mrb_integer_p(elem)) mrb_raise(mrb, E_TYPE_ERROR, "I2C data bytes can only be integers");
    txBuf[i] = mrb_integer(elem);
  }

  // Raw I2C write, returning number of bytes written.
  int result = write(fd, txBuf, sizeof(txBuf));
  return mrb_fixnum_value(result);
}

static mrb_value
mrbWX_i2c_read(mrb_state* mrb, mrb_value self) {
  // Args are I2C file descriptor, and number of bytes to read.
  mrb_int fd, length;
  mrb_get_args(mrb, "ii", &fd, &length);

  // Raw I2C read length bytes into C array.
  uint8_t rxBuf[length];
  read(fd, rxBuf, length);

  // Convert to mrb_ary and return.
  mrb_value rxArray = mrb_ary_new_capa(mrb, length);
  for (int i=0; i<length; i++) mrb_ary_push(mrb, rxArray, mrb_fixnum_value(rxBuf[i]));
  return rxArray;
}

/****************************************************************************/
/*                               GEM INIT                                   */
/****************************************************************************/
void
mrb_mruby_milkv_wiringx_gem_init(mrb_state* mrb) {
  // Module
  struct RClass *mrbWX = mrb_define_module(mrb, "WiringX");

  // Constants
  mrb_define_const(mrb, mrbWX, "PINMODE_NOT_SET",   mrb_fixnum_value(PINMODE_NOT_SET));
  mrb_define_const(mrb, mrbWX, "PINMODE_INPUT",     mrb_fixnum_value(PINMODE_INPUT));
  mrb_define_const(mrb, mrbWX, "PINMODE_OUTPUT",    mrb_fixnum_value(PINMODE_OUTPUT));
  mrb_define_const(mrb, mrbWX, "PINMODE_INTERRUPT", mrb_fixnum_value(PINMODE_INTERRUPT));
  mrb_define_const(mrb, mrbWX, "PINMODE_INTERRUPT", mrb_fixnum_value(PINMODE_INTERRUPT));
  mrb_define_const(mrb, mrbWX, "PINMODE_INTERRUPT", mrb_fixnum_value(PINMODE_INTERRUPT));
  mrb_define_const(mrb, mrbWX, "LOW",               mrb_fixnum_value(LOW));
  mrb_define_const(mrb, mrbWX, "HIGH",              mrb_fixnum_value(HIGH));

  // Class Methods
  mrbWX_setup(mrb, mrb_nil_value()); // Save user from calling WiringX.setup each script.
  mrb_define_method(mrb, mrbWX, "setup",          mrbWX_setup,          MRB_ARGS_REQ(0));
  mrb_define_method(mrb, mrbWX, "valid_gpio",     mrbWX_valid_gpio,     MRB_ARGS_REQ(1));

  // Digital I/O
  mrb_define_method(mrb, mrbWX, "pin_mode",       mrbWX_pin_mode,       MRB_ARGS_REQ(2));
  mrb_define_method(mrb, mrbWX, "digital_write",  mrbWX_digital_write,  MRB_ARGS_REQ(2));
  mrb_define_method(mrb, mrbWX, "digital_read",   mrbWX_digital_read,   MRB_ARGS_REQ(1));

  // PWM
  mrb_define_method(mrb, mrbWX, "pwm_enable",       mrbWX_pwm_enable,       MRB_ARGS_REQ(2));
  mrb_define_method(mrb, mrbWX, "pwm_set_polarity", mrbWX_pwm_set_polarity, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, mrbWX, "pwm_set_period",   mrbWX_pwm_set_period,   MRB_ARGS_REQ(2));
  mrb_define_method(mrb, mrbWX, "pwm_set_duty",     mrbWX_pwm_set_duty,     MRB_ARGS_REQ(2));

  // I2C
  mrb_define_method(mrb, mrbWX, "i2c_setup",        mrbWX_i2c_setup,        MRB_ARGS_REQ(2));
  mrb_define_method(mrb, mrbWX, "i2c_write",        mrbWX_i2c_write,        MRB_ARGS_REQ(2));
  mrb_define_method(mrb, mrbWX, "i2c_read",         mrbWX_i2c_read,         MRB_ARGS_REQ(2));
}

void
mrb_mruby_milkv_wiringx_gem_final(mrb_state* mrb) {
}
