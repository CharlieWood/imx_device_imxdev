#
# Product-specific compile-time definitions.
#

include device/imxdev/imx5x/BoardConfigCommon.mk

TARGET_BOOTLOADER_BOARD_NAME := imx51_tulip
TARGET_KERNEL_CONFIG_NAME    := imx5_android_defconfig
TARGET_UBOOT_CONFIG_NAME     := mx51_tulip_android_config

BOARD_SOC_CLASS := IMX5X
BOARD_SOC_TYPE := IMX51
BOARD_SOC_REV := TO2

# Wifi related defines
BOARD_WPA_SUPPLICANT_DRIVER := WEXT
WPA_SUPPLICANT_VERSION      := VER_0_6_X
BOARD_WLAN_DEVICE           := bcm4329
WIFI_DRIVER_MODULE_PATH     := "/system/lib/modules/bcm4329.ko"
WIFI_DRIVER_FW_STA_PATH     := "/system/vendor/firmware/fw_bcm4329.bin"
WIFI_DRIVER_FW_AP_PATH      := "/system/vendor/firmware/fw_bcm4329_apsta.bin"
WIFI_DRIVER_MODULE_ARG      := "firmware_path=/system/vendor/firmware/fw_bcm4329.bin nvram_path=/system/etc/wifi/nvram.txt iface_name=wlan"
WIFI_DRIVER_MODULE_NAME     := "dhd"

# Broadcom initialization utility
BOARD_HAVE_BLUETOOTH_BCM := true

BOARD_HAVE_VPU := true
HAVE_FSL_IMX_GPU := true
HAVE_FSL_IMX_IPU := true

# for recovery service
TARGET_SELECT_KEY := 28
TARGET_USERIMAGES_USE_EXT4 := true

BOARD_ASTER_HAS_SENSOR := true

# BCM4751 GPS
BOARD_HAVE_BROADCOM_GPS := true

# MU509 3G Modem
BOARD_3GMODEM_USE_MODEMD := true
MODEM_DEVICE_PATH := /devices/platform/fsl-ehci.1/usb1/1-1/1-1.1
