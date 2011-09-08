# Copyright 2007 The Android Open Source Project
#
# The libffi code is organized primarily by architecture, but at some
# point OS-specific issues started to creep in.  In some cases there are
# OS-specific source files, in others there are just #ifdefs in the code.
# We need to generate the appropriate defines and select the right set of
# source files for the OS and architecture.

#LOCAL_PATH:= $(call my-dir)
#include $(CLEAR_VARS)
#LOCAL_SRC_FILES := hwcursortest.c
#LOCAL_MODULE := hwcursortest
#include $(BUILD_EXECUTABLE)
