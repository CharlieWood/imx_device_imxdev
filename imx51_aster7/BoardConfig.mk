#
# Product-specific compile-time definitions.
#

include device/imxdev/imx5x/BoardConfigCommon.mk

TARGET_BOOTLOADER_BOARD_NAME := imx51_aster7
TARGET_KERNEL_CONFIG_NAME    := imx5_android_defconfig
TARGET_UBOOT_CONFIG_NAME     := mx51_aster7_android_config

BOARD_SOC_CLASS := IMX5X
BOARD_SOC_TYPE := IMX51
BOARD_SOC_REV := TO2

WIFI_DRIVER_MODULE_PATH :=  "/system/lib/modules/ar6000.ko"
WIFI_DRIVER_MODULE_ARG      := ""
WIFI_DRIVER_MODULE_NAME     := "ar6000"
WIFI_FIRMWARE_LOADER        := ""

BOARD_HAVE_VPU := true
HAVE_FSL_IMX_GPU := true
HAVE_FSL_IMX_IPU := true

# for recovery service
TARGET_SELECT_KEY := 28
TARGET_USERIMAGES_USE_EXT4 := true

BOARD_ASTER_HAS_SENSOR := true

# NMEA protocol GPS
BOARD_GPS_LIBRARIES := libgps_nmea
BOARD_HAVE_NMEA_GPS := true
