LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := cpuloadgen.c timers_b.c

LOCAL_CFLAGS := -Wall -Wformat=2 -pthread

LOCAL_MODULE := cpuloadgen

LOCAL_STATIC_LIBRARIES := libm libc

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

LOCAL_MODULE_TAGS := debug

LOCAL_FORCE_STATIC_EXECUTABLE := true

include $(BUILD_EXECUTABLE)
