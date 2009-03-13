
/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/* ==============================================================================
*             Texas Instruments OMAP (TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* ============================================================================ */
/**
* @file OMX_AacEncoder.c
*
* This file implements OMX Component for AAC Encoder that
* is fully compliant with the OMX Audio specification 1.5.
*
* @path  $(CSLPATH)\
*
* @rev  1.0
*/
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! ===================================
*! 13-Enc-2005 mf:  Initial Version. Change required per OMAPSWxxxxxxxxx
*! to provide _________________.
*!
* ============================================================================= */


/* ------compilation control switches -------------------------*/
/****************************************************************
*  INCLUDE FILES
****************************************************************/
/* ----- system and platform files ----------------------------*/
#ifdef UNDER_CE 
#include <windows.h>
#include <oaf_osal.h>
#include <omx_core.h>
#else
#include <wchar.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <errno.h>
#include <pthread.h>
#endif
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dbapi.h>
#include <dlfcn.h>




/*-------program files ----------------------------------------*/
#include "LCML_DspCodec.h"


#ifndef UNDER_CE
#ifdef DSP_RENDERING_ON
#include <AudioManagerAPI.h>
#endif
#endif
#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif
#include <OMX_Component.h>
#include "OMX_AacEncoder.h"
#define  AAC_ENC_ROLE "audio_encoder.aac"
#include "OMX_AacEnc_Utils.h"
#include "TIDspOmx.h"


/****************************************************************
*  EXTERNAL REFERENCES NOTE : only use if not found in header file
****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/

/****************************************************************
*  PUBLIC DECLARATIONS Defined here, used elsewhere
****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/

/****************************************************************
*  PRIVATE DECLARATIONS Defined here, used only here
****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/

static OMX_ERRORTYPE SetCallbacks(OMX_HANDLETYPE hComp, OMX_CALLBACKTYPE* pCallBacks, OMX_PTR pAppData);
static OMX_ERRORTYPE GetComponentVersion(OMX_HANDLETYPE hComp, OMX_STRING pComponentName, OMX_VERSIONTYPE* pComponentVersion, OMX_VERSIONTYPE* pSpecVersion, OMX_UUIDTYPE* pComponentUUID);
static OMX_ERRORTYPE SendCommand(OMX_HANDLETYPE hComp, OMX_COMMANDTYPE nCommand, OMX_U32 nParam,OMX_PTR pCmdData);
static OMX_ERRORTYPE GetParameter(OMX_HANDLETYPE hComp, OMX_INDEXTYPE nParamIndex, OMX_PTR ComponentParamStruct);
static OMX_ERRORTYPE SetParameter(OMX_HANDLETYPE hComp, OMX_INDEXTYPE nParamIndex, OMX_PTR ComponentParamStruct);
static OMX_ERRORTYPE GetConfig(OMX_HANDLETYPE hComp, OMX_INDEXTYPE nConfigIndex, OMX_PTR pComponentConfigStructure);
static OMX_ERRORTYPE SetConfig(OMX_HANDLETYPE hComp, OMX_INDEXTYPE nConfigIndex, OMX_PTR pComponentConfigStructure);
static OMX_ERRORTYPE EmptyThisBuffer(OMX_HANDLETYPE hComp, OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE FillThisBuffer(OMX_HANDLETYPE hComp, OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE GetState(OMX_HANDLETYPE hComp, OMX_STATETYPE* pState);
static OMX_ERRORTYPE ComponentTunnelRequest(OMX_HANDLETYPE hComp, OMX_U32 nPort, OMX_HANDLETYPE hTunneledComp, OMX_U32 nTunneledPort, OMX_TUNNELSETUPTYPE* pTunnelSetup);
static OMX_ERRORTYPE ComponentDeInit(OMX_HANDLETYPE pHandle);
static OMX_ERRORTYPE AllocateBuffer(OMX_IN OMX_HANDLETYPE hComponent, OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer, OMX_IN OMX_U32 nPortIndex, OMX_IN OMX_PTR pAppPrivate, OMX_IN OMX_U32 nSizeBytes);
static OMX_ERRORTYPE FreeBuffer(OMX_IN  OMX_HANDLETYPE hComponent, OMX_IN OMX_U32 nPortIndex, OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE UseBuffer(OMX_IN OMX_HANDLETYPE hComponent, OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_IN OMX_U32 nPortIndex, OMX_IN OMX_PTR pAppPrivate, OMX_IN OMX_U32 nSizeBytes, OMX_IN OMX_U8* pBuffer);
static OMX_ERRORTYPE GetExtensionIndex(OMX_IN  OMX_HANDLETYPE hComponent, OMX_IN  OMX_STRING cParameterName, OMX_OUT OMX_INDEXTYPE* pIndexType); 
static OMX_ERRORTYPE ComponentRoleEnum( OMX_IN OMX_HANDLETYPE hComponent,
                                                  OMX_OUT OMX_U8 *cRole,
                                                  OMX_IN OMX_U32 nIndex);


#ifdef DSP_RENDERING_ON

/* interface with audio manager*/
#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"


int Aacenc_fdwrite, Aacenc_fdread;

#ifndef UNDER_CE
AM_COMMANDDATATYPE cmd_data;
#endif

#define PERMS 0666
int errno;
#endif

/*-------------------------------------------------------------------*/
/**
  * OMX_ComponentInit() Set the all the function pointers of component
  *
  * This method will update the component function pointer to the handle
  *
  * @param hComp         handle for this instance of the component
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_ErrorInsufficientResources If the malloc fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp)
{

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONF_CHECK_CMD(hComp,1,1);                              /* checking for NULL pointers */
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef_ip = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef_op = NULL;
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_AUDIO_PARAM_AACPROFILETYPE *aac_ip = NULL;
    OMX_AUDIO_PARAM_AACPROFILETYPE *aac_op = NULL;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    OMX_AUDIO_PARAM_PCMMODETYPE *aac_pcm_ip = NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE *aac_pcm_op = NULL;
    int i;

    AACENC_DPRINT("%d :: AACENC : Entering OMX_ComponentInit\n", __LINE__);
/* For Android Tests */
    printf("%d :: AACENC : Entering OMX_ComponentInit\n", __LINE__);

    /*Set the all component function pointer to the handle*/
    pHandle->SetCallbacks           = SetCallbacks;
    pHandle->GetComponentVersion    = GetComponentVersion;
    pHandle->SendCommand            = SendCommand;
    pHandle->GetParameter           = GetParameter;
    pHandle->GetExtensionIndex      = GetExtensionIndex;
    pHandle->SetParameter           = SetParameter;
    pHandle->GetConfig              = GetConfig;
    pHandle->SetConfig              = SetConfig;
    pHandle->GetState               = GetState;
    pHandle->EmptyThisBuffer        = EmptyThisBuffer;
    pHandle->FillThisBuffer         = FillThisBuffer;
    pHandle->ComponentTunnelRequest = ComponentTunnelRequest;
    pHandle->ComponentDeInit        = ComponentDeInit;
    pHandle->AllocateBuffer         = AllocateBuffer;
    pHandle->FreeBuffer             = FreeBuffer;
    pHandle->UseBuffer              = UseBuffer;
    pHandle->ComponentRoleEnum      = ComponentRoleEnum;
    
    OMX_MALLOC_STRUCT(pHandle->pComponentPrivate, AACENC_COMPONENT_PRIVATE);
    ((AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pHandle = pHandle;
    AACENC_DPRINT("AACENC: pComponentPrivate %p \n",pHandle->pComponentPrivate);
    
    /* Initialize component data structures to default values */
    ((AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->sPortParam.nPorts = 0x2;
    ((AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->sPortParam.nStartPortNumber = 0x0;


    /* ---------start of MX_AUDIO_PARAM_AACPROFILETYPE --------- */

    OMX_MALLOC_STRUCT(aac_ip, OMX_AUDIO_PARAM_AACPROFILETYPE);
    AACENC_DPRINT("AACENC: aac_ip %p \n", aac_ip);
    OMX_MALLOC_STRUCT(aac_op, OMX_AUDIO_PARAM_AACPROFILETYPE);
    AACENC_DPRINT("AACENC: aac_op %p \n",aac_op);

    ((AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->aacParams[INPUT_PORT] = aac_ip;
    ((AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->aacParams[OUTPUT_PORT] = aac_op;
        
    aac_op->nSize               = sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE);
    aac_op->nChannels           = 2;
    aac_op->nSampleRate         = 44100;
    aac_op->eAACProfile         = OMX_AUDIO_AACObjectLC;
    aac_op->eAACStreamFormat    = OMX_AUDIO_AACStreamFormatMP2ADTS;         /* For khronos only : should  be MP4ADTS*/
    aac_op->nBitRate            = 128000;
    aac_op->eChannelMode        = OMX_AUDIO_ChannelModeStereo;
    aac_op->nPortIndex          = 1;
    aac_op->nFrameLength        = 0;
    aac_op->nAudioBandWidth     = 0;                                        /* ? */
    
    /* ---------end of MX_AUDIO_PARAM_AACPROFILETYPE --------- */


    /* ---------start of OMX_AUDIO_PARAM_PCMMODETYPE --------- */

    OMX_MALLOC_STRUCT(aac_pcm_ip, OMX_AUDIO_PARAM_PCMMODETYPE);
    AACENC_DPRINT("AACENC: aac_pcm_ip %p \n ",aac_pcm_ip);
    OMX_MALLOC_STRUCT(aac_pcm_op, OMX_AUDIO_PARAM_PCMMODETYPE);

    AACENC_DPRINT("%d :: AACENC: %p\n",__LINE__,aac_pcm_op);
    AACENC_DPRINT("AACENC: aac_pcm_op%p \n ",aac_pcm_op);
    
    aac_pcm_ip->nSize               = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
    aac_pcm_ip->nBitPerSample       = 16;                           /*Will be remapped for SN. 16:2,  24:3*/
    aac_pcm_ip->nPortIndex          = 0;
    aac_pcm_ip->nChannels           = 1;                            /*Will be remapped for SN.  0:mono, 1:stereo*/
    aac_pcm_ip->eNumData            = OMX_NumericalDataSigned;  
    aac_pcm_ip->nSamplingRate       = 8000;
    aac_pcm_ip->ePCMMode            = OMX_AUDIO_PCMModeLinear; 
    aac_pcm_ip->bInterleaved        = OMX_TRUE; 

    ((AACENC_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pcmParam[INPUT_PORT] = aac_pcm_ip;
    ((AACENC_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pcmParam[OUTPUT_PORT] = aac_pcm_op;

    /* ---------end of OMX_AUDIO_PARAM_PCMMODETYPE --------- */


    pComponentPrivate = pHandle->pComponentPrivate;

#ifdef ANDROID /* leave this now, we may need them later. */
    pComponentPrivate->iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_TRUE; /* this should be true always for TI components */
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentNeedsNALStartCode = OMX_FALSE; /* used only for H.264, leave this as false */
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_TRUE; /* N/C */
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_TRUE; /* N/C */
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_TRUE; /* experiment with this */
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsPartialFrames = OMX_TRUE; /* N/C */
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentCanHandleIncompleteFrames = OMX_TRUE; /* N/C */
#endif
    
#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERF = PERF_Create(PERF_FOURCC('A','A','E','_'),
                                               PERF_ModuleLLMM |
                                               PERF_ModuleAudioDecode);
#endif

    OMX_MALLOC_STRUCT(pComponentPrivate->pInputBufferList, BUFFERLIST);

    AACENC_DPRINT("AACENC: pInputBufferList %p\n ", pComponentPrivate->pInputBufferList);
    pComponentPrivate->pInputBufferList->numBuffers = 0; /* initialize number of buffers */

    OMX_MALLOC_STRUCT(pComponentPrivate->pOutputBufferList, BUFFERLIST);
    pComponentPrivate->pOutputBufferList->numBuffers = 0; /* initialize number of buffers */
    AACENC_DPRINT("AACENC: pOutputBufferList %p\n ", pComponentPrivate->pOutputBufferList);

    for (i=0; i < MAX_NUM_OF_BUFS; i++) 
    {
        pComponentPrivate->pOutputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->pInputBufferList->pBufHdr[i]  = NULL;
        pComponentPrivate->tickcountBufIndex[i] = 0;
        pComponentPrivate->timestampBufIndex[i]  = 0;
    }

    pComponentPrivate->IpBufindex = 0;
    pComponentPrivate->OpBufindex = 0;

    OMX_MALLOC_STRUCT_SIZE(pComponentPrivate->sDeviceString, 100*sizeof(OMX_STRING), void);
    
    /* Initialize device string to the default value */
    strcpy((char*)pComponentPrivate->sDeviceString,"/rtmdn:i2:o1/codec\0");

    /*Safety value for frames per output buffer ( for Khronos)  */
    pComponentPrivate->FramesPer_OutputBuffer = 1; 
    pComponentPrivate->CustomConfiguration = OMX_FALSE;
    
    pComponentPrivate->dasfmode                     = 0;
    pComponentPrivate->unNumChannels                = 2;
    pComponentPrivate->ulSamplingRate               = 44100;
    pComponentPrivate->unBitrate                    = 128000;
    pComponentPrivate->nObjectType                  = 2;
    pComponentPrivate->bitRateMode                  = 0;
    pComponentPrivate->File_Format                  = 2;
    pComponentPrivate->EmptybufferdoneCount         = 0;
    pComponentPrivate->EmptythisbufferCount         = 0;
    pComponentPrivate->FillbufferdoneCount          = 0;
    pComponentPrivate->FillthisbufferCount          = 0;

    pComponentPrivate->bPortDefsAllocated           = 0;
    pComponentPrivate->bCompThreadStarted           = 0;
    pComponentPrivate->bPlayCompleteFlag            = 0;
  AACENC_DPRINT("%d :: AACENC: pComponentPrivate->bPlayCompleteFlag = %ld\n",__LINE__,pComponentPrivate->bPlayCompleteFlag);
    pComponentPrivate->strmAttr                     = NULL;
    pComponentPrivate->pMarkBuf                     = NULL;
    pComponentPrivate->pMarkData                    = NULL;
    pComponentPrivate->pParams                      = NULL;
    pComponentPrivate->ptAlgDynParams               = NULL;
    pComponentPrivate->LastOutputBufferHdrQueued    = NULL;
    pComponentPrivate->bDspStoppedWhileExecuting    = OMX_FALSE;
    pComponentPrivate->nOutStandingEmptyDones       = 0;
    pComponentPrivate->nOutStandingFillDones        = 0;
    pComponentPrivate->bPauseCommandPending         = OMX_FALSE;
    pComponentPrivate->bEnableCommandPending        = 0;
    pComponentPrivate->bDisableCommandPending       = 0;
    pComponentPrivate->nNumOutputBufPending         = 0;
    pComponentPrivate->bLoadedCommandPending        = OMX_FALSE;
    pComponentPrivate->bFirstOutputBuffer = 1;

    pComponentPrivate->nUnhandledFillThisBuffers=0;
    pComponentPrivate->nUnhandledEmptyThisBuffers = 0;
    
    pComponentPrivate->bFlushOutputPortCommandPending = OMX_FALSE;
    pComponentPrivate->bFlushInputPortCommandPending = OMX_FALSE;

    pComponentPrivate->nNumInputBufPending          = 0;
    pComponentPrivate->nNumOutputBufPending         = 0;

    pComponentPrivate->PendingInPausedBufs          = 0;
    pComponentPrivate->PendingOutPausedBufs         = 0;

    /* Port format type */
    pComponentPrivate->sOutPortFormat.eEncoding     = OMX_AUDIO_CodingAAC;
    pComponentPrivate->sOutPortFormat.nIndex        = 0;/*OMX_IndexParamAudioAac;*/
    pComponentPrivate->sOutPortFormat.nPortIndex    = OUTPUT_PORT;

    pComponentPrivate->sInPortFormat.eEncoding      = OMX_AUDIO_CodingPCM;
    pComponentPrivate->sInPortFormat.nIndex         = 1;/*OMX_IndexParamAudioPcm;  */
    pComponentPrivate->sInPortFormat.nPortIndex     = INPUT_PORT;

    /*flags that control LCML closing*/
    pComponentPrivate->ptrLibLCML                   = NULL;
    pComponentPrivate->bGotLCML                     = OMX_FALSE;
    pComponentPrivate->bCodecDestroyed              = OMX_FALSE;
    

    /* initialize role name */
    strcpy((char *)pComponentPrivate->componentRole.cRole, "audio_encoder.aac");
    
#ifndef UNDER_CE
    pthread_mutex_init(&pComponentPrivate->AlloBuf_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->AlloBuf_threshold, NULL);
    pComponentPrivate->AlloBuf_waitingsignal = 0;
            
    pthread_mutex_init(&pComponentPrivate->InLoaded_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->InLoaded_threshold, NULL);
    pComponentPrivate->InLoaded_readytoidle = 0;
    
    pthread_mutex_init(&pComponentPrivate->InIdle_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->InIdle_threshold, NULL);
    pComponentPrivate->InIdle_goingtoloaded = 0;
#else
    OMX_CreateEvent(&(pComponentPrivate->AlloBuf_event));
    pComponentPrivate->AlloBuf_waitingsignal = 0;
    
    OMX_CreateEvent(&(pComponentPrivate->InLoaded_event));
    pComponentPrivate->InLoaded_readytoidle = 0;
    
    OMX_CreateEvent(&(pComponentPrivate->InIdle_event));
    pComponentPrivate->InIdle_goingtoloaded = 0;
#endif
        
    /* port definition, input port */
    OMX_MALLOC_STRUCT(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    AACENC_DPRINT("AACENC: pPortDef_ip %p \n",pPortDef_ip );


    /* port definition, output port */
    OMX_MALLOC_STRUCT(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);
    AACENC_DPRINT("AACENC: pPortDef_op %p,  size: %x \n",pPortDef_op, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));

    
    ((AACENC_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pPortDef[INPUT_PORT] = pPortDef_ip;
    ((AACENC_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pPortDef[OUTPUT_PORT] = pPortDef_op;

    
    pPortDef_ip->nSize                  = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_ip->nPortIndex             = 0x0;
    pPortDef_ip->nBufferCountActual     = NUM_AACENC_INPUT_BUFFERS;
    pPortDef_ip->nBufferCountMin        = NUM_AACENC_INPUT_BUFFERS;
    pPortDef_ip->eDir                   = OMX_DirInput;
    pPortDef_ip->bEnabled               = OMX_TRUE;
    pPortDef_ip->nBufferSize            = INPUT_AACENC_BUFFER_SIZE;
    pPortDef_ip->bPopulated             = 0;
    pPortDef_ip->format.audio.eEncoding =OMX_AUDIO_CodingPCM;  
    pPortDef_ip->eDomain                = OMX_PortDomainAudio;
    
    pPortDef_op->nSize                  = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_op->nPortIndex             = 0x1;
    pPortDef_op->nBufferCountActual     = NUM_AACENC_OUTPUT_BUFFERS; /*4*/
    pPortDef_op->nBufferCountMin        = NUM_AACENC_OUTPUT_BUFFERS;
    pPortDef_op->eDir                   = OMX_DirOutput;
    pPortDef_op->bEnabled               = OMX_TRUE;
    pPortDef_op->nBufferSize            = OUTPUT_AACENC_BUFFER_SIZE;
    pPortDef_op->bPopulated             = 0;
    pPortDef_op->format.audio.eEncoding = OMX_AUDIO_CodingAAC;   
    pPortDef_op->eDomain                = OMX_PortDomainAudio;

    pComponentPrivate->bIsInvalidState = OMX_FALSE;

    pComponentPrivate->bPreempted = OMX_FALSE; 

#ifdef RESOURCE_MANAGER_ENABLED
    /* start Resource Manager Proxy */
    eError = RMProxy_NewInitalize();
    if (eError != OMX_ErrorNone) 
    {
        AACENC_EPRINT("%d :: Error returned from loading ResourceManagerProxy thread\n",__LINE__);
        goto EXIT;
    }
#endif
#ifndef UNDER_CE
#ifdef DSP_RENDERING_ON

    /* start Audio Manager to get streamId */
    if((Aacenc_fdwrite=open(FIFO1,O_WRONLY))<0) 
    {
        AACENC_EPRINT("%d :: [AAC Encoder Component] - failure to open WRITE pipe\n",__LINE__);
    }

    if((Aacenc_fdread=open(FIFO2,O_RDONLY))<0) 
    {
        AACENC_EPRINT("%d :: [AAC Encoder Component] - failure to open READ pipe\n",__LINE__);
    }
#endif
#endif

    eError = AACENC_StartComponentThread(pHandle);
    if (eError != OMX_ErrorNone) 
    {
      AACENC_EPRINT ("%d :: Error returned from the Component\n",__LINE__);
      goto EXIT;
    }


#ifdef __PERF_INSTRUMENTATION__
    PERF_ThreadCreated(pComponentPrivate->pPERF, pComponentPrivate->ComponentThread,
                       PERF_FOURCC('A','A','E','T'));
#endif

EXIT:
    AACENC_DPRINT ("%d :: AACENC: Exiting OMX_ComponentInit\n", __LINE__);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  SetCallbacks() Sets application callbacks to the component
  *
  * This method will update application callbacks
  * to the component. So that component can make use of those call back
  * while sending buffers to the application. And also it will copy the
  * application private data to component memory
  *
  * @param pComponent    handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param pAppData      Application private data
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE SetCallbacks (OMX_HANDLETYPE pComponent,
                                   OMX_CALLBACKTYPE* pCallBacks,
                                   OMX_PTR pAppData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONF_CHECK_CMD(pComponent,1,1);                 /* Checking for NULL pointers:  pAppData is NULL for Khronos */
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*)pComponent;
    
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    AACENC_DPRINT ("%d :: AACENC: Entering SetCallbacks\n", __LINE__);
    if (pCallBacks == NULL) 
    {
        AACENC_EPRINT("%d :: Error: About to return OMX_ErrorBadParameter\n",__LINE__);
        eError = OMX_ErrorBadParameter;
        AACENC_DPRINT ("%d :: Received the empty callbacks from the application\n",__LINE__);
        goto EXIT;
    }

    /*Copy the callbacks of the application to the component private*/
    memcpy (&(pComponentPrivate->cbInfo), pCallBacks, sizeof(OMX_CALLBACKTYPE));
    /*copy the application private data to component memory */
    pHandle->pApplicationPrivate = pAppData;
    pComponentPrivate->curState = OMX_StateLoaded;

EXIT:
    AACENC_DPRINT("%d :: AACENC: Exiting SetCallbacks\n", __LINE__);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  GetComponentVersion() This will return the component version
  *
  * This method will retrun the component version
  *
  * @param hComp               handle for this instance of the component
  * @param pCompnentName       Name of the component
  * @param pCompnentVersion    handle for this instance of the component
  * @param pSpecVersion        application callbacks
  * @param pCompnentUUID
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE GetComponentVersion (OMX_HANDLETYPE hComp,
                                          OMX_STRING pComponentName,
                                          OMX_VERSIONTYPE* pComponentVersion,
                                          OMX_VERSIONTYPE* pSpecVersion,
                                          OMX_UUIDTYPE* pComponentUUID)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = (AACENC_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;

    AACENC_DPRINT("%d :: AACENC: Entering GetComponentVersion\n", __LINE__);

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

    /* Copy component version structure */
    if(pComponentVersion != NULL && pComponentName != NULL) 
    {
        strcpy(pComponentName, pComponentPrivate->cComponentName);
        memcpy(pComponentVersion, &(pComponentPrivate->ComponentVersion.s), sizeof(pComponentPrivate->ComponentVersion.s));
    }
    else 
    {
        eError = OMX_ErrorBadParameter;
    }

EXIT:

    AACENC_DPRINT("%d :: AACENC: Exiting GetComponentVersion\n", __LINE__);
    return eError;
}
/*-------------------------------------------------------------------*/
/**
  *  SendCommand() used to send the commands to the component
  *
  * This method will be used by the application.
  *
  * @param phandle         handle for this instance of the component
  * @param Cmd             Command to be sent to the component
  * @param nParam          indicates commmad is sent using this method
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE SendCommand (OMX_HANDLETYPE phandle,
                                  OMX_COMMANDTYPE Cmd,
                                  OMX_U32 nParam,OMX_PTR pCmdData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONF_CHECK_CMD(phandle,1,1);        /*NOTE: Cmd,  pCmdData, nParam  are  NULL  for khronos*/
    int nRet = 0;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)phandle;
    AACENC_COMPONENT_PRIVATE *pCompPrivate = (AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

#ifdef _ERROR_PROPAGATION__
    if (pCompPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#else
    AACENC_DPRINT ("%d :: AACENC: Entering SendCommand()\n", __LINE__);
    if(pCompPrivate->curState == OMX_StateInvalid) 
    {
           AACENC_DPRINT ("%d :: AACENC: Inside SendCommand\n",__LINE__);
           eError = OMX_ErrorInvalidState;
           AACENC_EPRINT("%d :: Error Notofication Sent to App\n",__LINE__);
           pCompPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                             OMX_EventError, 
                                             OMX_ErrorInvalidState,
                                             OMX_TI_ErrorMinor,
                                             "Invalid State");
           goto EXIT;
    }
#endif

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingCommand(pCompPrivate->pPERF,Cmd,
                        (Cmd == OMX_CommandMarkBuffer) ? ((OMX_U32) pCmdData) : nParam,
                        PERF_ModuleComponent);
#endif

    switch(Cmd) 
    {
        case OMX_CommandStateSet:
            if (nParam == OMX_StateLoaded) 
            {
                pCompPrivate->bLoadedCommandPending = OMX_TRUE;
            }
        
                AACENC_DPRINT ("%d :: AACENC: Inside SendCommand\n",__LINE__);
                AACENC_DPRINT ("%d :: AACENC: pCompPrivate->curState = %d\n",__LINE__,pCompPrivate->curState);
                if(pCompPrivate->curState == OMX_StateLoaded) 
                {
                    if((nParam == OMX_StateExecuting) || (nParam == OMX_StatePause)) 
                    {
                        pCompPrivate->cbInfo.EventHandler(pHandle,
                                                          pHandle->pApplicationPrivate,
                                                          OMX_EventError,
                                                          OMX_ErrorIncorrectStateTransition,
                                                          OMX_TI_ErrorMinor,
                                                          NULL);
                        goto EXIT;
                    }

                    if(nParam == OMX_StateInvalid) 
                    {
                        AACENC_DPRINT ("%d :: AACENC: Inside SendCommand\n",__LINE__);
                        AACENC_DPRINT("AACENC: State changed to OMX_StateInvalid Line %d\n",__LINE__);
                        pCompPrivate->curState = OMX_StateInvalid;
                        pCompPrivate->cbInfo.EventHandler(pHandle,
                                                          pHandle->pApplicationPrivate,
                                                          OMX_EventError,
                                                          OMX_ErrorInvalidState,
                                                          OMX_TI_ErrorMinor,
                                                          NULL);
                        goto EXIT;
                    }
                }
                break;

        case OMX_CommandFlush:
                AACENC_DPRINT ("%d :: IAACENC: nside SendCommand\n",__LINE__);
                if(nParam > 1 && nParam != -1) 
                {
                    eError = OMX_ErrorBadPortIndex;
                    goto EXIT;
                }
                break;
            
        case OMX_CommandPortDisable:
                AACENC_DPRINT ("%d :: AACENC: Inside SendCommand OMX_CommandPortDisable\n",__LINE__);
                break;
        
        case OMX_CommandPortEnable:
                AACENC_DPRINT ("%d :: AACENC: Inside SendCommand OMX_CommandPortEnable\n",__LINE__);
                break;
        
        case OMX_CommandMarkBuffer:
                AACENC_DPRINT ("%d :: AACENC: Inside SendCommand OMX_CommandMarkBuffer\n",__LINE__);
                if (nParam > 0) 
                {
                    eError = OMX_ErrorBadPortIndex;
                    goto EXIT;
                }
                break;
        
        default:
                AACENC_EPRINT("%d :: Error: Command Received Default error\n",__LINE__);
                pCompPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                  OMX_EventError,
                                                  OMX_ErrorBadParameter,
                                                  OMX_TI_ErrorMinor,
                                                  "Invalid Command");
                break;

    }

    AACENC_DPRINT("%d :: AACENC: Inside SendCommand\n",__LINE__);
    nRet = write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
    AACENC_DPRINT("%d :: AACENC: Cmd pipe has been writen. nRet = %d  \n",__LINE__,nRet);
    AACENC_DPRINT("%d :: AACENC: pCompPrivate->cmdPipe[1] = %d  \n",__LINE__,pCompPrivate->cmdPipe[1]);
    AACENC_DPRINT("%d :: AACENC: &Cmd = %p  \n",__LINE__,&Cmd);

    if (nRet == -1) 
    {
        AACENC_DPRINT("%d :: AACENC: Inside SendCommand\n",__LINE__);
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if (Cmd == OMX_CommandMarkBuffer) 
    {
        nRet = write(pCompPrivate->cmdDataPipe[1], &pCmdData, sizeof(OMX_PTR));
    } 
    else 
    {
        nRet = write(pCompPrivate->cmdDataPipe[1], &nParam, sizeof(OMX_U32));
    }
    
    if (nRet == -1) 
    {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    
EXIT:
    AACENC_DPRINT ("%d :: AACENC: Exiting SendCommand()\n", __LINE__);
    return eError;
}
/*-------------------------------------------------------------------*/
/**
  *  GetParameter() Gets the current configurations of the component
  *
  * @param hComp         handle for this instance of the component
  * @param nParamIndex
  * @param ComponentParameterStructure
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE GetParameter (OMX_HANDLETYPE hComp, OMX_INDEXTYPE nParamIndex, OMX_PTR ComponentParameterStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONF_CHECK_CMD(hComp,1,1);
    AACENC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pParameterStructure = NULL;

    AACENC_DPRINT("%d :: AACENC: Entering GetParameter\n", __LINE__);
    AACENC_DPRINT("%d :: AACENC: Inside the GetParameter nParamIndex = %x\n",__LINE__, nParamIndex);
    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    
    pParameterStructure = (OMX_PARAM_PORTDEFINITIONTYPE*)ComponentParameterStructure;
    if (pParameterStructure == NULL) 
    {
        AACENC_EPRINT("%d :: Error: OMX_ErrorBadParameter Inside the GetParameter Line\n",__LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;

    }

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#else
    if(pComponentPrivate->curState == OMX_StateInvalid) 
    {
        pComponentPrivate->cbInfo.EventHandler(hComp, 
                                                ((OMX_COMPONENTTYPE *)hComp)->pApplicationPrivate,
                                                OMX_EventError, 
                                                OMX_ErrorIncorrectStateOperation, 
                                                OMX_TI_ErrorMinor,
                                                "Invalid State");
        AACENC_EPRINT("%d :: AACENC: Inside the GetParameter Line\n",__LINE__);
    }
#endif  
    switch(nParamIndex)
    {

        case OMX_IndexParamAudioInit:
             memcpy(ComponentParameterStructure, &pComponentPrivate->sPortParam, sizeof(OMX_PORT_PARAM_TYPE));
             break;
        
        case OMX_IndexParamPortDefinition:
            if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pPortDef[INPUT_PORT]->nPortIndex) 
            {
                AACENC_DPRINT("%d :: AACENC: Inside the GetParameter Line\n",__LINE__);
                memcpy(ComponentParameterStructure,pComponentPrivate->pPortDef[INPUT_PORT], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            } 
            else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pPortDef[OUTPUT_PORT]->nPortIndex) 
            {
                memcpy(ComponentParameterStructure, pComponentPrivate->pPortDef[OUTPUT_PORT], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
                AACENC_DPRINT("%d :: AACENC: Inside the GetParameter \n",__LINE__);
            } 
            else 
            {
                AACENC_EPRINT("%d :: Error: BadPortIndex Inside the GetParameter \n",__LINE__);
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamAudioPortFormat:
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pPortDef[INPUT_PORT]->nPortIndex) 
            {
                if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex > pComponentPrivate->sInPortFormat.nIndex) 
                {
                    AACENC_EPRINT("%d :: Error: ErrorNoMore Inside the GetParameter Line\n",__LINE__);
                    eError = OMX_ErrorNoMore;
                } 
                else 
                {
                    AACENC_DPRINT("%d :: AACENC: About to copy Inside GetParameter \n",__LINE__);
                    memcpy(ComponentParameterStructure, &pComponentPrivate->sInPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
                }
            }
            else if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pPortDef[OUTPUT_PORT]->nPortIndex) 
            {
                    if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex > pComponentPrivate->sOutPortFormat.nIndex) 
                    {
                        AACENC_EPRINT("%d :: Error: ErrorNoMore Inside the GetParameter Line\n",__LINE__);
                        eError = OMX_ErrorNoMore;
                    } 
                    else 
                    {
                        AACENC_DPRINT("%d :: AACENC: About to copy Inside GetParameter \n",__LINE__);
                        memcpy(ComponentParameterStructure, &pComponentPrivate->sOutPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
                    }
            } 
            else 
            {
                AACENC_EPRINT("%d :: Error: BadPortIndex Inside the GetParameter Line\n",__LINE__);
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamAudioAac:
            AACENC_DPRINT("AACENC: OMX_IndexParamAudioAac : nPortIndex = %d\n", (int)((OMX_AUDIO_PARAM_AACPROFILETYPE *)(ComponentParameterStructure))->nPortIndex);
            AACENC_DPRINT("AACENC: acParams[INPUT_PORT]->nPortIndex = %d\n", (int)pComponentPrivate->aacParams[INPUT_PORT]->nPortIndex);
            AACENC_DPRINT("AACENC: acParams[OUPUT_PORT]->nPortIndex = %d\n", (int)pComponentPrivate->aacParams[OUTPUT_PORT]->nPortIndex);
            
            if(((OMX_AUDIO_PARAM_AACPROFILETYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->aacParams[INPUT_PORT]->nPortIndex) 
            {

                if (pComponentPrivate->CustomConfiguration ) /* For Testapp: An  Index  was providded. Getting the required Structure */
                                                             /* The flag is set in Setconfig() function*/
                {
                    AACENC_DPRINT("AACENC: OMX_IndexParamAudioAac :input port \n");
                    memcpy(ComponentParameterStructure,pComponentPrivate->aacParams[INPUT_PORT], sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
                    
                }
                else            /*for Khronos:  Getting the default structure (Ouput) for an  index not providded*/
                {
                    AACENC_DPRINT("AACENC: OMX_IndexParamAudioAac :output port \n");
                    memcpy(ComponentParameterStructure, pComponentPrivate->aacParams[OUTPUT_PORT], sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
                }
            } 
            else if(((OMX_AUDIO_PARAM_AACPROFILETYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->aacParams[OUTPUT_PORT]->nPortIndex) 
            {
                AACENC_DPRINT("AACENC: OMX_IndexParamAudioAac :output port \n");
                memcpy(ComponentParameterStructure, pComponentPrivate->aacParams[OUTPUT_PORT], sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));

            } 
            else 
            {
                AACENC_EPRINT("%d :: Error: BadPortIndex Inside the GetParameter Line\n",__LINE__);
                eError = OMX_ErrorBadPortIndex;
            }
            break;


        case OMX_IndexParamAudioPcm:
            if(((OMX_AUDIO_PARAM_AACPROFILETYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pcmParam[INPUT_PORT]->nPortIndex) 
            {
                memcpy(ComponentParameterStructure,pComponentPrivate->pcmParam[INPUT_PORT], sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
                AACENC_DPRINT("AACENC: OMX_IndexParamAudioPcm :input port \n");
            } 
            else if(((OMX_AUDIO_PARAM_AACPROFILETYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pcmParam[OUTPUT_PORT]->nPortIndex) 
            {
                memcpy(ComponentParameterStructure, pComponentPrivate->pcmParam[OUTPUT_PORT], sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
                AACENC_DPRINT("AACENC: OMX_IndexParamAudioPcm :output port \n");
            } 
            else 
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamCompBufferSupplier:
         if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirInput) 
         {
             AACENC_DPRINT("AACENC: GetParameter OMX_IndexParamCompBufferSupplier \n");
             
         }
         else if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirOutput) 
         {
             AACENC_DPRINT("AACENC: GetParameter OMX_IndexParamCompBufferSupplier \n");
         } 
         else 
         {
             AACENC_EPRINT("%d :: Error: OMX_ErrorBadPortIndex from GetParameter",__LINE__);
             eError = OMX_ErrorBadPortIndex;
         } 
         
            break;

#ifdef ANDROID
    case (OMX_INDEXTYPE) PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX:
    {
	AACENC_EPRINT ("Entering PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX::%d\n", __LINE__);
        PV_OMXComponentCapabilityFlagsType* pCap_flags = (PV_OMXComponentCapabilityFlagsType *) ComponentParameterStructure;
        if (NULL == pCap_flags)
        {
            AACENC_EPRINT ("%d :: ERROR PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX\n", __LINE__);
            eError =  OMX_ErrorBadParameter;
            goto EXIT;
        }
        AACENC_EPRINT ("%d :: Copying PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX\n", __LINE__);
        memcpy(pCap_flags, &(pComponentPrivate->iPVCapabilityFlags), sizeof(PV_OMXComponentCapabilityFlagsType));
	eError = OMX_ErrorNone;
    }
    break;
#endif          
            

        case OMX_IndexParamPriorityMgmt:
            break;

        
        case OMX_IndexParamVideoInit:
               break;
        
        case OMX_IndexParamImageInit:
               break;
        
        case OMX_IndexParamOtherInit:
               break;

        default:
            eError = OMX_ErrorUnsupportedIndex;
            break;
    }
EXIT:
    AACENC_DPRINT("%d :: AACENC: Exiting GetParameter:: %x  error :: %x \n",__LINE__,nParamIndex,eError);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  SetParameter() Sets configuration paramets to the component
  *
  * @param hComp         handle for this instance of the component
  * @param nParamIndex
  * @param pCompParam
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE SetParameter (OMX_HANDLETYPE hComp, OMX_INDEXTYPE nParamIndex, OMX_PTR pCompParam)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONF_CHECK_CMD(hComp,1,1);
    AACENC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    OMX_PARAM_COMPONENTROLETYPE  *pRole = NULL;
    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;

    AACENC_DPRINT ("%d :: AACENC: Entering the SetParameter()\n",__LINE__);
    AACENC_DPRINT("%d :: AACENC: Inside the SetParameter nParamIndex = %x\n",__LINE__, nParamIndex);

    if (pCompParam == NULL) 
    {
        eError = OMX_ErrorBadParameter;
        AACENC_EPRINT("%d :: Error: About to return OMX_ErrorBadParameter \n",__LINE__);
        goto EXIT;
    }

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif
    
    switch(nParamIndex) 
    {
        case OMX_IndexParamAudioPortFormat:
            {
                OMX_AUDIO_PARAM_PORTFORMATTYPE *pComponentParam = (OMX_AUDIO_PARAM_PORTFORMATTYPE *)pCompParam;
                AACENC_DPRINT("%d :: AACENC: pCompParam = index %d\n",__LINE__,(int)pComponentParam->nIndex);
                AACENC_DPRINT("%d :: AACENC: pCompParam = nportindex  %d\n",__LINE__,(int)pComponentParam->nPortIndex);
                /* for input port */
                if (pComponentParam->nPortIndex == 0) 
                {
                    AACENC_DPRINT ("%d :: AACENC: OMX_IndexParamAudioPortFormat - index 0 \n",__LINE__);
                   
                } 
                else if (pComponentParam->nPortIndex == 1) 
                {
                    AACENC_DPRINT ("%d :: AACENC: OMX_IndexParamAudioPortFormat - index 1 \n",__LINE__);                    
                }
               else 
               {
                    AACENC_EPRINT("%d :: Error: Wrong Port Index Parameter\n", __LINE__);
                    AACENC_EPRINT("%d :: Error: About to return OMX_ErrorBadParameter\n",__LINE__);
                    eError = OMX_ErrorBadParameter;
                    goto EXIT;
               }
               break;
            }
        
        case OMX_IndexParamAudioAac:
            AACENC_DPRINT ("%d :: AACENC: Inside the SetParameter - OMX_IndexParamAudioAac\n", __LINE__);
            if(((OMX_AUDIO_PARAM_AACPROFILETYPE *)(pCompParam))->nPortIndex ==
                                     pComponentPrivate->pPortDef[OUTPUT_PORT]->nPortIndex) 
            {
                memcpy(pComponentPrivate->aacParams[OUTPUT_PORT], pCompParam,
                                               sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
                AACENC_DPRINT("AACENC: nSampleRate %ld\n",pComponentPrivate->aacParams[OUTPUT_PORT]->nSampleRate);

                /* check support stereo record support at DASF */ 
                if(pComponentPrivate->aacParams[OUTPUT_PORT]->nChannels ==2)
                {
#ifndef UNDER_CE
#ifdef DSP_RENDERING_ON

                    /* inform Audio Manager support DASF stereo record */
                    cmd_data.hComponent = hComp;
                    cmd_data.AM_Cmd = AM_CommandMixerStereoRecordSupport;
                    cmd_data.param1 = OMX_TRUE;
                    if((write(Aacenc_fdwrite, &cmd_data, sizeof(cmd_data)))<0) 
                    {
                        AACENC_DPRINT("[AAC Enc Component] - send command to audio manager\n");
                    }
#endif
#endif
                }
            } 
            else if(((OMX_AUDIO_PARAM_AACPROFILETYPE *)(pCompParam))->nPortIndex ==
                                     pComponentPrivate->pPortDef[INPUT_PORT]->nPortIndex) 
            {
                                        memcpy(pComponentPrivate->aacParams[INPUT_PORT], pCompParam,
                                               sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
            }
            else 
            {
                eError = OMX_ErrorBadPortIndex;
                AACENC_EPRINT ("%d :: Error in SetParameter - OMX_IndexParamAudioAac = %x\n", __LINE__, eError);
            }
        break;

        case OMX_IndexParamAudioPcm:
            if(((OMX_AUDIO_PARAM_PCMMODETYPE *)(pCompParam))->nPortIndex ==
                    pComponentPrivate->pcmParam[INPUT_PORT]->nPortIndex) 
            {
                memcpy(pComponentPrivate->pcmParam[INPUT_PORT],pCompParam,
                       sizeof(OMX_AUDIO_PARAM_PCMMODETYPE)
                      );
            } 
            else if(((OMX_AUDIO_PARAM_PCMMODETYPE *)(pCompParam))->nPortIndex ==
                    pComponentPrivate->pcmParam[OUTPUT_PORT]->nPortIndex) 
            {
                memcpy(pComponentPrivate->pcmParam[OUTPUT_PORT],pCompParam,
                       sizeof(OMX_AUDIO_PARAM_PCMMODETYPE)
                      );

            } 
            else 
            {
                eError = OMX_ErrorBadPortIndex;
            }
        break;
        
        case OMX_IndexParamPortDefinition:
            if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex == pComponentPrivate->pPortDef[INPUT_PORT]->nPortIndex) 
            {
                memcpy(pComponentPrivate->pPortDef[INPUT_PORT], pCompParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            }
            else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex == pComponentPrivate->pPortDef[OUTPUT_PORT]->nPortIndex) 
            {
                memcpy(pComponentPrivate->pPortDef[OUTPUT_PORT], pCompParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));

            }
            else 
            {
                AACENC_EPRINT("%d :: Error: Wrong Port Index Parameter\n", __LINE__);
                AACENC_EPRINT("%d :: Error: About to return OMX_ErrorBadParameter\n",__LINE__);
                eError = OMX_ErrorBadPortIndex;
                goto EXIT;
            }
            break;

        case OMX_IndexParamPriorityMgmt:
            if (pComponentPrivate->curState != OMX_StateLoaded) 
            {
                eError = OMX_ErrorIncorrectStateOperation;
                AACENC_EPRINT("%d :: Error: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
            }
            break;

        case OMX_IndexParamStandardComponentRole:
            if (pCompParam) 
            {
                pRole = (OMX_PARAM_COMPONENTROLETYPE *)pCompParam;
                memcpy(&(pComponentPrivate->componentRole), (void *)pRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
            } 
            else 
            {
                eError = OMX_ErrorBadParameter;
            }
            break;

        
         case OMX_IndexParamCompBufferSupplier:
            if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                                    pComponentPrivate->pPortDef[INPUT_PORT]->nPortIndex) 
            {
                    AACENC_DPRINT("AACENC: SetParameter OMX_IndexParamCompBufferSupplier \n");
                                   sBufferSupplier.eBufferSupplier = OMX_BufferSupplyInput;
                                   memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));                                 
                    
            }
            else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                              pComponentPrivate->pPortDef[OUTPUT_PORT]->nPortIndex) 
            {
                AACENC_DPRINT("AACENC: SetParameter OMX_IndexParamCompBufferSupplier \n");
                sBufferSupplier.eBufferSupplier = OMX_BufferSupplyOutput;
                memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
            } 
            else 
            {
                AACENC_EPRINT("%d:: Error: OMX_ErrorBadPortIndex from SetParameter",__LINE__);
                eError = OMX_ErrorBadPortIndex;
            }
            break;


        
            
        default:
            break;
    }
EXIT:
    AACENC_DPRINT("%d :: AACENC: Exiting SetParameter:: %x\n",__LINE__,nParamIndex);
    AACENC_DPRINT ("%d :: AACENC: Exiting the SetParameter() returned eError = %d\n",__LINE__, eError);
    return eError;
}
/*-------------------------------------------------------------------*/
/**
  *  GetConfig() Gets the current configuration of to the component
  *
  * @param hComp         handle for this instance of the component
  * @param nConfigIndex
  * @param ComponentConfigStructure
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE GetConfig (OMX_HANDLETYPE hComp, OMX_INDEXTYPE nConfigIndex, OMX_PTR ComponentConfigStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    
    AACENC_COMPONENT_PRIVATE *pComponentPrivate =  NULL;
    TI_OMX_STREAM_INFO *streamInfo = NULL;

    OMX_MALLOC_STRUCT_SIZE(streamInfo, sizeof(TI_OMX_STREAM_INFO), void);
    AACENC_DPRINT("AACENC: streamInfo %p \n",streamInfo);
    
    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)
            (((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

    if(nConfigIndex == OMX_IndexCustomAacEncStreamIDConfig)
    {
        /* copy component info */
        streamInfo->streamId = pComponentPrivate->streamID;
        memcpy(ComponentConfigStructure,streamInfo,sizeof(TI_OMX_STREAM_INFO));
    }

    if(streamInfo)
    {
        OMX_MEMFREE_STRUCT(streamInfo);
        streamInfo = NULL;
    }

EXIT:    return eError;
}
/*-------------------------------------------------------------------*/
/**
  *  SetConfig() Sets the configuration of the component
  *
  * @param hComp         handle for this instance of the component
  * @param nConfigIndex
  * @param ComponentConfigStructure
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE SetConfig (OMX_HANDLETYPE hComp, OMX_INDEXTYPE nConfigIndex, OMX_PTR ComponentConfigStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
 /*   OMX_CONF_CHECK_CMD(hComp,1,ComponentConfigStructure);*/
    OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)hComp;
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    TI_OMX_DSP_DEFINITION* pDspDefinition = NULL;
    OMX_U16 FramesPerOutBuf =0  ;
    OMX_U16* ptrFramesPerOutBuf =NULL ;
    TI_OMX_DATAPATH dataPath;
    OMX_S16 *customFlag = NULL;
    
    
#ifdef DSP_RENDERING_ON
    OMX_AUDIO_CONFIG_VOLUMETYPE *pGainStructure = NULL;
#endif
    AACENC_DPRINT("%d :: AACENC: Entering SetConfig\n", __LINE__);
    if (pHandle == NULL) 
    {
        AACENC_EPRINT ("%d :: AACENC: Invalid HANDLE OMX_ErrorBadParameter \n",__LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

    switch (nConfigIndex) 
    {
        case OMX_IndexCustomAacEncHeaderInfoConfig:
        {
            pDspDefinition = (TI_OMX_DSP_DEFINITION *)ComponentConfigStructure;
            if (pDspDefinition == NULL) 
            {
                eError = OMX_ErrorBadParameter;
                AACENC_EPRINT("%d :: Error: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
                goto EXIT;
            }            
            pComponentPrivate->dasfmode = pDspDefinition->dasfMode;
            AACENC_DPRINT("AACENC: pComponentPrivate->dasfmode = %d\n",(int)pComponentPrivate->dasfmode);
            pComponentPrivate->bitRateMode = pDspDefinition->aacencHeaderInfo->bitratemode;
            AACENC_DPRINT("AACENC: pComponentPrivate->bitRateMode = %d\n",(int)pComponentPrivate->bitRateMode);
        pComponentPrivate->streamID = pDspDefinition->streamId;
            break;
        }  

        case OMX_IndexCustomAacEncFramesPerOutBuf:
        {   
            ptrFramesPerOutBuf = (OMX_U16*)ComponentConfigStructure;
            if (ptrFramesPerOutBuf == NULL)
            {
                eError = OMX_ErrorBadParameter;
                AACENC_EPRINT("%d :: Error: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
                
            }
            FramesPerOutBuf = *ptrFramesPerOutBuf;
            pComponentPrivate->FramesPer_OutputBuffer= FramesPerOutBuf;
            AACENC_DPRINT("AACENC: pComponentPrivate->FramesPer_OutputBuffer = %d \n",pComponentPrivate->FramesPer_OutputBuffer);


            pComponentPrivate->CustomConfiguration = OMX_TRUE;  /* Flag specific to test app */
            break;          
        }
           
        case OMX_IndexConfigAudioVolume:
#ifdef DSP_RENDERING_ON
            pGainStructure = (OMX_AUDIO_CONFIG_VOLUMETYPE *)ComponentConfigStructure;
            cmd_data.hComponent = hComp;
            cmd_data.AM_Cmd = AM_CommandSWGain;
            cmd_data.param1 = pGainStructure->sVolume.nValue;
            cmd_data.param2 = 0;
            cmd_data.streamID = pComponentPrivate->streamID;

            if((write(Aacenc_fdwrite, &cmd_data, sizeof(cmd_data)))<0)
            {   
                AACENC_EPRINT("[AAC encoder] - fail to send command to audio manager\n");
                AACENC_DPRINT("[AAC encoder] - fail to send command to audio manager\n");
            }
            else
            {
                AACENC_DPRINT("[AAC encoder] - ok to send command to audio manager\n");
            }
#endif
            break;

        case  OMX_IndexCustomAacEncDataPath:
            customFlag = (OMX_S16*)ComponentConfigStructure; 
            if (customFlag == NULL) 
            {
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
/*
            dataPath = *customFlag;
*/
            switch(dataPath) 
            {
                case DATAPATH_APPLICATION:
                    OMX_MMMIXER_DATAPATH(pComponentPrivate->sDeviceString, RENDERTYPE_ENCODER, pComponentPrivate->streamID);
                break;

                case DATAPATH_APPLICATION_RTMIXER:
                    strcpy((char*)pComponentPrivate->sDeviceString,(char*)RTM_STRING_ENCODER);
                break;

                default:
                break;
                    
            }
        break;
        
        default:
/*          eError = OMX_ErrorUnsupportedIndex; */
        break;
    }
EXIT:
    AACENC_DPRINT("%d :: AACENC: Exiting SetConfig\n", __LINE__);
    AACENC_DPRINT("%d :: AACENC: Returning = 0x%x\n",__LINE__,eError);
    return eError;


}
/*-------------------------------------------------------------------*/
/**
  *  GetState() Gets the current state of the component
  *
  * @param pCompomponent handle for this instance of the component
  * @param pState
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE GetState (OMX_HANDLETYPE pComponent, OMX_STATETYPE* pState)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    OMX_CONF_CHECK_CMD(pComponent,1,1);
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;

    AACENC_DPRINT ("%d :: AACENC: Entering GetState\n", __LINE__);
    if (!pState) 
    {
        eError = OMX_ErrorBadParameter;
        AACENC_EPRINT("%d :: Error: About to return OMX_ErrorBadParameter \n",__LINE__);
        goto EXIT;
    }

    if (pHandle && pHandle->pComponentPrivate) 
    {
        *pState =  ((AACENC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->curState;
    } 
    else 
    {
        *pState = OMX_StateLoaded;
    }
    eError = OMX_ErrorNone;

EXIT:
    AACENC_DPRINT ("%d :: AACENC: Exiting GetState\n", __LINE__);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  EmptyThisBuffer() This callback is used to send the input buffer to
  *  component
  *
  * @param pComponent       handle for this instance of the component
  * @param nPortIndex       input port index
  * @param pBuffer          buffer to be sent to codec
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE EmptyThisBuffer (OMX_HANDLETYPE pComponent,
                                      OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONF_CHECK_CMD(pComponent,pBuffer,1);
    int ret = 0;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;

    AACENC_DPRINT ("%d :: AACENC: Entering EmptyThisBuffer\n", __LINE__);

    pPortDef = ((AACENC_COMPONENT_PRIVATE*) 
                    pComponentPrivate)->pPortDef[INPUT_PORT];

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

#ifdef __PERF_INSTRUMENTATION__
        PERF_ReceivedFrame(pComponentPrivate->pPERF,pBuffer->pBuffer, pBuffer->nFilledLen,
                           PERF_ModuleHLMM);
#endif

    AACENC_DPRINT("AACENC: pBuffer->nSize %d \n",(int)pBuffer->nSize);
    AACENC_DPRINT("AACENC: size OMX_BUFFERHEADERTYPE %d \n",sizeof(OMX_BUFFERHEADERTYPE));
    
    if(!pPortDef->bEnabled) 
    {
        eError = OMX_ErrorIncorrectStateOperation;
        AACENC_DPRINT("%d :: Error: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
        goto EXIT;
    }
    if (pBuffer == NULL) 
    {
        eError = OMX_ErrorBadParameter;
        AACENC_DPRINT("%d :: Error: About to return OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }
    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) 
    {
        AACENC_DPRINT("%d :: Error: About to return OMX_ErrorBadParameter\n",__LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }


    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) 
    {
        eError = OMX_ErrorVersionMismatch;
        goto EXIT;
    }

    if (pBuffer->nInputPortIndex != INPUT_PORT) 
    {
        AACENC_DPRINT("%d :: Error: About to return OMX_ErrorBadPortIndex\n",__LINE__);
        eError  = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
    AACENC_DPRINT("\n------------------------------------------\n\n");
    AACENC_DPRINT ("%d :: AACENC: Component Sending Filled ip buff %p  to Component Thread\n", __LINE__,pBuffer);
    AACENC_DPRINT("\n------------------------------------------\n\n");

    pComponentPrivate->pMarkData = pBuffer->pMarkData;
    pComponentPrivate->hMarkTargetComponent = pBuffer->hMarkTargetComponent;

    pComponentPrivate->nUnhandledEmptyThisBuffers++;

    ret = write (pComponentPrivate->dataPipe[1], &pBuffer, sizeof(OMX_BUFFERHEADERTYPE*));
    if (ret == -1) 
    {
        AACENC_EPRINT ("%d :: Error in Writing to the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    pComponentPrivate->EmptythisbufferCount++;

EXIT:
    AACENC_DPRINT ("%d :: AACENC: Exiting EmptyThisBuffer\n", __LINE__);
    return eError;
}
/*-------------------------------------------------------------------*/
/**
  *  FillThisBuffer() This callback is used to send the output buffer to
  *  the component
  *
  * @param pComponent    handle for this instance of the component
  * @param nPortIndex    output port number
  * @param pBuffer       buffer to be sent to codec
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE FillThisBuffer (OMX_HANDLETYPE pComponent,
                                     OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONF_CHECK_CMD(pComponent,pBuffer,1);
    int ret = 0;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AACENC_COMPONENT_PRIVATE *pComponentPrivate =
                         (AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;

    AACENC_DPRINT ("%d :: AACENC: Entering FillThisBuffer\n", __LINE__);

    AACENC_DPRINT("\n------------------------------------------\n\n");
    AACENC_DPRINT("%d :: AACENC: Component Sending Emptied op buff %p to Component Thread\n",__LINE__,pBuffer);
    AACENC_DPRINT("\n------------------------------------------\n\n");
    pPortDef = ((AACENC_COMPONENT_PRIVATE*) 
                    pComponentPrivate)->pPortDef[OUTPUT_PORT];

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,pBuffer->pBuffer,0,PERF_ModuleHLMM);
#endif

    if(!pPortDef->bEnabled) 
    {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    if (pBuffer == NULL) 
    {
        eError = OMX_ErrorBadParameter;
        AACENC_EPRINT(" %d :: Error: About to return OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }
    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) 
    {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) 
    {
        eError = OMX_ErrorVersionMismatch;
        goto EXIT;
    }

    if (pBuffer->nOutputPortIndex != OUTPUT_PORT) 
    {
        eError  = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
    pBuffer->nFilledLen = 0;
    /*Filling the Output buffer with zero */
#ifndef UNDER_CE    
    /*memset(pBuffer->pBuffer, 0, pBuffer->nAllocLen);*/
#endif    

    AACENC_DPRINT("%d :: AACENC: pComponentPrivate->pMarkBuf = %p\n",__LINE__, pComponentPrivate->pMarkBuf);
    AACENC_DPRINT("%d :: AACENC: pComponentPrivate->pMarkData = %p\n",__LINE__, pComponentPrivate->pMarkData);
    if(pComponentPrivate->pMarkBuf) 
    {
        pBuffer->hMarkTargetComponent = pComponentPrivate->pMarkBuf->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkBuf->pMarkData;
        pComponentPrivate->pMarkBuf = NULL;
    }

    if (pComponentPrivate->pMarkData) 
    {
        pBuffer->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkData;
        pComponentPrivate->pMarkData = NULL;
    }

    pComponentPrivate->nUnhandledFillThisBuffers++;

    ret = write (pComponentPrivate->dataPipe[1], &pBuffer, sizeof (OMX_BUFFERHEADERTYPE*));
    if (ret == -1) 
    {
        AACENC_EPRINT ("%d :: Error in Writing to the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    pComponentPrivate->FillthisbufferCount++;

EXIT:
    AACENC_DPRINT("%d :: AACENC: Exiting FillThisBuffer error= %d \n", __LINE__,eError);
    return eError;
}
/*-------------------------------------------------------------------*/
/**
  * OMX_ComponentDeinit() this methold will de init the component
  *
  * @param pComp         handle for this instance of the component
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE ComponentDeInit(OMX_HANDLETYPE pHandle)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONF_CHECK_CMD(pHandle,1,1);
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)pComponent->pComponentPrivate;

    AACENC_DPRINT("%d :: AACENC: ComponentDeInit\n",__LINE__);
    AACENC_DPRINT("AACENC:  LCML %p \n",pComponentPrivate->ptrLibLCML);

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif

#ifndef UNDER_CE
#ifdef DSP_RENDERING_ON
    close(Aacenc_fdwrite);
    close(Aacenc_fdread);
#endif
#endif

#ifdef RESOURCE_MANAGER_ENABLED
    eError = RMProxy_NewSendCommand(pHandle, RMProxy_FreeResource, OMX_AAC_Encoder_COMPONENT, 0, 3456, NULL);
    if (eError != OMX_ErrorNone) 
    {
         AACENC_EPRINT ("%d :: Error returned from destroy ResourceManagerProxy thread\n",
                                                        __LINE__);
    }

    eError = RMProxy_Deinitalize();
    if (eError != OMX_ErrorNone) 
    {
         AACENC_EPRINT ("%d :: Error returned from destroy ResourceManagerProxy thread\n",__LINE__);
    }
#endif

#ifdef SWAT_ANALYSIS
    SWATAPI_ReleaseHandle(pComponentPrivate->pSwatInfo->pSwatApiHandle);
    SWAT_Boundary(pComponentPrivate->pSwatInfo->pSwatObjHandle,
                          pComponentPrivate->pSwatInfo->ctUC,
                          SWAT_BoundaryComplete | SWAT_BoundaryCleanup);
    AACENC_DPRINT("%d :: AACENC: Instrumentation: SWAT_BoundaryComplete Done\n",__LINE__);
    SWAT_Done(pComponentPrivate->pSwatInfo->pSwatObjHandle);
#endif

    AACENC_DPRINT("%d :: AACENC: Inside ComponentDeInit point A \n",__LINE__);
    pComponentPrivate->bIsThreadstop = 1;
    eError = AACENC_StopComponentThread(pHandle);
    AACENC_DPRINT("%d :: AACENC: Inside ComponentDeInit Point B \n",__LINE__);
    /* Wait for thread to exit so we can get the status into "error" */

    /* close the pipe handles */
    AACENC_FreeCompResources(pHandle);

#ifdef __PERF_INSTRUMENTATION__
        PERF_Boundary(pComponentPrivate->pPERF,
                      PERF_BoundaryComplete | PERF_BoundaryCleanup);
        PERF_Done(pComponentPrivate->pPERF);
#endif
    
    OMX_MEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);
    AACENC_DPRINT("%d :: AACENC: After AACENC_FreeCompResources\n",__LINE__);
    AACENC_DPRINT("%d :: AACENC: [FREE] %p\n",__LINE__,pComponentPrivate);

    if (pComponentPrivate->sDeviceString != NULL) 
    {
        OMX_MEMFREE_STRUCT(pComponentPrivate->sDeviceString);
    }
    
    /* CLose LCML .      - Note:  Need to handle better - */ 
    if ((pComponentPrivate->ptrLibLCML != NULL && pComponentPrivate->bGotLCML) &&
        (pComponentPrivate->bCodecDestroyed))
    {
        AACENC_DPRINT("AACENC: About to Close LCML %p \n",pComponentPrivate->ptrLibLCML);
        dlclose( pComponentPrivate->ptrLibLCML  );   
        pComponentPrivate->ptrLibLCML = NULL;
        AACENC_DPRINT("AACENC: Closed LCML \n");  

        pComponentPrivate->bCodecDestroyed = OMX_FALSE;     /* restoring flags */
        pComponentPrivate->bGotLCML        = OMX_FALSE;
    }
    
    OMX_MEMFREE_STRUCT(pComponentPrivate);
    pComponentPrivate = NULL;
    
EXIT:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  ComponentTunnelRequest() this method is not implemented in 1.5
  *
  * This method will update application callbacks
  * the application.
  *
  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE ComponentTunnelRequest (OMX_HANDLETYPE hComp,
                                             OMX_U32 nPort, OMX_HANDLETYPE hTunneledComp,
                                             OMX_U32 nTunneledPort,
                                             OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    eError = OMX_ErrorNotImplemented;
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  AllocateBuffer()

  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE AllocateBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                   OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
                   OMX_IN OMX_U32 nPortIndex,
                   OMX_IN OMX_PTR pAppPrivate,
                   OMX_IN OMX_U32 nSizeBytes)

{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef= NULL;
    AACENC_COMPONENT_PRIVATE *pComponentPrivate= NULL;

    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;
    OMX_CONF_CHECK_CMD(hComponent,pBuffer,1); 

    AACENC_DPRINT("%d :: AACENC: Entering AllocateBuffer\n", __LINE__);
    AACENC_DPRINT("%d :: AACENC: pBuffer = %p\n", __LINE__,pBuffer);
    AACENC_DPRINT("AACENC: nPortIndex = %d - %p \n",(int)nPortIndex,&nPortIndex);
    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)
            (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pPortDef = ((AACENC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[nPortIndex];

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedBuffer(pComponentPrivate->pPERF,(*pBuffer)->pBuffer,nSizeBytes,PERF_ModuleMemory);
#endif

    AACENC_DPRINT("%d :: AACENC: pPortDef = %p\n", __LINE__, pPortDef);
    AACENC_DPRINT("%d :: AACENC: pPortDef->bEnabled = %d\n", __LINE__, pPortDef->bEnabled);
    while (1) 
    {
        if(pPortDef->bEnabled) 
        {
            break;
        }
        pComponentPrivate->AlloBuf_waitingsignal = 1;  

#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex); 
        pthread_cond_wait(&pComponentPrivate->AlloBuf_threshold, &pComponentPrivate->AlloBuf_mutex);
        pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
#else
        OMX_WaitForEvent(&(pComponentPrivate->AlloBuf_event));
#endif
        break;

    }

    OMX_MALLOC_STRUCT(pBufferHeader, OMX_BUFFERHEADERTYPE);
    AACENC_DPRINT("AACENC: pBufferHeader %p\n",pBufferHeader);

    OMX_MALLOC_STRUCT_SIZE(pBufferHeader->pBuffer, nSizeBytes + 256, OMX_U8);

    pBufferHeader->pBuffer += 128;
    AACENC_DPRINT("AACENC: pBufferHeader->pbuffer %p to %p \n",pBufferHeader->pBuffer,(pBufferHeader->pBuffer + sizeof(pBufferHeader->pBuffer)) );

    if (nPortIndex == INPUT_PORT) 
    {
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1;
        pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
        AACENC_DPRINT("%d :: AACENC: pComponentPrivate->pInputBufferList->pBufHdr[%d] = %p\n",__LINE__, pComponentPrivate->pInputBufferList->numBuffers,pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers]);
        pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;
        pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 1;
        AACENC_DPRINT("%d :: AACENC: pComponentPrivate->pInputBufferList->numBuffers = %d \n",__LINE__, pComponentPrivate->pInputBufferList->numBuffers);
        AACENC_DPRINT("%d :: AACENC: pPortDef->nBufferCountMin = %ld \n",__LINE__, pPortDef->nBufferCountMin);
        if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) 
        {
            pPortDef->bPopulated = OMX_TRUE;
        }
        AACENC_DPRINT("%d :: AACENC: INPUT PORT - pPortDef->bPopulated = %d\n",__LINE__, pPortDef->bPopulated);
    }
    else if (nPortIndex == OUTPUT_PORT) 
    {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex;
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
        AACENC_DPRINT("%d :: AACENC: pComponentPrivate->pOutputBufferList->pBufHdr[%d] = %p\n",__LINE__, pComponentPrivate->pOutputBufferList->numBuffers,pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers]);
        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 1;
        AACENC_DPRINT("%d :: AACENC: pComponentPrivate->pOutputBufferList->numBuffers = %d \n",__LINE__, pComponentPrivate->pOutputBufferList->numBuffers);
        AACENC_DPRINT("%d :: AACENC: pPortDef->nBufferCountMin = %ld \n",__LINE__, pPortDef->nBufferCountMin);
        if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) 
        {
            pPortDef->bPopulated = OMX_TRUE;
        }
        AACENC_DPRINT("%d :: AACENC: OUTPUT PORT - pPortDef->bPopulated = %d\n",__LINE__, pPortDef->bPopulated);
    }
    else 
    {
        eError = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

  if((pComponentPrivate->pPortDef[OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[INPUT_PORT]->bEnabled) &&
       (pComponentPrivate->InLoaded_readytoidle))
    {
        pComponentPrivate->InLoaded_readytoidle = 0;                  
#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->InLoaded_mutex);
        pthread_cond_signal(&pComponentPrivate->InLoaded_threshold);
        pthread_mutex_unlock(&pComponentPrivate->InLoaded_mutex);
#else
        OMX_SignalEvent(&(pComponentPrivate->InLoaded_event));
#endif
    }

    pBufferHeader->pAppPrivate = pAppPrivate;
    pBufferHeader->pPlatformPrivate = pComponentPrivate;
    pBufferHeader->nAllocLen = nSizeBytes;
    pBufferHeader->nVersion.s.nVersionMajor = AACENC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = AACENC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *pBuffer = pBufferHeader;
    if (pComponentPrivate->bEnableCommandPending && pPortDef->bPopulated) 
    {
        SendCommand (pComponentPrivate->pHandle,OMX_CommandPortEnable,pComponentPrivate->nEnableCommandParam,NULL);
    }

#ifdef __PERF_INSTRUMENTATION__
        PERF_ReceivedBuffer(pComponentPrivate->pPERF,(*pBuffer)->pBuffer, nSizeBytes,PERF_ModuleMemory);
#endif

EXIT:
    AACENC_DPRINT("%d :: AACENC:  AllocateBuffer returning eError =  %d\n",__LINE__,eError);

    AACENC_DPRINT("AACENC: pBufferHeader = %p\n",pBufferHeader);
    AACENC_DPRINT("AACENC: pBufferHeader->pBuffer = %p\n",pBufferHeader->pBuffer);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  FreeBuffer()

  * @param hComponent   handle for this instance of the component
  * @param pCallBacks   application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE FreeBuffer(OMX_IN  OMX_HANDLETYPE hComponent,
                                OMX_IN  OMX_U32 nPortIndex,
                                OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONF_CHECK_CMD(hComponent,1,pBuffer);
    AACENC_COMPONENT_PRIVATE * pComponentPrivate = NULL;
    OMX_BUFFERHEADERTYPE* buff = NULL;
    OMX_U8* tempBuff = NULL;
    int i =0;
    int inputIndex = -1;
    int outputIndex = -1;
    OMX_COMPONENTTYPE *pHandle = NULL;

    AACENC_DPRINT("%d :: AACENC: FreeBuffer\n", __LINE__);
    AACENC_DPRINT("%d :: AACENC: pBuffer = %p\n",__LINE__,pBuffer);
    AACENC_DPRINT("%d :: AACENC: pBuffer->pBuffer = %p\n",__LINE__,pBuffer->pBuffer);
    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    AACENC_DPRINT("%d :: AACENC: pComponentPrivate = %p\n", __LINE__,pComponentPrivate);
    for (i=0; i < MAX_NUM_OF_BUFS; i++) 
    {
        buff = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if (buff == pBuffer) 
        {
            AACENC_DPRINT("%d :: AACENC: Found matching input buffer\n",__LINE__);
            AACENC_DPRINT("%d :: AACENC: buff = %p\n",__LINE__,buff);
            AACENC_DPRINT("%d :: AACENC: pBuffer = %p\n",__LINE__,pBuffer);
            inputIndex = i;
            break;
        }
        else 
        {
            AACENC_DPRINT("%d :: AACENC: This is not a match\n",__LINE__);
            AACENC_DPRINT("%d :: AACENC: buff = %p\n",__LINE__,buff);
            AACENC_DPRINT("%d :: AACENC: pBuffer = %p\n",__LINE__,pBuffer);
        }
    }

    for (i=0; i < MAX_NUM_OF_BUFS; i++) 
    {
        buff = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if (buff == pBuffer) 
        {
            AACENC_DPRINT("%d :: AACENC: Found matching output buffer\n",__LINE__);
            AACENC_DPRINT("%d :: AACENC: buff = %p\n",__LINE__,buff);
            AACENC_DPRINT("%d :: AACENC: pBuffer = %p\n",__LINE__,pBuffer);
            outputIndex = i;
            break;
        }
        else 
        {
            AACENC_DPRINT("%d :: AACENC: This is not a match\n",__LINE__);
            AACENC_DPRINT("%d :: AACENC: buff = %p\n",__LINE__,buff);
            AACENC_DPRINT("%d :: AACENC: pBuffer = %p\n",__LINE__,pBuffer);
        }
    }


    if (inputIndex != -1) 
    {
        if (pComponentPrivate->pInputBufferList->bufferOwner[inputIndex] == 1) 
        {
            tempBuff = pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer;
            if (tempBuff != 0)
            {
                tempBuff -= 128;
            }
            OMX_MEMFREE_STRUCT(tempBuff);
        }
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingBuffer(pComponentPrivate->pPERF,
                               pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer, 
                               pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->nAllocLen,
                               PERF_ModuleMemory );

#endif
        
        AACENC_DPRINT("%d :: AACENC: [FREE] %p\n",__LINE__,pComponentPrivate->pBufHeader[INPUT_PORT]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);
        pComponentPrivate->pInputBufferList->numBuffers--;

        AACENC_DPRINT("%d :: AACENC: pComponentPrivate->pInputBufferList->numBuffers = %d \n",__LINE__,pComponentPrivate->pInputBufferList->numBuffers);
        AACENC_DPRINT("%d :: AACENC: pComponentPrivate->pPortDef[INPUT_PORT]->nBufferCountMin = %ld \n",__LINE__,pComponentPrivate->pPortDef[INPUT_PORT]->nBufferCountMin);
        if (pComponentPrivate->pInputBufferList->numBuffers < pComponentPrivate->pPortDef[INPUT_PORT]->nBufferCountActual) 
        {

            pComponentPrivate->pPortDef[INPUT_PORT]->bPopulated = OMX_FALSE;
        }
        
        if(pComponentPrivate->pPortDef[INPUT_PORT]->bEnabled &&
            pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
            (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause)) 
        {
            AACENC_DPRINT("%d :: AACENC: PortUnpopulated\n",__LINE__);
            pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError, 
                                                    OMX_ErrorPortUnpopulated,
                                                    OMX_TI_ErrorMinor, 
                                                    "Input Port Unpopulated");
        }
    }
    else if (outputIndex != -1) 
    {
        if (pComponentPrivate->pOutputBufferList->bufferOwner[outputIndex] == 1) 
        {
            tempBuff = pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer;
            if (tempBuff != 0)
            {
               tempBuff -= 128;
            }
            OMX_MEMFREE_STRUCT(tempBuff);
        }
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingBuffer(pComponentPrivate->pPERF,
                               pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer, 
                               pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->nAllocLen,
                               PERF_ModuleMemory);

#endif
        
        AACENC_DPRINT("%d :: AACENC: [FREE] %p\n",__LINE__,pComponentPrivate->pBufHeader[OUTPUT_PORT]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);
        pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex] = NULL;
        pComponentPrivate->pOutputBufferList->numBuffers--;

        AACENC_DPRINT("%d :: AACENC: pComponentPrivate->pOutputBufferList->numBuffers = %d \n",__LINE__,pComponentPrivate->pOutputBufferList->numBuffers);
        AACENC_DPRINT("%d :: AACENC: pComponentPrivate->pPortDef[OUTPUT_PORT]->nBufferCountMin = %ld \n",__LINE__,pComponentPrivate->pPortDef[OUTPUT_PORT]->nBufferCountMin);
        if (pComponentPrivate->pOutputBufferList->numBuffers <
            pComponentPrivate->pPortDef[OUTPUT_PORT]->nBufferCountActual) 
        {

            pComponentPrivate->pPortDef[OUTPUT_PORT]->bPopulated = OMX_FALSE;
        }
        if(pComponentPrivate->pPortDef[OUTPUT_PORT]->bEnabled &&
            pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
            (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause)) 
        {
            AACENC_DPRINT("%d :: AACENC: PortUnpopulated\n",__LINE__);
            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError, 
                                                    OMX_ErrorPortUnpopulated,
                                                    OMX_TI_ErrorMinor, 
                                                    "Output Port Unpopulated");
        }
    }
    else 
    {
        AACENC_DPRINT("%d :: Error: Returning OMX_ErrorBadParameter\n",__LINE__);
        eError = OMX_ErrorBadParameter;
    }
    if ((!pComponentPrivate->pInputBufferList->numBuffers &&
         !pComponentPrivate->pOutputBufferList->numBuffers) &&
         pComponentPrivate->InIdle_goingtoloaded)
    {
        pComponentPrivate->InIdle_goingtoloaded = 0;                  
#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->InIdle_mutex);
        pthread_cond_signal(&pComponentPrivate->InIdle_threshold);
        pthread_mutex_unlock(&pComponentPrivate->InIdle_mutex);
#else
        OMX_SignalEvent(&(pComponentPrivate->InIdle_event));
#endif
    }

    if (pComponentPrivate->bDisableCommandPending && (pComponentPrivate->pInputBufferList->numBuffers + pComponentPrivate->pOutputBufferList->numBuffers == 0)) 
    {
        SendCommand (pComponentPrivate->pHandle,OMX_CommandPortDisable,pComponentPrivate->bDisableCommandParam,NULL);
    }

    AACENC_DPRINT ("%d :: AACENC: Exiting FreeBuffer\n", __LINE__);
EXIT:
    return eError;
}

/* ================================================================================= */
/**
* @fn UseBuffer() description for UseBuffer  
UseBuffer().  
Called by the OMX IL client to pass a buffer to be used.   
*
*  @see         OMX_Core.h
*/
/* ================================================================================ */
static OMX_ERRORTYPE UseBuffer (OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes,
            OMX_IN OMX_U8* pBuffer)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef= NULL;
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;
    OMX_CONF_CHECK_CMD(hComponent,ppBufferHdr,pBuffer);

    AACENC_DPRINT ("%d :: AACENC: Entering UseBuffer\n", __LINE__);
    AACENC_DPRINT ("%d :: AACENC: pBuffer = %p\n", __LINE__,pBuffer);
    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)
            (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
    
#endif

#ifdef __PERF_INSTRUMENTATION__
        PERF_ReceivedBuffer(pComponentPrivate->pPERF,pBuffer, nSizeBytes,
                            PERF_ModuleHLMM);
#endif

    pPortDef = ((AACENC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[nPortIndex];
    AACENC_DPRINT ("%d :: AACENC: pPortDef = %p\n", __LINE__,pPortDef);
    AACENC_DPRINT ("%d :: AACENC: pPortDef->bEnabled = %d\n", __LINE__,pPortDef->bEnabled);

    if(!pPortDef->bEnabled) 
    {
        AACENC_EPRINT ("%d :: Error: In AllocateBuffer\n", __LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    AACENC_DPRINT("AACENC: nSizeBytes = %ld\n",nSizeBytes);
    AACENC_DPRINT("AACENC: pPortDef->nBufferSize = %ld\n",pPortDef->nBufferSize);
    AACENC_DPRINT("AACENC: pPortDef->bPopulated = %d\n",pPortDef->bPopulated);
    if(nSizeBytes != pPortDef->nBufferSize || pPortDef->bPopulated) 
    {
        AACENC_DPRINT("%d :: Error: In AllocateBuffer\n", __LINE__);
        AACENC_EPRINT("%d :: Error: About to return OMX_ErrorBadParameter\n",__LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    OMX_MALLOC_STRUCT(pBufferHeader, OMX_BUFFERHEADERTYPE);

    AACENC_DPRINT("%d :: AACENC: [ALLOC] %p\n",__LINE__,pBufferHeader);

    if (nPortIndex == OUTPUT_PORT) 
    {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex; 
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 0;
        if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) 
        {
            pPortDef->bPopulated = OMX_TRUE;
        }
    }
    else 
    {
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1; 
        pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;
        pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 0;
        if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) 
        {
            pPortDef->bPopulated = OMX_TRUE;
        }
    }

    if((pComponentPrivate->pPortDef[OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[INPUT_PORT]->bEnabled) &&
       (pComponentPrivate->InLoaded_readytoidle))
    {
       pComponentPrivate->InLoaded_readytoidle = 0;                  
#ifndef UNDER_CE
       pthread_mutex_lock(&pComponentPrivate->InLoaded_mutex);
       pthread_cond_signal(&pComponentPrivate->InLoaded_threshold);
       pthread_mutex_unlock(&pComponentPrivate->InLoaded_mutex);
#else
       OMX_SignalEvent(&(pComponentPrivate->InLoaded_event));
#endif
    }

    pBufferHeader->pAppPrivate = pAppPrivate;
    pBufferHeader->pPlatformPrivate = pComponentPrivate;
    pBufferHeader->nAllocLen = nSizeBytes;
    pBufferHeader->nVersion.s.nVersionMajor = AACENC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = AACENC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->pBuffer = pBuffer;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *ppBufferHdr = pBufferHeader;
    AACENC_DPRINT("%d :: AACENC: pBufferHeader = %p\n",__LINE__,pBufferHeader);

    if (pComponentPrivate->bEnableCommandPending) 
    {
        SendCommand (pComponentPrivate->pHandle,OMX_CommandPortEnable,pComponentPrivate->nEnableCommandParam,NULL);
    }

EXIT:
    AACENC_DPRINT("AACENC: [UseBuffer] pBufferHeader = %p\n",pBufferHeader);
    AACENC_DPRINT("AACENC: [UseBuffer] pBufferHeader->pBuffer = %p\n",pBufferHeader->pBuffer);

    return eError;
}

/* ================================================================================= */
/**
* @fn GetExtensionIndex() description for GetExtensionIndex  
GetExtensionIndex().  
Returns index for vendor specific settings.   
*
*  @see         OMX_Core.h
*/
/* ================================================================================ */
static OMX_ERRORTYPE GetExtensionIndex(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_STRING cParameterName,
            OMX_OUT OMX_INDEXTYPE* pIndexType) 
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    AACENC_DPRINT("AACENC: GetExtensionIndex\n");
    if (!(strcmp(cParameterName,"OMX.TI.index.config.aacencHeaderInfo"))) 
    {
        *pIndexType = OMX_IndexCustomAacEncHeaderInfoConfig;
        AACENC_DPRINT("OMX_IndexCustomAacEncHeaderInfoConfig\n");
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.aacencstreamIDinfo"))) 
    {
        *pIndexType = OMX_IndexCustomAacEncStreamIDConfig;
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.aac.datapath"))) 
    {
        *pIndexType = OMX_IndexCustomAacEncDataPath;
    }
    else if (!(strcmp(cParameterName,"OMX.TI.index.config.aacencframesPerOutBuf")))
    {
        *pIndexType =OMX_IndexCustomAacEncFramesPerOutBuf;
    }
    else 
    {
        eError = OMX_ErrorBadParameter;
    }

    AACENC_DPRINT("AACENC: Exiting GetExtensionIndex\n");

    return eError;
}

/* ================================================================================= */
/**
* @fn ComponentRoleEnum() description for ComponentRoleEnum()  

Returns the role at the given index
*
*  @see         OMX_Core.h
*/
/* ================================================================================ */
static OMX_ERRORTYPE ComponentRoleEnum(
      OMX_IN OMX_HANDLETYPE hComponent,
      OMX_OUT OMX_U8 *cRole,
      OMX_IN OMX_U32 nIndex)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    if(nIndex == 0)
    {
      memcpy(cRole, &pComponentPrivate->componentRole.cRole, sizeof(OMX_U8) * OMX_MAX_STRINGNAME_SIZE); 
    }
    else 
    {
      eError = OMX_ErrorNoMore;
      goto EXIT;
    }

EXIT:  
    return eError;
};

#ifdef UNDER_CE
/* ================================================================================= */
/**
* @fns Sleep replace for WIN CE
*/
/* ================================================================================ */
int OMX_CreateEvent(OMX_Event *event){
    int ret = OMX_ErrorNone;   
    HANDLE createdEvent = NULL;
    if(event == NULL){
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    event->event  = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(event->event == NULL)
        ret = (int)GetLastError();
EXIT:
    return ret;
}

int OMX_SignalEvent(OMX_Event *event){
     int ret = OMX_ErrorNone;     
     if(event == NULL){
        ret = OMX_ErrorBadParameter;
        goto EXIT;
     }     
     SetEvent(event->event);
     ret = (int)GetLastError();
EXIT:
    return ret;
}

int OMX_WaitForEvent(OMX_Event *event) {
     int ret = OMX_ErrorNone;         
     if(event == NULL){
        ret = OMX_ErrorBadParameter;
        goto EXIT;
     }     
     WaitForSingleObject(event->event, INFINITE);    
     ret = (int)GetLastError();
EXIT:
     return ret;
}

int OMX_DestroyEvent(OMX_Event *event) {
     int ret = OMX_ErrorNone;
     if(event == NULL){
        ret = OMX_ErrorBadParameter;
        goto EXIT;
     }  
     CloseHandle(event->event);
EXIT:    
     return ret;
}
#endif
