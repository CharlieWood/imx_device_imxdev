LOCAL_PATH:= $(call my-dir)
ALSA_LIB_INCLUDE := external/alsa-lib/include
HARDWARE_ADAPTER := $(LOCAL_PATH)/../hardware_adapter/inc 

include $(CLEAR_VARS)

# Common
LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libhardware_legacy \
	libasound \
	libmedia \
	libdl

LOCAL_STATIC_LIBRARIES += \
	libaudiointerface \
        libhardware_adapter

ifeq ($(BOARD_HAVE_BLUETOOTH),true)
  LOCAL_SHARED_LIBRARIES += liba2dp
endif

LOCAL_C_INCLUDES += \
	$(HARDWARE_ADAPTER) \
	$(ALSA_LIB_INCLUDE)

LOCAL_CFLAGS += \
	-DPIC -D_POSIX_SOURCE \
	-DALSA_CONFIG_DIR=\"/system/usr/share/alsa\" \
	-DALSA_DEVICE_DIRECTORY=\"/dev/snd/\" \
	-fno-short-enums

# build AUDIO
LOCAL_SRC_FILES+= \
	AudioHardware.cpp

LOCAL_MODULE:= libaudio

# define LOCAL_PRELINK_MODULE to false to not use pre-link map
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)
