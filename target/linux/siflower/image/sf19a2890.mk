KERNEL_LOADADDR := 0x80100000

define Device/Default
  PROFILES := Default
  BLOCKSIZE := 64k
  FILESYSTEMS := squashfs
  DEVICE_DTS_DIR := ../dts
  KERNEL := kernel-bin | append-dtb | lzma | uImage lzma
  KERNEL_INITRAMFS := kernel-bin | append-dtb | lzma | uImage lzma
  IMAGES := sysupgrade.bin
  IMAGE/sysupgrade.bin = append-kernel | pad-to $$$$(BLOCKSIZE) | \
	append-rootfs | pad-rootfs | append-metadata
endef

define Device/glinet_gl-sft1200
  DEVICE_VENDOR := GL.iNet
  DEVICE_MODEL := GL-SFT1200
  DEVICE_ALT0_VENDOR := GL.iNet
  DEVICE_ALT0_MODEL := Opal
  DEVICE_DTS := sf19a2890_glinet_gl-sft1200
  DEVICE_PACKAGES := kmod-dsa-mxl-gsw1xx kmod-phy-intel-xway \
	kmod-phy-sf19a2890-usb kmod-usb-dwc2 -uboot-envtools \
	kmod-siwifi wpad-basic-mbedtls
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  # U-Boot reads only the first 3 MiB of the 4 MiB kernel partition
  KERNEL_SIZE := 3072k
  IMAGE_SIZE := 128256k
  UBINIZE_OPTS := -E 5
  IMAGES := factory.img sysupgrade.bin
  IMAGE/factory.img := append-kernel | pad-to 4096k | append-ubi | check-size
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += glinet_gl-sft1200

define Device/siflower_sf19a2890-evb
  DEVICE_VENDOR := Siflower
  DEVICE_MODEL := SF19A2890 EVB
  BOARD_NAME := siflower,sf19a2890-evb
  DEVICE_DTS := sf19a2890_evb
  DEVICE_PACKAGES := kmod-switch-rtl8367b swconfig
endef
TARGET_DEVICES += siflower_sf19a2890-evb
