ifneq ($(TARGET_SIMULATOR),true)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= calibration.c
LOCAL_MODULE := calibrate
LOCAL_STATIC_LIBRARIES := libcutils libc
LOCAL_MODULE_TAGS := eng
include $(BUILD_EXECUTABLE)

endif
