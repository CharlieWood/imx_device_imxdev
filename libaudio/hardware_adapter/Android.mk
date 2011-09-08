LOCAL_PATH := $(call my-dir)
ALSA_LIB_INCLUDE := external/alsa-lib/include
HARDWARE_ADAPTER := $(LOCAL_PATH)/inc

include $(CLEAR_VARS)
LOCAL_MODULE := libhardware_adapter

LOCAL_SRC_FILES:= \
        src/hwa_control.c \
        src/audiogpo.c \
	src/audiosgtl5000.c \
        src/audionull.c 

LOCAL_SHARED_LIBRARIES := \
        libasound \
        libcutils \
        libutils \
        libc

LOCAL_C_INCLUDES += \
        $(HARDWARE_ADAPTER) \
        $(ALSA_LIB_INCLUDE)

LOCAL_CFLAGS += \
        -DPIC -D_POSIX_SOURCE \
        -DALSA_CONFIG_DIR=\"/system/usr/share/alsa\" \
        -DALSA_DEVICE_DIRECTORY=\"/dev/snd/\" \
        -fno-short-enums

include $(BUILD_STATIC_LIBRARY)
