#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only
#
# Run the board's real platform_check_image and the metadata check against
# good and wrong images, on the build host.
#
# Usage: test-sysupgrade.sh <openwrt tree> <bin dir>

set -u
top=$(realpath "$1")
bin=$(realpath "$2")
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
fail=0

board=glinet,gl-sft1200
dir=sysupgrade-glinet_gl-sft1200
good=$bin/openwrt-siflower-sf19a2890-glinet_gl-sft1200-squashfs-sysupgrade.bin
factory=$bin/openwrt-siflower-sf19a2890-glinet_gl-sft1200-squashfs-factory.img
evb=$bin/openwrt-siflower-sf19a2890-siflower_sf19a2890-evb-squashfs-sysupgrade.bin
fwtool=$(ls "$top"/staging_dir/host/bin/fwtool)

up=$top/package/base-files/files/lib/upgrade
sed 's|^\. /lib/functions.sh|:|' "$up/nand.sh" > "$work/nand.sh"

platform_check() {
	(
		board_name() { echo "$board"; }
		v() { echo "$@"; }
		. "$up/common.sh"
		. "$work/nand.sh"
		. "$top/target/linux/siflower/sf19a2890/base-files/lib/upgrade/platform.sh"
		platform_check_image "$1"
	) > "$work/log" 2>&1
}

metadata_check() {
	"$fwtool" -q -i "$work/meta.json" "$1" 2>/dev/null || return 1
	python3 -I -c 'import json,sys; m=json.load(open(sys.argv[1])); sys.exit(0 if sys.argv[2] in m.get("supported_devices", []) else 1)' "$work/meta.json" "$board"
}

expect() { # <pass|fail> <description> <command...>
	local want=$1 what=$2
	shift 2
	if "$@"; then got=pass; else got=fail; fi
	if [ "$got" = "$want" ]; then
		echo "PASS $what ($got as expected)"
	else
		echo "FAIL $what (wanted $want, got $got)"
		sed 's/^/     /' "$work/log" 2>/dev/null
		fail=1
	fi
}

# A sysupgrade tar whose kernel is one byte over what U-Boot reads
mkdir -p "$work/big/$dir"
tar xf "$good" -C "$work/big" 2>/dev/null
head -c 3145729 /dev/urandom > "$work/big/$dir/kernel"
tar cf "$work/bigkernel.bin" -C "$work/big" "$dir"

# A sysupgrade tar with no kernel at all
rm "$work/big/$dir/kernel"
tar cf "$work/nokernel.bin" -C "$work/big" "$dir"

# A truncated sysupgrade image
head -c $(( $(wc -c < "$good") / 2 )) "$good" > "$work/truncated.bin"

expect pass "good sysupgrade image" platform_check "$good"
expect pass "good image carries metadata for $board" metadata_check "$good"
expect fail "factory image given to sysupgrade" platform_check "$factory"
expect fail "kernel over 3 MiB" platform_check "$work/bigkernel.bin"
expect fail "image without a kernel" platform_check "$work/nokernel.bin"
expect fail "truncated sysupgrade image" platform_check "$work/truncated.bin"
if [ -f "$evb" ]; then
	expect fail "EVB image: metadata names another board" metadata_check "$evb"
	expect fail "EVB image: no $dir in it" platform_check "$evb"
else
	echo "FAIL EVB image not built"
	fail=1
fi

exit $fail
