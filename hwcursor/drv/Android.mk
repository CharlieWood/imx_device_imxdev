LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

HW_CUR_SRC := $(LOCAL_PATH)
HW_CUR_LINUXPATH := $(ANDROID_BUILD_TOP)/$(PRODUCT_OUT)/obj/kernel

HW_MOD := $(PRODUCT_OUT)/system/lib/modules/hwcursor.ko
HW_MOD_ENV := CROSS_COMPILE="$(ANDROID_BUILD_TOP)/$(TARGET_TOOLS_PREFIX)" \
	KERNELDIR=$(HW_CUR_LINUXPATH)

.PHONY: FORCE

$(HW_MOD): kernel FORCE | $(ACP)
	$(MAKE) -C $(HW_CUR_SRC) $(HW_MOD_ENV)
	-mkdir -p $(ANDROID_BUILD_TOP)/$(PRODUCT_OUT)/system/lib/modules
	@echo "Install: $@"
	$(hide) $(ACP) $(ANDROID_BUILD_TOP)/$(HW_CUR_SRC)/hwcursor.ko "$@"

ALL_PREBUILT += $(HW_MOD)
