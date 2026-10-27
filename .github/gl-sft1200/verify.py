#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Checks on GL.iNet GL-SFT1200 images, positive and negative.

U-Boot facts used here come from disassembling the unit's own U-Boot
(GL 2016.07, "uboot2.0 version:24.04.11"):
  - its web failsafe takes a firmware upload only if the file name ends in
    ".img" and the request carries at least 10 KB; it checks nothing else,
  - it writes the file raw from flash offset 0xc0000,
  - bootcmd reads 0x300000 bytes from 0xc0000 and runs bootm on them.

Usage: verify.py <bin dir> <device dtb>
"""

import struct
import subprocess
import sys
import tarfile
import zlib
from pathlib import Path

UIMAGE_MAGIC = 0x27051956
UBOOT_READ = 0x300000
KERNEL_SLOT = 0x400000
FIRMWARE_AREA = 0x7d40000      # 0xc0000 up to "log" at 0x7e00000
FAILSAFE_MIN = 10 * 1024
PEB = 128 * 1024
BOARD = "glinet,gl-sft1200"

failures = []


def check(cond, msg):
    print(("PASS " if cond else "FAIL ") + msg)
    if not cond:
        failures.append(msg)


def uimage_ok(blob):
    """Would U-Boot's bootm accept a legacy uImage at the start of blob?"""
    if len(blob) < 64:
        return False, "shorter than a uImage header"
    hdr = bytearray(blob[:64])
    magic, hcrc, _, size, load, ep, dcrc, os_, arch, typ, comp = \
        struct.unpack(">7I4B", bytes(hdr[:32]))
    if magic != UIMAGE_MAGIC:
        return False, "no uImage magic"
    hdr[4:8] = b"\0\0\0\0"
    if zlib.crc32(hdr) != hcrc:
        return False, "bad header CRC"
    if 64 + size > len(blob):
        return False, "data runs past what U-Boot read"
    if zlib.crc32(blob[64:64 + size]) != dcrc:
        return False, "bad data CRC"
    if (os_, arch, typ) != (5, 5, 2):
        return False, "not a Linux/MIPS kernel (os %d arch %d type %d)" % (os_, arch, typ)
    return True, "load 0x%x entry 0x%x comp %d size %d" % (load, ep, comp, 64 + size)


def failsafe(name, blob):
    """What GL's web failsafe does with an upload: rejected, or boots."""
    if not name.endswith(".img"):
        return "rejected (name)"
    if len(blob) < FAILSAFE_MIN:
        return "rejected (size)"
    ok, why = uimage_ok(blob[:UBOOT_READ])
    return "boots" if ok else "accepted, does not boot: " + why


def ubi_ok(blob):
    if blob[:4] != b"UBI#":
        return False, "no UBI erase counter header"
    vid_off, data_off = struct.unpack(">II", blob[16:24])
    if (vid_off, data_off) != (2048, 4096):
        return False, "VID header %d, data %d" % (vid_off, data_off)
    head = blob[:2 * PEB]
    for vol in (b"rootfs\0", b"rootfs_data\0"):
        if vol not in head:
            return False, "no %s volume" % vol[:-1].decode()
    return True, "vid header 2048, data 4096"


def dtb_partitions(dtb):
    dts = subprocess.run(["dtc", "-q", "-I", "dtb", "-O", "dts", dtb],
                         check=True, capture_output=True, text=True).stdout
    parts, cur = {}, None
    for line in dts.splitlines():
        line = line.strip()
        if line.startswith("partition@"):
            cur = {"ro": False}
        elif cur is not None and line.startswith("label ="):
            cur["label"] = line.split('"')[1]
        elif cur is not None and line.startswith("reg ="):
            cur["reg"] = line
        elif cur is not None and line == "read-only;":
            cur["ro"] = True
        elif cur is not None and line == "};" and "label" in cur:
            parts[cur["label"]] = cur
            cur = None
    return parts


def main():
    bindir, dtb = Path(sys.argv[1]), sys.argv[2]
    prefix = "openwrt-siflower-sf19a2890-glinet_gl-sft1200-"
    factory = bindir / (prefix + "squashfs-factory.img")
    sysup = bindir / (prefix + "squashfs-sysupgrade.bin")
    initramfs = bindir / (prefix + "initramfs-kernel.bin")

    print("== flash partitions in the built DTB")
    parts = dtb_partitions(dtb)
    for name in ("spl-loader", "u-boot", "u-boot-env", "factory"):
        check(parts.get(name, {}).get("ro"), "%s is read-only" % name)
    for name in ("kernel", "ubi"):
        check(name in parts and not parts[name]["ro"], "%s is writable" % name)

    print("== factory image")
    f = factory.read_bytes()
    check(len(f) <= FIRMWARE_AREA, "fits before log (%d bytes)" % len(f))
    ok, why = uimage_ok(f[:UBOOT_READ])
    check(ok, "kernel boots from the first 3 MiB: " + why)
    ok, why = ubi_ok(f[KERNEL_SLOT:])
    check(ok, "UBI starts at 4 MiB: " + why)
    check(failsafe(factory.name, f) == "boots", "failsafe takes it and it boots")

    print("== uploads the failsafe must refuse, or that must not boot")
    check(failsafe("factory.bin", f) == "rejected (name)",
          "factory image without .img is refused")
    check(failsafe("x.img", f[:4096]).startswith("rejected"),
          "upload under 10 KB is refused")
    s = sysup.read_bytes()
    check(failsafe("x.img", s) != "boots", "sysupgrade.bin renamed .img does not boot: "
          + failsafe("x.img", s))
    cut = f[:64 + 1024 * 1024]
    check(failsafe("x.img", cut + b"\0" * FAILSAFE_MIN) != "boots",
          "factory image with a truncated kernel does not boot")
    bad = bytearray(f)
    bad[100] ^= 0xff
    check(failsafe("x.img", bytes(bad)) != "boots", "factory image with a flipped kernel byte does not boot")
    if initramfs.exists():
        i = initramfs.read_bytes()
        verdict = failsafe("x.img", i)
        print("INFO initramfs is %d bytes; via failsafe: %s" % (len(i), verdict))
        check(len(i) <= UBOOT_READ or verdict != "boots",
              "an initramfs over 3 MiB is not mistaken for a bootable flash image")

    print("== sysupgrade image")
    with tarfile.open(sysup) as t:
        names = t.getnames()
        d = "sysupgrade-" + BOARD.replace(",", "_")
        for m in ("CONTROL", "kernel", "root"):
            check(d + "/" + m in names, "tar has %s/%s" % (d, m))
        k = t.extractfile(d + "/kernel").read()
        check(len(k) <= UBOOT_READ, "sysupgrade kernel within 3 MiB (%d bytes)" % len(k))
        check(k == f[:len(k)], "sysupgrade and factory carry the same kernel")
        root = t.extractfile(d + "/root").read(4)
        check(root == b"hsqs", "root is squashfs")

    print()
    if failures:
        print("%d check(s) failed" % len(failures))
        sys.exit(1)
    print("all checks passed")


if __name__ == "__main__":
    main()
