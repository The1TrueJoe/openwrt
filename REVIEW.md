# GL.iNet GL-SFT1200 (Opal) series: review notes

For the `siflower-gl-sft1200` series. This file and `.github/gl-sft1200/` live
only on the `siflower-gl-sft1200-ci` branch and are never sent upstream.
Nothing has been submitted or pushed.

Series, on openwrt/openwrt `main` 7d0bf92bee:

1. `kernel: 6.18: backport MaxLinear GSW1xx DSA switch support`
2. `siflower: sf19a2890: terminate the RGMII delay string before parsing`
3. `siflower: sf19a2890: enable SPI NAND and UBI`
4. `siflower: sf19a2890: add support for GL.iNet GL-SFT1200 (Opal)`

"Unit" below means the harvested GL-SFT1200, label MAC 94:83:c4:9a:ea:d1, read
on GL firmware 4.8.3 (`device-dumps/glinet-sft1200/94-83-c4-9a-ea-d1`). No
OpenWrt image has run on it yet.

## Verification matrix

| # | Claim | Evidence | On the unit |
|---|---|---|---|
| 1 | SF19A2890, 128 MiB RAM | stock dmesg, `mem=128M` cmdline | yes (read) |
| 2 | GigaDevice SPI NAND, 128 MiB, 128 KiB blocks, 2 KiB pages | stock dmesg | yes (read) |
| 3 | Flash map: spl 0x0, u-boot 0x20000, env 0x80000, factory 0xa0000, kernel 0xc0000 (4 MiB), ubi 0x4c0000, log 0x7e00000 | `/proc/mtd`, stock DTB | yes (read) |
| 4 | Stock also marks spl, u-boot, u-boot-env and factory read-only | stock DTB | yes (read) |
| 5 | U-Boot reads 0x300000 bytes of kernel | bootcmd string in the unit's U-Boot | yes (binary), not executed |
| 6 | Failsafe takes any upload named `*.img` of at least 10 KB, writes it raw at 0xc0000, and erases 0xc0000 to the end of flash (`log` included); the image is a raw uImage, padding to 4 MiB, then UBI | disassembly of the unit's U-Boot (0xa002ad0c, 0xa003bc2c); GL's own stock image sft1200-4.8.3 (SHA-256 matches GL's published value) has exactly that layout, and its kernel is byte-identical to the unit's mtd5 | layout yes (GL's image); write path inferred from code, not yet exercised |
| 7 | bootm allows 64 MiB uncompressed | disassembly (unc_len 0x4000000) | inferred from code |
| 8 | U-Boot reads an env from 0x80000 and falls back to defaults on a bad CRC | env code present, the partition is all zeros, the stock unit boots | inferred |
| 9 | Base MAC at factory 0x0 = label MAC; LAN = base + 1 | factory hexdump, stock `mtd-mac-address-increment`, stock interface MACs | yes (read) |
| 10 | RGMII delay string `110a` at factory 0xb2 | factory hexdump, stock dmesg `get gmac delay: 110a` | yes (read) |
| 11 | Switch is an Intel GSW150 at SMI address 31 (0x1f) | measured on the unit by bit-banging MDIO on GPIO 28/29: only SMI 31 answers; MANU_ID (0xfa10) = 0x1713 (FIX1 1, MANID 0x389), PNUM_ID (0xfa11) = 0x3003, so part 0x31 v1.3; MII_CFG5 (0xf100) = 0x4044 as stock. The vendor's `gsw_mdio_addr` table is {0, 0x1f}; the first image used 0 and failed | yes (unit) |
| 12 | Switch port 5 adds no RGMII delay (PCDU 0xF101 = 0) | live register read | yes (read) |
| 13 | Port 0 = WAN, ports 1 and 2 = LAN | stock swconfig VLANs; link test on port 2 | port 2 yes; port 1 by elimination; port 0 from config only |
| 14 | Reset GPIO 27 and slide switch GPIO 1, both active low | stock DTB flags, `/sys/kernel/debug/gpio` idle high | yes (read) |
| 15 | Internal PHYs are 0xd565a409, handled by intel-xway | harvest readout; `PHY_ID_PHY11G_VR9_1_2` in 6.18 | ID yes; driver binding inferred |
| 16 | PL022 plus GPIO chip select drives the NAND | Hans Ulli Kroll's dmesg, from another unit; stock uses a vendor SPI driver | no |
| 17 | GSW150 DSA driver brings up all three ports with this binding | Hans Ulli Kroll's report: another unit, initramfs, his DTS | no |
| 18 | TX FIFO ≥ 2 KiB, so `tx-fifo-depth = <2048>` is a safe lower bound; `rx-fifo-depth = <2048>` keeps hardware flow control off as stock does | stock sends 1522-byte VLAN-tagged frames (CPU port `5t`, eth0.1/eth0.2 at MTU 1500) with TX store-and-forward set (vendor sf_gmac, `DMA_OPERATION_TSF`); vendor never sets `DMA_OPERATION_EFC`; 6.18 `dwmac1000_configure_fc()` enables EFC only at ≥ 4096. The real FIFO sizes are not documented and the 3.73a does not report them | config yes; sizes inferred |
| 19 | Kernel uImage fits 3 MiB | CI run 40448141508: 2918848 bytes at -Os, 226880 under; on the 25.12.6 toolchain (ParkezeOS build, run 40453704327) 3000258 bytes, 145470 under (3349991 at -O2 on main; reserve options measured: no kallsyms 2638704, stmmac+DSA as modules 2820708) | yes (build) |
| 20 | ET6326 LED controller at I2C 0x30 | U-Boot `i2c mw 0x30 ...` strings, stock DTB | yes (read); out of this series |
| 21 | Series applies with no fuzz and is refresh-clean | `patch -F0` on the build host; `make target/linux/refresh` in CI run 40448141508 leaves an empty diff (generic + siflower); OpenWrt's kernel.yml on all 98 subtargets (run 40486566465): every kernel builds; 25 patches in 8 other targets needed hunk-header-only refreshes (MAINTAINERS and DSA context moved by the backport), now folded into the backport commit | yes (CI) |
| 22 | GMAC fix and the newest backports are checkpatch clean | `checkpatch.pl --strict` from 6.18.55 | yes (host) |
| 23 | First boot on hardware (52f3163837, via GL sysupgrade) | serial log: SPL, U-Boot, kernel, SPI NAND GD5F1GQ5 (c8:51), UBI attach, ubiblock root, overlay, procd, login shell; GMAC probes with the factory MAC. The switch did not probe: at SMI 0 nothing answers, so the chip-ID read sees 0xffff (-ENODEV). Fixed: `switch@1f` | yes (unit) |
| 24 | GSW150 reset is GPIO 0, active low | stock DT `esw-rst-gpio = <0>`; stock sgmac.ko `sf_trigger_eswitch_hwReset`: output high, low for 2 ms, high, then 100 ms. A 1 s pulse with nothing bound resets the switch (RST_REQ and MDC_CFG0 back to defaults) | yes (unit) |
| 25 | All-target kernel check on a1710648ff | kernel.yml run 40542009664: 383/383 jobs green | yes (CI) |
| 26 | 7debf930a7 on hardware: DSA, sysupgrade, RGMII | `switch@1f` probes (part 0x31 v1.3, GSWIP 23); `nand_do_upgrade` from our own image works and leaves mtd0-3 and log unchanged; DHCP on LAN; 5.8 MB scp intact, 0 RGMII errors; eth0 MTU 1508 | yes (unit) |
| 27 | wan needs a hardware reset of the switch | without one, GL U-Boot's state leaves the GPHYs at internal MDIO 1-4 (port 0 has no PHY, and each port's address answers with its neighbour's GPHY); after a GPIO 0 pulse they are at 0-3, and the unmodified driver brings wan up at 1 Gbit/s. The MDIO core ignores reset-gpios on non-PHY devices, hence pending patch 761 | yes (unit, manual pulse); patched driver not yet booted |
| 28 | RST_REQ (82e2c7db0830) and GPIO_ALTSEL are not the cause | rst_override sweep and a clean reset + unmodified driver | yes (unit) |

## Tests

They live in `.github/gl-sft1200/` and run from `.github/workflows/gl-sft1200.yml`.
All passed in CI run 40448141508 (series 52f3163837).

| Test | Expected | Where |
|---|---|---|
| spl-loader, u-boot, u-boot-env and factory carry `read-only` in the built DTB; kernel and ubi do not | pass | verify.py |
| factory.img: valid uImage (header and data CRC, Linux/MIPS) in the first 3 MiB, UBI (`UBI#`, VID header 2048, data 4096, rootfs + rootfs_data) at 4 MiB, ends before `log` | pass | verify.py |
| Failsafe model refuses an upload not named `.img`, or under 10 KB | refused | verify.py |
| sysupgrade.bin renamed `.img`, a truncated kernel, or a flipped kernel byte: accepted by the failsafe but not bootable | does not boot (recoverable: U-Boot untouched) | verify.py |
| sysupgrade.bin: board dir with CONTROL/kernel/root, kernel ≤ 3 MiB and identical to factory's, root is squashfs | pass | verify.py |
| Real `platform_check_image` on the good image | accepted | test-sysupgrade.sh |
| ...on factory.img, a kernel of 3 MiB + 1 byte, no kernel, a truncated tar, and the EVB's image | rejected | test-sysupgrade.sh |
| fwtool metadata: good image lists `glinet,gl-sft1200`; EVB image does not | as stated | test-sysupgrade.sh |
| Rebuild with `KERNEL_SIZE=1024k`: "is too big", and no factory or sysupgrade image produced | no images | workflow |
| `make target/linux/refresh` leaves no diff | empty diff | workflow |
| dt-validate against Linux v7.3 bindings | switch, partitions and nvmem layout: clean. Board findings: none since the gpio-keys nodes were renamed `key-reset` and `switch-mode`. SoC-level: `rgmii-delay` nvmem cell is not in the dwmac binding (shared with the EVB); siflower clock/reset vendor properties and cpu nodes have no schema | workflow, informational |

The platform_check cases ran on macOS against synthetic images, with the real
`common.sh`, `nand.sh` and `platform.sh`. The good, factory, oversized-kernel,
no-kernel and truncated cases behaved as expected.

Not testable without the unit:
- A bad or missing factory MAC. In 6.18, `of_get_mac_address()` rejects an
  invalid address from the `mac-base` cell. stmmac then uses a random MAC, and
  the DSA user ports inherit the conduit's (`dsa_user_create()` →
  `eth_hw_addr_inherit()`). The network still comes up, with unstable MACs.
- An erased factory `rgmii-delay` cell (`ffff…`): `kstrtou16()` fails, and the
  delay registers keep U-Boot's values. That is the behaviour before commit 2,
  and the one Hans Ulli Kroll's working boot had.

## Objections a maintainer would raise, with answers

1. **"45 backport patches for one board?"** mxl-gsw1xx sits on the lantiq_gswip
   regmap conversion and the common/MMIO split, so a shorter set means
   rewriting code. Every patch is an upstream commit with its id. The only
   edit is the `dsa.h` context in 723-27, because 6.18 has no YT921X.
   Three commits are left out, each for a stated reason (commit message).
2. **"You renumbered other targets' tag ids."** That keeps upstream's values
   (GSW1XX 31, MXL862 32), so later 7.x backports apply without edits. The ids
   are an in-kernel enum; userspace sees tag names. The alternative, GSW1XX = 34
   with no other target touched, is a one-line change if preferred.
3. **"lantiq xrx200 now loads two modules. Tested?"** Packaging is updated
   (`kmod-dsa-lantiq-common`). Compile coverage comes from OpenWrt's own kernel
   workflow when the branch is pushed. Not runtime-tested on an xrx200. Open risk.
4. **"723-43..45 are not in a release."** They are in torvalds/master after
   v7.3, so they are tagged v7.4. 723-43/44 touch only GSW1xx SerDes and 2.5G
   paths, which the GSW150 lacks. They are easy to drop if only released
   commits are wanted.
5. **"stmmac FIFO depth is a guess."** It is a lower bound, not a guess.
   Stock sends 1522-byte VLAN-tagged frames in TX store-and-forward mode, so
   the TX FIFO holds at least 2 KiB. That is enough for DSA's 1508-byte MTU.
   2048 for RX keeps stmmac's hardware flow control off, which is what the
   vendor driver does. A larger TX value could let through frames the FIFO
   cannot hold. The real sizes are undocumented, so the values stay in the
   board DTS rather than the SoC dtsi.
6. **"GMAC has `pause`, the switch's fixed-link does not."** It is copied from
   the only configuration known to pass traffic (Hans Ulli Kroll's). The
   switch ignores pause frames it does not negotiate.
7. **"`rgmii` on the GMAC, `rgmii-id` with 0 ps on port 5: which side delays?"**
   The GMAC delays both directions (factory 0xb2, as stock programs it), and the
   switch adds none (stock PCDU = 0). The explicit `*-internal-delay-ps = <0>`
   states that regardless of how phy-mode is read.
8. **"A custom size check in platform_check_image."** U-Boot reads only 3 MiB of
   a 4 MiB partition, so nand.sh would happily write a kernel that cannot boot.
   The check is 6 lines and covered by a test.
9. **"Why not move the kernel into UBI or a FIT image?"** U-Boot's bootcmd is
   compiled in, and its env partition is unused. Changing either means writing
   bootloader-adjacent flash on every unit and breaking GL's recovery. The stock
   layout keeps U-Boot, the web failsafe and the factory data untouched.
10. **"No vendor header on factory.img."** GL's own SDK builds the same raw
    `kernel | pad-to 4096k | ubi`, and U-Boot checks nothing but the name and
    size (row 6).
11. **"Placeholder nodes."** Removed. The LED (`ktd202x`/ET6326) and Wi-Fi
    nodes come as follow-up commits with their drivers.
12. **"Commit 1 touches lantiq, ipq40xx, qualcommax, ramips and realtek."**
    Required for bisectability: each tree must build after every commit.
13. **"UBIFS without zlib/zstd."** Size: the 3 MiB kernel limit. LZO is
    OpenWrt's default UBIFS compressor anyway.
14. **"LAN labels."** They follow GL's firmware (port 2 = lan1), not the case
    print. Check on the unit before submitting.
15. **"Why -Os for the whole subtarget?"** At -O2 the kernel is 204 KB over
    the 3 MiB U-Boot loads. -Os puts it 227 KB under and removes no feature.
    The alternatives cost something: kallsyms is needed to read oopses on a
    new port, and namespaces and seccomp are used by procd-ujail and sshd. The
    interAptiv has 16 KiB L1 I-caches, where -Os rarely loses speed. Moving
    stmmac and DSA to modules is measured as a further option.
16. **"The target is source-only."** Unchanged here; a call for the maintainers.
17. **"`reset-gpios` is not in lantiq,gswip.yaml, and nothing uses it."**
    The MDIO core handles reset-gpios only for PHY devices, so on the switch
    node it did nothing. Two Linux patches, ready but not sent (shared notes,
    `linux-mxl-gsw1xx-reset/`, checkpatch --strict clean): the binding allows
    `reset-gpios` for the standalone GSW1xx, and mxl-gsw1xx pulses it at probe
    (2 ms asserted, 100 ms after, the vendor's timing). OpenWrt carries the
    driver half as pending-6.18 761. Targeted at net-next, not net: it adds
    DT ABI and no in-tree board is affected, so a Fixes: tag would be hard
    to justify.
18. **"Why not reset the switch in the bootloader or with a GPIO hog?"** A hog
    can only hold a level, not pulse it, and the bootloader is GL's; the
    switch driver owning its reset matches mt7530 and other MDIO switches.

## Open risks

- The pending reset patch (row 27) is not yet booted; the jack map for lan1/lan2 must be rechecked once the GPHY addresses are right (wan = WAN jack is confirmed).
- The reset timing is the vendor's, not a measured minimum or a datasheet value.
- The all-target `Build Kernel` check passed on the build but needed 25 hunk-header refreshes in other targets (folded in). Re-run it on every rebase: the backport moves MAINTAINERS and DSA context that many target patches share.
- Kernel margin on 25.12 is 145 KB, against 227 KB on main.
- -Os is a subtarget-wide default in config/Config-kernel.in. Precedent exists (`default y if TARGET_...`), but reviewers may prefer another way.
- The failsafe behaviour (row 6) comes from disassembly. opal-harvest can
  confirm the `log` wipe: write a marker on stock, recover, read it back.
- Lantiq xrx200 module split: compile only.
- Generated patches were made with git, not GNU diff/quilt; the CI refresh
  step shows any header differences.
- A boot log without serial would need `u-boot-env` writable in test images
  (proposal in the shared notes). Not implemented; needs the owner's go-ahead.
