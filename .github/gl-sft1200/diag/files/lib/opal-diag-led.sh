# Diagnostic image only: drive the ET6326 LED controller (I2C 0x30) directly,
# the way GL's gl_i2c_led does, as stage beacons readable without a console.
#
#   white solid       preinit reached
#   blue solid        /etc/rc.d finished
#   blue blinking     br-lan up and lan1, lan2 or wan has carrier
#   white and blue    a stage failed (see the USB log or netconsole)

opal_led_bus() {
	local d

	for d in /sys/bus/platform/devices/18100000.i2c/i2c-*; do
		[ -d "$d" ] && echo "${d##*/i2c-}" && return 0
	done
	return 1
}

opal_led_load() {
	[ -n "$(opal_led_bus)" ] && return 0
	mkdir -p /tmp/opal-led.d
	printf 'i2c-core\ni2c-dev\ni2c-designware-core\ni2c-designware-platform\n' \
		> /tmp/opal-led.d/50-opal-led
	kmodloader /tmp/opal-led.d/ >/dev/null 2>&1
	[ -n "$(opal_led_bus)" ]
}

# opal_led <reset|white|blue|blink|fail>
opal_led() {
	local bus mode

	opal_led_load || return 1
	bus=$(opal_led_bus)
	case "$1" in
	reset)
		i2cset -y "$bus" 0x30 0x00 0x1f 2>/dev/null
		i2cset -y "$bus" 0x30 0x00 0x1a 2>/dev/null
		i2cset -y "$bus" 0x30 0x01 0x07	# flash period 1 s
		i2cset -y "$bus" 0x30 0x02 0x80	# on for half the period
		i2cset -y "$bus" 0x30 0x05 0x00	# no ramp
		i2cset -y "$bus" 0x30 0x06 0x0a	# blue, 1.375 mA as stock
		i2cset -y "$bus" 0x30 0x07 0x0a	# white, 1.375 mA as stock
		return
		;;
	white) mode=0x04 ;;
	blue) mode=0x01 ;;
	blink) mode=0x02 ;;
	fail) mode=0x05 ;;
	*) return 1 ;;
	esac
	i2cset -y "$bus" 0x30 0x04 "$mode"
}
