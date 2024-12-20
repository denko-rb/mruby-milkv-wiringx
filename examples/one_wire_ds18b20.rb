include Duo

PIN       = 15
PARASITE  = false
one_wire  = Duo::OneWire.new(PIN)

one_wire.reset
# Skip ROM
one_wire.write([Duo::OneWire::SKIP_ROM], parasite: PARASITE)
# Start conversion
one_wire.write([Duo::OneWire::CONVERT_T], parasite: PARASITE)
# Wait for conversion
sleep(1)
# Reset
one_wire.reset
# Skip ROM
one_wire.write([Duo::OneWire::SKIP_ROM], parasite: PARASITE)
# Read 9 bytes from scratchpad
one_wire.write([Duo::OneWire::READ_SCRATCH], parasite: PARASITE)
bytes = one_wire.read(9)

# Temperature is the first 16 bits (2 bytes of 9 read).
# It's a signed, 2's complement, little-endian decimal. LSB = 2 ^ -4.
#
temperature = bytes[0..1].pack('C*').unpack('s<')[0] * (2.0 ** -4)

puts "DS18B20 reading: #{temperature} \xC2\xB0C"
