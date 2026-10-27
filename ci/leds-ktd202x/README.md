# leds-ktd202x upstream checks

This branch only exists for CI. `patches/` holds the ET6326/KTD202x LED
series as it would be mailed to linux-leds (based on next-20261026).
`.github/workflows/leds-ktd202x.yml` builds and checks it the way the
kernel test robot and the maintainers would:

- every commit, with W=1, on x86_64 (gcc with sparse, and clang), 32-bit
  i386, 32-bit MIPS and arm64
- no open-coded 64-bit division in the object
- checkpatch --strict, dt_binding_check, coccicheck, and the docs build
- the simulated-chip harness under ASan and UBSan

Nothing here is meant for openwrt/openwrt.
