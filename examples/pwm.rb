# Top-level include Duo, to call its class methods directly.
include Duo

# Connect external LED to GPIO2
PIN = 10
raise "Error: pin #{PIN} is not a valid GPIO" unless valid_gpio(PIN)

steps = (0..100).to_a + (1..99).to_a.reverse

led = Duo::HardwarePWM.new(PIN, frequency: 1000)

# Fade the LED up and down.
steps.cycle do |value|
  led.duty_percent = value
  sleep 0.02
end
