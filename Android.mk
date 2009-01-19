
ifdef HARDWARE_OMX

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TI_BRIDGE_TOP := hardware/ti/omap3/dspbridge

OMX_DEBUG := 0
RESOURCE_MANAGER_ENABLED := 0
PERF_INSTRUMENTATION := 1
PERF_CUSTOMIZABLE := 1
PERF_READER := 1

TI_OMX_CFLAGS := -Wall -fpic -pipe -DSTATIC_TABLE -O0 

ifeq ($(RESOURCE_MANAGER_ENABLED),1)
TI_OMX_CFLAGS += -DRESOURCE_MANAGER_ENABLED 
endif
ifeq ($(PERF_INSTRUMENTATION),1)
TI_OMX_CFLAGS += -D__PERF_INSTRUMENTATION__
endif

TI_OMX_TOP := $(LOCAL_PATH)
TI_OMX_SYSTEM := $(TI_OMX_TOP)/system/src/openmax_il
TI_OMX_VIDEO := $(TI_OMX_TOP)/video/src/openmax_il
TI_OMX_AUDIO := $(TI_OMX_TOP)/audio/src/openmax_il
TI_OMX_IMAGE := $(TI_OMX_TOP)/image/src/openmax_il

TI_OMX_INCLUDES := \
	$(TI_OMX_SYSTEM)/omx_core/inc

TI_OMX_COMP_SHARED_LIBRARIES := \
	libdl \
	libbridge \
	libOMX_Core \
	libLCML \
	liblog	

ifeq ($(PERF_INSTRUMENTATION),1)
TI_OMX_COMP_SHARED_LIBRARIES += \
	libPERF
endif

TI_OMX_COMP_C_INCLUDES := \
	$(TI_OMX_INCLUDES) \
	$(TI_BRIDGE_TOP)/api/inc \
	$(TI_OMX_SYSTEM)/lcml/inc \
	$(TI_OMX_SYSTEM)/common/inc \
	$(TI_OMX_SYSTEM)/perf/inc 

#call to common omx
include $(TI_OMX_SYSTEM)/omx_core/src/Android.mk
include $(TI_OMX_SYSTEM)/lcml/src/Android.mk

ifeq ($(PERF_INSTRUMENTATION),1)
include $(TI_OMX_SYSTEM)/perf/Android.mk
endif
ifeq ($(PERF_READER),1)
#TODO: Implement automatic building
#include $(TI_OMX_SYSTEM)/perf/reader/Android.mk
endif

#call to audio
include $(TI_OMX_AUDIO)/aac_dec/src/Android.mk
include $(TI_OMX_AUDIO)/aac_enc/src/Android.mk
include $(TI_OMX_AUDIO)/aac_enc/tests/Android.mk
include $(TI_OMX_AUDIO)/mp3_dec/src/Android.mk
include $(TI_OMX_AUDIO)/nbamr_dec/src/Android.mk
include $(TI_OMX_AUDIO)/nbamr_enc/src/Android.mk
include $(TI_OMX_AUDIO)/nbamr_enc/tests/Android.mk
include $(TI_OMX_AUDIO)/wbamr_dec/src/Android.mk
include $(TI_OMX_AUDIO)/wbamr_enc/src/Android.mk
include $(TI_OMX_AUDIO)/wbamr_enc/tests/Android.mk

#call to video
include $(TI_OMX_VIDEO)/video_decode/Android.mk
include $(TI_OMX_VIDEO)/video_encode/Android.mk

#call to image
include $(TI_OMX_IMAGE)/jpeg_enc/Android.mk
include $(TI_OMX_IMAGE)/jpeg_dec/Android.mk

endif
