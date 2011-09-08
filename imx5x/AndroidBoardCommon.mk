LOCAL_PATH := $(call my-dir)
DEVICE_IMX5X_PATH := $(LOCAL_PATH)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := Dell_Dell_USB_Keyboard.kcm
LOCAL_MODULE_TAGS := eng
include $(BUILD_KEY_CHAR_MAP)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := mxckpd.kcm
LOCAL_MODULE_TAGS := eng
include $(BUILD_KEY_CHAR_MAP)

PRODUCT_COPY_FILES += \
	$(DEVICE_IMX5X_PATH)/apns-conf.xml:system/etc/apns-conf.xml

PRODUCT_COPY_FILES +=	\
	$(DEVICE_IMX5X_PATH)/mxckpd.kl:system/usr/keylayout/mxckpd.kl \
	$(DEVICE_IMX5X_PATH)/Dell_Dell_USB_Keyboard.kl:system/usr/keylayout/Dell_Dell_USB_Keyboard.kl

PRODUCT_COPY_FILES +=	\
	$(DEVICE_IMX5X_PATH)/init.rc:root/init.rc
