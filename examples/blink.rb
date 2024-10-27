# Top-level include Duo, to call its class methods directly.
include Duo

PIN     = 25
pin_mode(PIN, PINMODE_OUTPUT)

loop do
  digital_write(PIN, 1)
  sleep(0.5)
  digital_write(PIN, 0)
  sleep(0.5)
end
