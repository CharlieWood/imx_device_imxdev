LOCAL_PATH := $(call my-dir)
DEVICE_CFG_PATH := $(LOCAL_PATH)

include $(DEVICE_CFG_PATH)/../imx5x/AndroidBoardCommon.mk

ifeq ($(PREBUILT_FSL_IMX_CODEC),true)
include device/fsl/proprietary/codec/fsl-codec.mk
endif

include device/fsl/proprietary/gpu/fsl-gpu.mk
include device/fsl/proprietary/omx/fsl-omx.mk
include device/fsl/proprietary/render/fsl-render.mk

PRODUCT_COPY_FILES += \
	$(DEVICE_CFG_PATH)/init.aster7.rc:root/init.aster7.rc \
	$(DEVICE_CFG_PATH)/vold.fstab:system/etc/vold.fstab \
	$(DEVICE_CFG_PATH)/init.sh:system/etc/init.sh \
	$(DEVICE_CFG_PATH)/init.post_boot.sh:system/etc/init.post_boot.sh

# Original file: system/core/rootdir/etc/ueventd.freescale.rc
PRODUCT_COPY_FILES += \
	$(DEVICE_CFG_PATH)/ueventd.aster7.rc:root/ueventd.aster7.rc

PRODUCT_COPY_FILES += \
	frameworks/base/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml \
	frameworks/base/data/etc/android.hardware.camera.autofocus.xml:system/etc/permissions/android.hardware.camera.autofocus.xml \
	frameworks/base/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
	frameworks/base/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
	frameworks/base/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
	frameworks/base/data/etc/android.hardware.touchscreen.multitouch.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.xml \
	frameworks/base/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
	frameworks/base/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
	frameworks/base/data/etc/android.software.sip.xml:system/etc/permissions/android.software.sip.xml \
	frameworks/base/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml

PRODUCT_COPY_FILES += \
	$(DEVICE_CFG_PATH)/qcom_bt/init.qcom.bt.sh:system/etc/init.qcom.bt.sh \
	$(DEVICE_CFG_PATH)/qcom_bt/hci_qcomm_init:system/bin/hci_qcomm_init
