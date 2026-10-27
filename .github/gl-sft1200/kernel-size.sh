#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only
#
# Measure the GL-SFT1200 kernel uImage under candidate size reductions.
# U-Boot loads only 3145728 bytes of it. Run from the top of a tree that has
# already built tools, toolchain and packages for siflower/sf19a2890.

set -u
limit=3145728
cfg=.config
tcfg=target/linux/siflower/sf19a2890/config-6.18
cp $cfg /tmp/config.orig
cp $tcfg /tmp/tconfig.orig
results=kernel-size.txt
: > $results

kdir() { ls -d build_dir/target-*/linux-siflower_sf19a2890; }

set_opt() { # <symbol> <y|n>
	sed -i "/^CONFIG_$1=/d; /^# CONFIG_$1 is not set/d" $cfg
	if [ "$2" = y ]; then echo "CONFIG_$1=y" >> $cfg; else echo "# CONFIG_$1 is not set" >> $cfg; fi
}

set_kopt() { # <kernel symbol> <value or n>
	sed -i "/^CONFIG_$1=/d; /^# CONFIG_$1 is not set/d" $tcfg
	if [ "$2" = n ]; then echo "# CONFIG_$1 is not set" >> $tcfg; else echo "CONFIG_$1=$2" >> $tcfg; fi
}

breakdown() {
	local size=$(ls staging_dir/toolchain-*/bin/*-size | head -1)
	local ldir=$(ls -d "$(kdir)"/linux-6.*)
	echo "== built-in sizes, $1 (text+data, bytes)" >> $results
	for d in arch/mips kernel mm fs net drivers lib crypto block security init ipc; do
		[ -f "$ldir/$d/built-in.a" ] || continue
		$size -t "$ldir/$d/built-in.a" 2>/dev/null | tail -1 |
			awk -v d="$d" '{printf "%-12s %9d\n", d, $1 + $2}' >> $results
	done
	for d in drivers/net drivers/mtd net/dsa net/ipv6 net/netfilter fs/ubifs drivers/mtd/ubi; do
		[ -f "$ldir/$d/built-in.a" ] || continue
		$size -t "$ldir/$d/built-in.a" 2>/dev/null | tail -1 |
			awk -v d="$d" '{printf "  %-18s %9d\n", d, $1 + $2}' >> $results
	done
	$size "$ldir/vmlinux" >> $results
}

measure() { # <name>
	make defconfig > /dev/null
	make target/linux/clean > /dev/null 2>&1
	if ! make -j$(nproc) target/linux/compile > "build-$1.log" 2>&1; then
		printf '%-34s compile failed\n' "$1" | tee -a $results
		return
	fi
	make target/linux/install KERNEL_SIZE=16384k >> "build-$1.log" 2>&1
	local k="$(kdir)/glinet_gl-sft1200-kernel.bin"
	if [ -f "$k" ]; then
		local s=$(stat -c %s "$k")
		printf '%-34s %9d  %+8d vs limit\n' "$1" "$s" $((s - limit)) | tee -a $results
	else
		printf '%-34s no kernel image\n' "$1" | tee -a $results
	fi
	[ "$1" = baseline ] && breakdown "$1"
}

restore() {
	cp /tmp/config.orig $cfg
	cp /tmp/tconfig.orig $tcfg
}

measure baseline

set_opt KERNEL_CC_OPTIMIZE_FOR_SIZE n
set_opt KERNEL_CC_OPTIMIZE_FOR_PERFORMANCE y
measure "O2"
restore

set_opt KERNEL_KALLSYMS n
measure "no-kallsyms"
restore

for k in STMMAC_ETH STMMAC_PLATFORM DWMAC_SF19A2890 PCS_XPCS NET_DSA PHYLINK; do
	set_kopt $k m
done
measure "ethernet-dsa-modules"
restore

make defconfig > /dev/null
cat $results
