PART_NAME=firmware
REQUIRE_IMAGE_METADATA=1

platform_check_image() {
	local board=$(board_name)

	case "$board" in
	glinet,gl-sft1200)
		# U-Boot loads only the first 3 MiB of the kernel partition
		local kernel_size=$(tar xOf "$1" "sysupgrade-${board//,/_}/kernel" 2>/dev/null | wc -c)
		[ "$kernel_size" -gt 0 ] && [ "$kernel_size" -le 3145728 ] || {
			echo "Kernel is missing or larger than 3 MiB"
			return 1
		}
		nand_do_platform_check "$board" "$1"
		return $?
		;;
	*)
		return 0
		;;
	esac
}

platform_do_upgrade() {
	local board=$(board_name)

	case "$board" in
	glinet,gl-sft1200)
		nand_do_upgrade "$1"
		;;
	*)
		default_do_upgrade "$1"
		;;
	esac
}
