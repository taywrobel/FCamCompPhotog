# Copyright (c) 2011-2012, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

LOCAL_PATH := $(call my-dir)

# Define the example build instructions
include $(CLEAR_VARS)

LOCAL_MODULE      := jni_fcamerapro
LOCAL_ARM_MODE    := arm
TARGET_ARCH_ABI   := armeabi-v7a

ifeq ($(NDK_DEBUG),1)
  LOCAL_CFLAGS      += -DDEBUG
else
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  LOCAL_ARM_NEON  := true
endif
endif

LOCAL_CFLAGS        += -DFCAM_PLATFORM_ANDROID

MY_PREFIX           := $(LOCAL_PATH)
MY_SOURCES          := $(wildcard $(LOCAL_PATH)/*.cpp)
MY_SOURCES          += $(wildcard $(LOCAL_PATH)/*.c)
LOCAL_SRC_FILES     += $(MY_SOURCES:$(MY_PREFIX)%=%)

LOCAL_STATIC_LIBRARIES += fcamlib
LOCAL_SHARED_LIBRARIES += fcamhal
LOCAL_LDLIBS           += -llog -lGLESv2 -lEGL

include $(BUILD_SHARED_LIBRARY)

#$(call import-add-path, $(LOCAL_PATH)/../../../modules)
$(call import-add-path, $(FCAM4TEGRA_PATH)/modules)
$(call import-module,fcam/lib)
