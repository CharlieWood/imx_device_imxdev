LOCAL_PATH := $(call my-dir)

# HAL module implemenation, not prelinked and stored in
# hw/<HWCURSOR_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_SHARED_LIBRARIES := libcutils

#LOCAL_C_INCLUDES += 
LOCAL_SRC_FILES := 	hwcursor.c
	
LOCAL_MODULE := libhwcursor
LOCAL_MODULE_TAGS := optional
#LOCAL_CFLAGS:=
include $(BUILD_SHARED_LIBRARY)
