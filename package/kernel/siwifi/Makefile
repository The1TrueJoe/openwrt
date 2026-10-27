#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=siwifi
PKG_RELEASE:=1

PKG_SOURCE_URL:=https://github.com/Siflower/sf_wifi.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_DATE:=2024-11-07
PKG_SOURCE_VERSION:=54f084a69087621fa52df2505327faf1d0b0c123
PKG_MIRROR_HASH:=7536f60c919aae7beb894378cf423836d6a5c336fdf83dc6063693db71d73504

# The driver has no licence file; its modules declare MODULE_LICENSE("GPL").
PKG_LICENSE:=GPL-2.0-only
PKG_LICENSE_FILES:=

PKG_BUILD_PARALLEL:=1
PKG_FLAGS:=nonshared

STAMP_CONFIGURED_DEPENDS := $(STAGING_DIR)/usr/include/mac80211-backport/backport/autoconf.h

# Kbuild runs in sf_smac/src, so Module.symvers and modules.order land there
PKG_EXTMOD_SUBDIRS:=sf_smac/src

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

SIWIFI_SRC:=$(PKG_BUILD_DIR)/sf_smac/src
SIWIFI_CFG:=$(PKG_BUILD_DIR)/sf_smac/config

define KernelPackage/siwifi
  SUBMENU:=Wireless Drivers
  TITLE:=Siflower SF19A2890 Wi-Fi driver
  DEPENDS:=@TARGET_siflower_sf19a2890 +kmod-cfg80211 +siwifi-firmware \
	+@DRIVER_11N_SUPPORT +@DRIVER_11AC_SUPPORT
  FILES:= \
	$(SIWIFI_SRC)/startcore/startcore.ko \
	$(SIWIFI_SRC)/rf/sf16a18_rf.ko \
	$(SIWIFI_SRC)/fmac/sf16a18_fmac.ko
  AUTOLOAD:=$(call AutoProbe,startcore sf16a18_rf sf16a18_fmac)
endef

define KernelPackage/siwifi/description
 FullMAC driver for the two on-chip radios of the Siflower SF19A2890
 (2.4 GHz 2x2 802.11n and 5 GHz 2x2 802.11ac). The MAC firmware runs on
 the SoC's DSP; startcore loads it, sf16a18_rf drives the shared RF
 front end and sf16a18_fmac is the cfg80211 driver for both bands.
endef

define Package/siwifi-firmware
  SECTION:=firmware
  CATEGORY:=Firmware
  TITLE:=Siflower SF19A2890 Wi-Fi firmware and default tables
  DEPENDS:=@TARGET_siflower_sf19a2890
  # The RF firmware sources (WFAC22_50C0_*.mem) carry a BSD-3-Clause
  # notice from Catena. The LMAC images, the DSP RAM images and the
  # tables carry no licence statement at all.
  LICENSE:=BSD-3-Clause NOASSERTION
endef

define Package/siwifi-firmware/description
 LMAC firmware for both bands, RF firmware and register defaults, and
 the default gain and tx-power tables used by kmod-siwifi.
endef

# Settings the vendor package uses for the SF19A28 full-mask chip. The
# LMAC images installed below are the full-size ones, so the IPC layout
# is the default one (no CONFIG_WIFI_LITE_MEMORY); the driver checks
# the layout against the firmware when it starts the LMAC.
SIWIFI_KCONFIG := \
	CONFIG_SF16A18_WIFI_MAC_HOST_OFFLOAD=m \
	CONFIG_SF16A18_WIFI_RF=m \
	CONFIG_SF16A18_WIFI_FULL_MAC=m \
	CONFIG_SIWIFI_ACS_INTERNAL=y \
	CONFIG_SFFMAC_ENABLE_TOKEN=y \
	CONFIG_DUAL_ANTENNA_CALIBRATE=y \
	CONFIG_UPDATE_GAIN_TABLE=y

NOSTDINC_FLAGS := \
	$(KERNEL_NOSTDINC_FLAGS) \
	-I$(SIWIFI_SRC) \
	-I$(STAGING_DIR)/usr/include/mac80211-backport/uapi \
	-I$(STAGING_DIR)/usr/include/mac80211-backport \
	-I$(STAGING_DIR)/usr/include/mac80211/uapi \
	-I$(STAGING_DIR)/usr/include/mac80211 \
	-include backport/autoconf.h \
	-include backport/backport.h \
	-DCONFIG_SF16A18_WIFI_RF \
	-DCONFIG_SF16A18_LMAC_USE_M_SFDSP \
	-DCONFIG_SFA28_FULLMASK \
	-DCONFIG_SF16A18_WIFI_LB_EXT_PA_ENABLE \
	-DCONFIG_SF16A18_WIFI_HB_EXT_PA_ENABLE \
	-DCONFIG_SFFMAC_ENABLE_TOKEN \
	-DCONFIG_UPDATE_GAIN_TABLE \
	-DCONFIG_COMPILE_TIME=$(SOURCE_DATE_EPOCH)

# fmac/Makefile builds the MAC from a copy of bb_src inside fmac/, as the
# vendor package did.
define Build/Prepare
	$(call Build/Prepare/Default)
	$(CP) -R $(SIWIFI_SRC)/bb_src $(SIWIFI_SRC)/fmac/
endef

define Build/Compile
	+$(KERNEL_MAKE) $(PKG_JOBS) \
		M="$(SIWIFI_SRC)" \
		DRV_PATH="$(SIWIFI_SRC)" \
		NOSTDINC_FLAGS="$(NOSTDINC_FLAGS)" \
		$(SIWIFI_KCONFIG) \
		modules
endef

define Package/siwifi-firmware/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) \
		$(SIWIFI_CFG)/siwifi_aetnensis.ini \
		$(SIWIFI_CFG)/siwifi_settings.ini \
		$(SIWIFI_CFG)/tx_adjust_gain_table.bin \
		$(SIWIFI_CFG)/a28fullmask/sf1688_lb_fmac.bin \
		$(SIWIFI_CFG)/a28fullmask/sf1688_hb_fmac.bin \
		$(SIWIFI_CFG)/a28fullmask/rf_pmem.bin \
		$(SIWIFI_CFG)/a28fullmask/rf_default_reg.bin \
		$(SIWIFI_CFG)/a28fullmask/agcram.bin \
		$(SIWIFI_CFG)/a28fullmask/agcram_24g.bin \
		$(SIWIFI_CFG)/a28fullmask/ldpcram.bin \
		$(SIWIFI_CFG)/a28fullmask/dig_gaintable.ini \
		$(SIWIFI_CFG)/a28fullmask/dig_gaintable_expa.ini \
		$(SIWIFI_CFG)/a28fullmask/default_lb_txpower_table.ini \
		$(SIWIFI_CFG)/a28fullmask/default_hb_txpower_table.ini \
		$(SIWIFI_CFG)/a28fullmask/default_lb_txpower_table_expa.ini \
		$(SIWIFI_CFG)/a28fullmask/default_hb_txpower_table_expa.ini \
		$(SIWIFI_CFG)/a28fullmask/default_hb_txpower_table_expa_low_power.ini \
		$(SIWIFI_CFG)/a28fullmask/rf_gain_table.ini \
		$(SIWIFI_CFG)/a28fullmask/rf_trx_path.ini \
		$(SIWIFI_CFG)/a28fullmask/sf_rf_expa_config.ini \
		$(SIWIFI_CFG)/cali_table/default_txpower_calibrate_table.bin \
		$(SIWIFI_CFG)/cali_table/default_txpower_calibrate_expa_table.bin \
		$(SIWIFI_CFG)/cali_table/txp_offset_sleepmode_low.ini \
		$(SIWIFI_CFG)/cali_table/txp_offset_sleepmode_low_second.ini \
		$(1)/lib/firmware/
	# The RF firmware's BSD-3-Clause notice, from the head of its source
	sed -n '10,16{s|^//\* *||;s|\r||;p;}' \
		$(SIWIFI_CFG)/a28fullmask/1.7.001/WFAC22_50C0_init.mem \
		> $(1)/lib/firmware/LICENCE.catena
endef

$(eval $(call KernelPackage,siwifi))
$(eval $(call BuildPackage,siwifi-firmware))
