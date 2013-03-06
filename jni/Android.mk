LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := com.ekatas.soundtest
LOCAL_SRC_FILES := com.ekatas.soundtest.cpp
# for native audio
LOCAL_LDLIBS    += -lOpenSLES
# for logging
LOCAL_LDLIBS    += -llog
# for native asset manager
#LOCAL_LDLIBS    += -landroid

include $(BUILD_SHARED_LIBRARY)
