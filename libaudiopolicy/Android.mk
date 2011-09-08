ifeq ($(strip $(BOARD_USES_ALSA_AUDIO)),true)

  LOCAL_PATH := $(call my-dir)
# This is the ALSA audio policy manager

  include $(CLEAR_VARS)

  LOCAL_CFLAGS := -D_POSIX_SOURCE

ifeq ($(BOARD_HAVE_BLUETOOTH),true)
  LOCAL_CFLAGS += -DWITH_A2DP
endif

  LOCAL_SRC_FILES := AudioPolicyManagerALSA.cpp

  LOCAL_MODULE := libaudiopolicy

  LOCAL_WHOLE_STATIC_LIBRARIES += libaudiopolicybase

  LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libmedia

  include $(BUILD_SHARED_LIBRARY)
endif
