
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
/* =============================================================================
*             Texas Instruments OMAP (TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* =========================================================================== */
/**
* @file OMX_AmrEnc_Utils.c
*
* This file implements NBAMR Encoder Component Specific APIs and its functionality
* that is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
*
* @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\nbamr_enc\src
*
* @rev  1.0
*/
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! ===================================
*! 21-sept-2006 bk: updated review findings for alpha release
*! 24-Aug-2006 bk: Khronos OpenMAX (TM) 1.0 Conformance tests some more
*! 18-July-2006 bk: Khronos OpenMAX (TM) 1.0 Conformance tests validated for few cases
*! 21-Jun-2006 bk: Khronos OpenMAX (TM) 1.0 migration done
*! 22-May-2006 bk: DASF recording quality improved
*! 19-Apr-2006 bk: DASF recording speed issue resloved
*! 23-Feb-2006 bk: DASF functionality added
*! 18-Jan-2006 bk: Repated recording issue fixed and LCML changes taken care
*! 14-Dec-2005 bk: Initial Version
*! 16-Nov-2005 bk: Initial Version
*! 23-Sept-2005 bk: Initial Version
*! 10-Sept-2005 bk: Initial Version
*! 10-Sept-2005 bk:
*! This is newest file
* =========================================================================== */

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
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <malloc.h>
#include <memory.h>
#include <fcntl.h>
#include <errno.h>
#endif

#include <dbapi.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/*-------program files ----------------------------------------*/
#include "OMX_AmrEnc_Utils.h"
#include "amrencsocket_ti.h"
#include <encode_common_ti.h>
#include "OMX_AmrEnc_ComponentThread.h"
#include "usn.h"
#include "LCML_DspCodec.h"

#ifdef RESOURCE_MANAGER_ENABLED
    #include <ResourceManagerProxyAPI.h>
#endif

#ifdef NBAMRENC_DEBUGMEM
  void *arr[500];
  int lines[500];
  int bytes[500];
  char file[500][50];


void * mymalloc(int line, char *s, int size);
int mynewfree(void *dp, int line, char *s);

#define newmalloc(x) mymalloc(__LINE__,__FILE__,x)
#define newfree(z) myfree(z,__LINE__,__FILE__)

void * mymalloc(int line, char *s, int size)
{
   void *p;    
   int e=0;
/*   p = malloc(size);*/
   p = calloc(1,size);
   if(p==NULL){
       AMRENC_DPRINT("Memory not available\n");
       exit(1);
       }
   else{
         while((lines[e]!=0)&& (e<500) ){
              e++;
         }
         arr[e]=p;
         lines[e]=line;
         bytes[e]=size;
         strcpy(file[e],s);
         AMRENC_DPRINT("Allocating %d bytes on address %p, line %d file %s\n", size, p, line, s);
         return p;
   }
}

int myfree(void *dp, int line, char *s){
    int q;
    if(dp==NULL){
                 AMRENC_DPRINT("NULL can't be deleted\n");
                 return 0;
    }
    for(q=0;q<500;q++){
        if(arr[q]==dp){
           AMRENC_DPRINT("Deleting %d bytes on address %p, line %d file %s\n", bytes[q],dp, line, s);
           free(dp);
           dp = NULL;
           lines[q]=0;
           strcpy(file[q],"");
           break;
        }            
     }    
     if(500==q)
         AMRENC_DPRINT("\n\nPointer not found. Line:%d    File%s!!\n\n",line, s);
}
#else
#define newmalloc(x) malloc(x)
#define newfree(z) free(z)
#endif

/* ========================================================================== */
/**
* @NBAMRENC_FillLCMLInitParams () This function is used by the component thread to
* fill the all of its initialization parameters, buffer deatils  etc
* to LCML structure,
*
* @param pComponent  handle for this instance of the component
* @param plcml_Init  pointer to LCML structure to be filled
*
* @pre
*
* @post
*
* @return none
*/
/* ========================================================================== */

OMX_ERRORTYPE NBAMRENC_FillLCMLInitParams(OMX_HANDLETYPE pComponent,
                                  LCML_DSP *plcml_Init, OMX_U16 arr[])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf,nIpBufSize,nOpBuf,nOpBufSize;
    OMX_BUFFERHEADERTYPE *pTemp;
    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    AMRENC_COMPONENT_PRIVATE *pComponentPrivate = pHandle->pComponentPrivate;
    NBAMRENC_LCML_BUFHEADERTYPE *pTemp_lcml = NULL;
    OMX_U16 i;
    OMX_U32 size_lcml;
    OMX_U8 *pBufferParamTemp;
    char *pTemp_char = NULL;
    AMRENC_DPRINT("%d :: Entering NBAMRENC_FillLCMLInitParams\n",__LINE__);
    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nIpBufSize = pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->nBufferSize;
    pComponentPrivate->nRuntimeInputBuffers = nIpBuf;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nOpBufSize = pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->nBufferSize;
    pComponentPrivate->nRuntimeOutputBuffers = nOpBuf;
    AMRENC_DPRINT("%d :: ------ Buffer Details -----------\n",__LINE__);
    AMRENC_DPRINT("%d :: Input  Buffer Count = %ld\n",__LINE__,nIpBuf);
    AMRENC_DPRINT("%d :: Input  Buffer Size = %ld\n",__LINE__,nIpBufSize);
    AMRENC_DPRINT("%d :: Output Buffer Count = %ld\n",__LINE__,nOpBuf);
    AMRENC_DPRINT("%d :: Output Buffer Size = %ld\n",__LINE__,nOpBufSize);
    AMRENC_DPRINT("%d :: ------ Buffer Details ------------\n",__LINE__);
    /* Fill Input Buffers Info for LCML */
    plcml_Init->In_BufInfo.nBuffers = nIpBuf;
    plcml_Init->In_BufInfo.nSize = nIpBufSize;
    plcml_Init->In_BufInfo.DataTrMethod = DMM_METHOD;

    /* Fill Output Buffers Info for LCML */
    plcml_Init->Out_BufInfo.nBuffers = nOpBuf;
    plcml_Init->Out_BufInfo.nSize = nOpBufSize;
    plcml_Init->Out_BufInfo.DataTrMethod = DMM_METHOD;

    /*Copy the node information*/
    plcml_Init->NodeInfo.nNumOfDLLs = 3;

    plcml_Init->NodeInfo.AllUUIDs[0].uuid = &AMRENCSOCKET_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[0].DllName,NBAMRENC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;

    plcml_Init->NodeInfo.AllUUIDs[1].uuid = &AMRENCSOCKET_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[1].DllName,NBAMRENC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;

    plcml_Init->NodeInfo.AllUUIDs[2].uuid = &USN_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[2].DllName,NBAMRENC_USN_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;
    plcml_Init->DeviceInfo.TypeofDevice = 0;

    if(pComponentPrivate->dasfMode == 1) {
        AMRENC_DPRINT("%d :: Codec is configuring to DASF mode\n",__LINE__);
        NBAMRENC_OMX_MALLOC(pComponentPrivate->strmAttr, LCML_STRMATTR);
        pComponentPrivate->strmAttr->uSegid = NBAMRENC_DEFAULT_SEGMENT;
        pComponentPrivate->strmAttr->uAlignment = 0;
        pComponentPrivate->strmAttr->uTimeout = NBAMRENC_SN_TIMEOUT;
        pComponentPrivate->strmAttr->uBufsize = NBAMRENC_INPUT_BUFFER_SIZE_DASF;
        pComponentPrivate->strmAttr->uNumBufs = NBAMRENC_NUM_INPUT_BUFFERS_DASF;
        pComponentPrivate->strmAttr->lMode = STRMMODE_PROCCOPY;
        /* Device is Configuring to DASF Mode */
        plcml_Init->DeviceInfo.TypeofDevice = 1;
        /* Device is Configuring to Record Mode */
        plcml_Init->DeviceInfo.TypeofRender = 1;

        if(pComponentPrivate->acdnMode == 1) {
            /* ACDN mode */
            plcml_Init->DeviceInfo.AllUUIDs[0].uuid = &ACDN_TI_UUID;
        }
        else {
            /* DASF/TeeDN mode */
                if(pComponentPrivate->teeMode!= TEEMODE_NONE) {
                    plcml_Init->DeviceInfo.AllUUIDs[0].uuid = &TEEDN_TI_UUID;
                }
                else {
                    plcml_Init->DeviceInfo.AllUUIDs[0].uuid = &DCTN_TI_UUID;
                }
        }
        plcml_Init->DeviceInfo.DspStream = pComponentPrivate->strmAttr;
    }

    /*copy the other information*/
    plcml_Init->SegID = NBAMRENC_DEFAULT_SEGMENT;
    plcml_Init->Timeout = NBAMRENC_SN_TIMEOUT;
    plcml_Init->Alignment = 0;
    plcml_Init->Priority = NBAMRENC_SN_PRIORITY;
    plcml_Init->ProfileID = -1;

    /* Setting Creat Phase Parameters here */
    arr[0] = NBAMRENC_STREAM_COUNT;
    arr[1] = NBAMRENC_INPUT_PORT;

    if(pComponentPrivate->dasfMode == 1) {
        arr[2] = NBAMRENC_INSTRM;
        arr[3] = NBAMRENC_NUM_INPUT_BUFFERS_DASF;
    }
    else {
        arr[2] = NBAMRENC_DMM;
        if (pComponentPrivate->pInputBufferList->numBuffers) {
            arr[3] = (OMX_U16) pComponentPrivate->pInputBufferList->numBuffers;
        }
        else {
            arr[3] = 1;
        }
    }

    arr[4] = NBAMRENC_OUTPUT_PORT;
    arr[5] = NBAMRENC_DMM;
    if (pComponentPrivate->pOutputBufferList->numBuffers) {
        arr[6] = (OMX_U16) pComponentPrivate->pOutputBufferList->numBuffers;
    }
    else {
        arr[6] = 1;
    }

    if (pComponentPrivate->efrMode == 1) {
        AMRENC_DPRINT("%d :: Codec is configuring to EFR mode\n",__LINE__);
        arr[7] = NBAMRENC_EFR;
    }
    else {
        AMRENC_DPRINT("%d :: Codec is configuring to NBAMR mode\n",__LINE__);
        arr[7] = NBAMRENC_NBAMR;
    }

    if(pComponentPrivate->frameMode == NBAMRENC_MIMEMODE ) {
        AMRENC_DPRINT("%d :: Codec is configuring MIME mode\n",__LINE__);
        arr[8] = NBAMRENC_MIMEMODE;
    }
    else if(pComponentPrivate->frameMode == NBAMRENC_IF2 ){
        AMRENC_DPRINT("%d :: Codec is configuring IF2 mode\n",__LINE__);
        arr[8] = NBAMRENC_IF2;                            
        }
    else {
        AMRENC_DPRINT("%d :: Codec is configuring FORMAT CONFORMANCE mode\n",__LINE__);
        arr[8] = NBAMRENC_FORMATCONFORMANCE;
    }
    arr[9] = END_OF_CR_PHASE_ARGS;

    plcml_Init->pCrPhArgs = arr;

    /* Allocate memory for all input buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nIpBuf * sizeof(NBAMRENC_LCML_BUFHEADERTYPE);
    
    NBAMRENC_OMX_MALLOC_SIZE(pTemp_lcml, size_lcml,NBAMRENC_LCML_BUFHEADERTYPE);

    pComponentPrivate->pLcmlBufHeader[NBAMRENC_INPUT_PORT] = pTemp_lcml;
    for (i=0; i<nIpBuf; i++) {
        AMRENC_DPRINT("%d :: INPUT--------- Inside Ip Loop\n",__LINE__);
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = NBAMRENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = NBAMRENC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = NBAMRENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        AMRENC_DPRINT("%d :: pTemp_lcml->buffer->pBuffer = %p \n",__LINE__,pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirInput;

        
        NBAMRENC_OMX_MALLOC_SIZE(pBufferParamTemp, sizeof(NBAMRENC_ParamStruct) + DSP_CACHE_ALIGNMENT,OMX_U8);

        pTemp_lcml->pBufferParam =  (NBAMRENC_ParamStruct*)(pBufferParamTemp + EXTRA_BYTES);

        pTemp_lcml->pBufferParam->usNbFrames=0;
        pTemp_lcml->pBufferParam->pParamElem=NULL;
        pTemp_lcml->pFrameParam=NULL;
        
        NBAMRENC_OMX_MALLOC(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);

        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = NBAMRENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nOpBuf * sizeof(NBAMRENC_LCML_BUFHEADERTYPE);

    NBAMRENC_OMX_MALLOC_SIZE(pTemp_lcml, size_lcml,NBAMRENC_LCML_BUFHEADERTYPE);
    AMRENC_MEMPRINT("%d :: [ALLOC]  %p\n",__LINE__,pTemp_lcml);

    pComponentPrivate->pLcmlBufHeader[NBAMRENC_OUTPUT_PORT] = pTemp_lcml;
    for (i=0; i<nOpBuf; i++) {
        AMRENC_DPRINT("%d :: OUTPUT--------- Inside Op Loop\n",__LINE__);
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nFilledLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = NBAMRENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = NBAMRENC_MINOR_VER;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = NBAMRENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        AMRENC_DPRINT("%d :: pTemp_lcml->buffer->pBuffer = %p \n",__LINE__,pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirOutput;
        
        NBAMRENC_OMX_MALLOC_SIZE(pTemp_lcml->pBufferParam,
                                (sizeof(NBAMRENC_ParamStruct)+DSP_CACHE_ALIGNMENT),
                                NBAMRENC_ParamStruct); 

        pTemp_char = (char*)pTemp_lcml->pBufferParam;
        pTemp_char += EXTRA_BYTES;
        pTemp_lcml->pBufferParam = (NBAMRENC_ParamStruct*)pTemp_char;
        
        pTemp_lcml->pBufferParam->usNbFrames=0;
        pTemp_lcml->pBufferParam->pParamElem=NULL;
        pTemp_lcml->pFrameParam=NULL;

        NBAMRENC_OMX_MALLOC(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);

        pTemp->nFlags = NBAMRENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }
#ifdef __PERF_INSTRUMENTATION__
        pComponentPrivate->nLcml_nCntIp = 0;
        pComponentPrivate->nLcml_nCntOpReceived = 0;
#endif

    pComponentPrivate->bInitParamsInitialized = 1;
EXIT:
    AMRENC_DPRINT("%d :: Exiting NBAMRENC_FillLCMLInitParams\n",__LINE__);
    AMRENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ========================================================================== */
/**
* @NBAMRENC_StartComponentThread() This function is called by the component to create
* the component thread, command pipes, data pipes and LCML Pipes.
*
* @param pComponent  handle for this instance of the component
*
* @pre
*
* @post
*
* @return none
*/
/* ========================================================================== */

OMX_ERRORTYPE NBAMRENC_StartComponentThread(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AMRENC_COMPONENT_PRIVATE *pComponentPrivate =
                        (AMRENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
#ifdef UNDER_CE
    pthread_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.__inheritsched = PTHREAD_EXPLICIT_SCHED;
    attr.__schedparam.__sched_priority = OMX_AUDIO_ENCODER_THREAD_PRIORITY;
#endif

    AMRENC_DPRINT ("%d :: Entering  NBAMRENC_StartComponentThread\n", __LINE__);
    /* Initialize all the variables*/
    pComponentPrivate->bIsStopping = 0;
    pComponentPrivate->bIsThreadstop = 0;
    pComponentPrivate->lcml_nOpBuf = 0;
    pComponentPrivate->lcml_nIpBuf = 0;
    pComponentPrivate->app_nBuf = 0;
    pComponentPrivate->num_Op_Issued = 0;


    /* create the pipe used to send buffers to the thread */
    eError = pipe (pComponentPrivate->cmdDataPipe);
    if (eError) {
       eError = OMX_ErrorInsufficientResources;
       AMRENC_EPRINT("%d :: Error while creating cmdDataPipe\n",__LINE__);
       goto EXIT;
    }
    /* create the pipe used to send buffers to the thread */
    eError = pipe (pComponentPrivate->dataPipe);
    if (eError) {
       eError = OMX_ErrorInsufficientResources;
       AMRENC_EPRINT("%d :: Error while creating dataPipe\n",__LINE__);
       goto EXIT;
    }

    /* create the pipe used to send commands to the thread */
    eError = pipe (pComponentPrivate->cmdPipe);
    if (eError) {
       eError = OMX_ErrorInsufficientResources;
       AMRENC_EPRINT("%d :: Error while creating cmdPipe\n",__LINE__);
       goto EXIT;
    }

    /* Create the Component Thread */
#ifdef UNDER_CE
    eError = pthread_create (&(pComponentPrivate->ComponentThread), &attr, NBAMRENC_CompThread, pComponentPrivate);
#else
    eError = pthread_create (&(pComponentPrivate->ComponentThread), NULL, NBAMRENC_CompThread, pComponentPrivate);
#endif
    if (eError || !pComponentPrivate->ComponentThread) {
       eError = OMX_ErrorInsufficientResources;
       goto EXIT;
    }

    pComponentPrivate->bCompThreadStarted = 1;
EXIT:
    AMRENC_DPRINT("%d :: Exiting NBAMRENC_StartComponentThread\n", __LINE__);
    AMRENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ========================================================================== */
/**
* @NBAMRENC_FreeCompResources() This function is called by the component during
* de-init , to newfree Command pipe, data pipe & LCML pipe.
*
* @param pComponent  handle for this instance of the component
*
* @pre
*
* @post
*
* @return none
*/
/* ========================================================================== */

OMX_ERRORTYPE NBAMRENC_FreeCompResources(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AMRENC_COMPONENT_PRIVATE *pComponentPrivate = (AMRENC_COMPONENT_PRIVATE *)
                                                     pHandle->pComponentPrivate;
    AMRENC_DPRINT("%d :: Entering NBAMRENC_FreeCompResources\n",__LINE__);

    if (pComponentPrivate->bCompThreadStarted) {
        OMX_NBCLOSE_PIPE(pComponentPrivate->dataPipe[0],err);
        OMX_NBCLOSE_PIPE(pComponentPrivate->dataPipe[1],err);
        OMX_NBCLOSE_PIPE(pComponentPrivate->cmdPipe[0],err);
        OMX_NBCLOSE_PIPE(pComponentPrivate->cmdPipe[1],err);
        OMX_NBCLOSE_PIPE(pComponentPrivate->cmdDataPipe[0],err);
        OMX_NBCLOSE_PIPE(pComponentPrivate->cmdDataPipe[1],err);
    }

    OMX_NBMEMFREE_STRUCT(pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]);
    OMX_NBMEMFREE_STRUCT(pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]);
    OMX_NBMEMFREE_STRUCT(pComponentPrivate->pcmParams);
    OMX_NBMEMFREE_STRUCT(pComponentPrivate->amrParams);

    OMX_NBMEMFREE_STRUCT(pComponentPrivate->pCompPort[NBAMRENC_INPUT_PORT]->pPortFormat);
    OMX_NBMEMFREE_STRUCT(pComponentPrivate->pCompPort[NBAMRENC_OUTPUT_PORT]->pPortFormat);
    OMX_NBMEMFREE_STRUCT(pComponentPrivate->pCompPort[NBAMRENC_INPUT_PORT]);
    OMX_NBMEMFREE_STRUCT(pComponentPrivate->pCompPort[NBAMRENC_OUTPUT_PORT]);

    OMX_NBMEMFREE_STRUCT(pComponentPrivate->sPortParam);
    OMX_NBMEMFREE_STRUCT(pComponentPrivate->sPriorityMgmt);
    OMX_NBMEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
    OMX_NBMEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);

EXIT:
    AMRENC_DPRINT("%d :: Exiting NBAMRENC_FreeCompResources()\n",__LINE__);
    AMRENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ========================================================================== */
/**
* @NBAMRENC_CleanupInitParams() This function is called by the component during
* de-init to newfree structues that are been allocated at intialization stage
*
* @param pComponent  handle for this instance of the component
*
* @pre
*
* @post
*
* @return none
*/
/* ========================================================================== */

OMX_ERRORTYPE NBAMRENC_CleanupInitParams(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0;
    OMX_U32 nOpBuf = 0;
    OMX_U16 i = 0;
    NBAMRENC_LCML_BUFHEADERTYPE *pTemp_lcml;
    OMX_U8* pAlgParmTemp;
    OMX_U8* pBufParmsTemp;
    char *pTemp = NULL;
    LCML_DSP_INTERFACE *pLcmlHandle;
    LCML_DSP_INTERFACE *pLcmlHandleAux;

    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AMRENC_COMPONENT_PRIVATE *pComponentPrivate = (AMRENC_COMPONENT_PRIVATE *)
                                                     pHandle->pComponentPrivate;
    AMRENC_DPRINT("%d :: Entering NBAMRENC_CleanupInitParams()\n", __LINE__);

    if(pComponentPrivate->dasfMode == 1) {

        OMX_NBMEMFREE_STRUCT(pComponentPrivate->strmAttr);
    }

    pAlgParmTemp = (OMX_U8*)pComponentPrivate->pAlgParam;
    if (pAlgParmTemp != NULL){
       pAlgParmTemp -= EXTRA_BYTES;}
    pComponentPrivate->pAlgParam = (NBAMRENC_TALGCtrl*)pAlgParmTemp;
    OMX_NBMEMFREE_STRUCT(pComponentPrivate->pAlgParam);

        pComponentPrivate->nHoldLength = 0;
    OMX_NBMEMFREE_STRUCT(pComponentPrivate->pHoldBuffer);
    OMX_NBMEMFREE_STRUCT(pComponentPrivate->pHoldBuffer2);
    OMX_NBMEMFREE_STRUCT(pComponentPrivate->iHoldBuffer);
    OMX_NBMEMFREE_STRUCT(pComponentPrivate->iMMFDataLastBuffer);

    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[NBAMRENC_INPUT_PORT];
    nIpBuf = pComponentPrivate->nRuntimeInputBuffers;

    for(i=0; i<nIpBuf; i++) {
        if(pTemp_lcml->pFrameParam!=NULL){
              pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate->pLcmlHandle;
              pLcmlHandleAux = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);
              OMX_DmmUnMap(pLcmlHandleAux->dspCodec->hProc,
                               (void*)pTemp_lcml->pBufferParam->pParamElem,
                               pTemp_lcml->pDmmBuf->pReserved);
                                                                       
              pBufParmsTemp = (OMX_U8*)pTemp_lcml->pFrameParam;
              pBufParmsTemp -= EXTRA_BYTES;
              OMX_NBMEMFREE_STRUCT(pBufParmsTemp);
              pBufParmsTemp = NULL;
              pTemp_lcml->pFrameParam = NULL;
        }

    pTemp = (char*)pTemp_lcml->pBufferParam;
    if (pTemp != NULL) {        
        pTemp -= EXTRA_BYTES;
    }
    pTemp_lcml->pBufferParam = (NBAMRENC_ParamStruct*)pTemp;
    OMX_NBMEMFREE_STRUCT(pTemp_lcml->pBufferParam);
    

        if(pTemp_lcml->pDmmBuf!=NULL){
             OMX_NBMEMFREE_STRUCT(pTemp_lcml->pDmmBuf);
             pTemp_lcml->pDmmBuf = NULL;
        }
        pTemp_lcml++;
    }


    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[NBAMRENC_OUTPUT_PORT];
    nOpBuf = pComponentPrivate->nRuntimeOutputBuffers;

    for(i=0; i<nOpBuf; i++) {
        if(pTemp_lcml->pFrameParam!=NULL){
              pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate->pLcmlHandle;
              pLcmlHandleAux = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);
#ifndef UNDER_CE
            OMX_DmmUnMap(pLcmlHandleAux->dspCodec->hProc,
                              (void*)pTemp_lcml->pBufferParam->pParamElem,
                               pTemp_lcml->pDmmBuf->pReserved);
#endif
              
              pBufParmsTemp = (OMX_U8*)pTemp_lcml->pFrameParam;
              pBufParmsTemp -= EXTRA_BYTES;
              OMX_NBMEMFREE_STRUCT(pBufParmsTemp);
              pBufParmsTemp = NULL;
              pTemp_lcml->pFrameParam = NULL;              
        }

pTemp = (char*)pTemp_lcml->pBufferParam;
    if (pTemp != NULL) {        
        pTemp -= EXTRA_BYTES;
    }
    pTemp_lcml->pBufferParam = (NBAMRENC_ParamStruct*)pTemp;
    OMX_NBMEMFREE_STRUCT(pTemp_lcml->pBufferParam);
    


        if(pTemp_lcml->pDmmBuf!=NULL){
             OMX_NBMEMFREE_STRUCT(pTemp_lcml->pDmmBuf);
             pTemp_lcml->pDmmBuf = NULL;
        }
        pTemp_lcml++;
    }


    OMX_NBMEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[NBAMRENC_INPUT_PORT]);
    OMX_NBMEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[NBAMRENC_OUTPUT_PORT]);

    AMRENC_DPRINT("%d :: Exiting NBAMRENC_CleanupInitParams()\n",__LINE__);
    AMRENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ========================================================================== */
/**
* @NBAMRENC_StopComponentThread() This function is called by the component during
* de-init to close component thread.
*
* @param pComponent  handle for this instance of the component
*
* @pre
*
* @post
*
* @return none
*/
/* ========================================================================== */

OMX_ERRORTYPE NBAMRENC_StopComponentThread(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE threadError = OMX_ErrorNone;
    int pthreadError = 0;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AMRENC_COMPONENT_PRIVATE *pComponentPrivate = (AMRENC_COMPONENT_PRIVATE *)
                                                     pHandle->pComponentPrivate;
    AMRENC_DPRINT("%d :: Entering NBAMRENC_StopComponentThread\n",__LINE__);
    pComponentPrivate->bIsThreadstop = 1;
    write (pComponentPrivate->cmdPipe[1], &pComponentPrivate->bIsThreadstop, sizeof(OMX_U16));    
    AMRENC_DPRINT("%d :: About to call pthread_join\n",__LINE__);
    pthreadError = pthread_join (pComponentPrivate->ComponentThread,
                                 (void*)&threadError);
    if (0 != pthreadError) {
        eError = OMX_ErrorHardware;
        AMRENC_EPRINT("%d :: Error closing ComponentThread - pthreadError = %d\n",__LINE__,pthreadError);
        goto EXIT;
    }
    if (OMX_ErrorNone != threadError && OMX_ErrorNone != eError) {
        eError = OMX_ErrorInsufficientResources;
        AMRENC_EPRINT("%d :: Error while closing Component Thread\n",__LINE__);
        goto EXIT;
    }
EXIT:
   AMRENC_DPRINT("%d :: Exiting NBAMRENC_StopComponentThread\n",__LINE__);
   AMRENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
   return eError;
}


/* ========================================================================== */
/**
* @NBAMRENC_HandleCommand() This function is called by the component when ever it
* receives the command from the application
*
* @param pComponentPrivate  Component private data
*
* @pre
*
* @post
*
* @return none
*/
/* ========================================================================== */

OMX_U32 NBAMRENC_HandleCommand (AMRENC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMMANDTYPE command;
    OMX_STATETYPE commandedState;
    OMX_HANDLETYPE pLcmlHandle;
#ifdef RESOURCE_MANAGER_ENABLED
    OMX_ERRORTYPE rm_error;
#endif
    LCML_CALLBACKTYPE cb;
    LCML_DSP *pLcmlDsp;
    OMX_U32 cmdValues[4];
    OMX_U32 pValues[4];
    OMX_U32 commandData;
    OMX_U16 arr[100];
    char *pArgs = "damedesuStr";
    char *p = "hello";
    OMX_U8* pParmsTemp;
    OMX_U8* pAlgParmTemp;
    OMX_U16 i = 0;
    OMX_U32 ret = 0;
    OMX_U8 inputPortFlag=0,outputPortFlag=0;    
    NBAMRENC_LCML_BUFHEADERTYPE *pLcmlHdr = NULL;
#ifdef DSP_RENDERING_ON
#if 0
   AM_COMMANDDATATYPE cmd_data;
#endif
#endif
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    pLcmlHandle = pComponentPrivate->pLcmlHandle;

    AMRENC_DPRINT("%d :: Entering NBAMRENCHandleCommand Function \n",__LINE__);
    AMRENC_DPRINT("%d :: pComponentPrivate->curState = %d\n",__LINE__,pComponentPrivate->curState);
    ret = read(pComponentPrivate->cmdPipe[0], &command, sizeof (command));
    if (ret == -1) {
        AMRENC_EPRINT("%d :: Error in Reading from the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    ret = read(pComponentPrivate->cmdDataPipe[0], &commandData, sizeof (commandData));
    if (ret == -1) {
        AMRENC_EPRINT("%d :: Error in Reading from the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedCommand(pComponentPrivate->pPERFcomp,
                        command,
                        commandData,
                        PERF_ModuleLLMM);
#endif
    if (command == OMX_CommandStateSet) {
        commandedState = (OMX_STATETYPE)commandData;
        switch(commandedState) {
        case OMX_StateIdle:
            AMRENC_DPRINT("%d :: NBAMRENC_HandleCommand :: OMX_StateIdle \n",__LINE__);
            AMRENC_DPRINT("%d :: pComponentPrivate->curState = %d\n",__LINE__,pComponentPrivate->curState);
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventError,
                                                         OMX_ErrorSameState,
                                                         OMX_TI_ErrorMinor,
                                                         NULL);
                AMRENC_EPRINT("%d :: Error: Same State Given by Application\n",__LINE__);
            }
            else if (pComponentPrivate->curState == OMX_StateLoaded) {
#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundarySetup);
#endif

                 if(pComponentPrivate->dasfMode == 1) {
                     if(pComponentPrivate->streamID == 0)
                     {
                         AMRENC_EPRINT("**************************************\n");
                         AMRENC_EPRINT(":: Error = OMX_ErrorInsufficientResources\n");
                         AMRENC_EPRINT("**************************************\n");
                         eError = OMX_ErrorInsufficientResources;
                         pComponentPrivate->curState = OMX_StateInvalid;
                         pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventError, 
                                                                OMX_ErrorInvalidState,
                                                                OMX_TI_ErrorMajor, 
                                                                "No Stream ID Available");
                         goto EXIT;
                     }
                 }

                 if (pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bPopulated &&  pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bEnabled)  {
                                     inputPortFlag = 1;
                 }
                 if (pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bPopulated && pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bEnabled) {
                                     outputPortFlag = 1;
                 }

                if (!pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bPopulated && !pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bEnabled) {
                    inputPortFlag = 1;
                }               
                
                if (!pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bPopulated && !pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bEnabled) {
                    outputPortFlag = 1;
                }

                if((pComponentPrivate->dasfMode && !outputPortFlag) ||
               (!pComponentPrivate->dasfMode && (!inputPortFlag || !outputPortFlag)))
                 {
                         /* Sleep for a while, so the application thread can allocate buffers */
                         AMRENC_DPRINT("%d :: Sleeping...\n",__LINE__);
                         pComponentPrivate->InLoaded_readytoidle = 1;
#ifndef UNDER_CE
                         pthread_mutex_lock(&pComponentPrivate->InLoaded_mutex); 
                         pthread_cond_wait(&pComponentPrivate->InLoaded_threshold, &pComponentPrivate->InLoaded_mutex);
                         pthread_mutex_unlock(&pComponentPrivate->InLoaded_mutex);
#else
                         OMX_WaitForEvent(&(pComponentPrivate->InLoaded_event));
#endif
                 }  

                cb.LCML_Callback = (void *) NBAMRENC_LCMLCallback;
                pLcmlHandle = (OMX_HANDLETYPE) NBAMRENC_GetLCMLHandle(pComponentPrivate);

                if (pLcmlHandle == NULL) {
                    AMRENC_DPRINT("%d :: LCML Handle is NULL........exiting..\n",__LINE__);
                    goto EXIT;
                }


                /* Got handle of dsp via phandle filling information about DSP Specific things */
                pLcmlDsp = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);
                eError = NBAMRENC_FillLCMLInitParams(pHandle, pLcmlDsp, arr);
                if(eError != OMX_ErrorNone) {
                    AMRENC_EPRINT("%d :: Error from NBAMRENCFill_LCMLInitParams()\n",__LINE__);
                    goto EXIT;
                }


                pComponentPrivate->pLcmlHandle = (LCML_DSP_INTERFACE *)pLcmlHandle;
                cb.LCML_Callback = (void *) NBAMRENC_LCMLCallback;

#ifndef UNDER_CE
                eError = LCML_InitMMCodecEx(((LCML_DSP_INTERFACE *)pLcmlHandle)->pCodecinterfacehandle,
                                          p,&pLcmlHandle,(void *)p,&cb, (OMX_STRING)pComponentPrivate->sDeviceString);

#else
                eError = LCML_InitMMCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                                    p,&pLcmlHandle, (void *)p, &cb);
#endif

                if(eError != OMX_ErrorNone) {
                    AMRENC_EPRINT("%d :: Error returned from LCML_Init()\n",__LINE__);
                    goto EXIT;
                }

#ifdef RESOURCE_MANAGER_ENABLED
                /* Need check the resource with RM */
                pComponentPrivate->rmproxyCallback.RMPROXY_Callback = (void *) NBAMRENC_ResourceManagerCallback;

                if (pComponentPrivate->curState != OMX_StateWaitForResources){

                    rm_error = RMProxy_NewSendCommand(pHandle, 
                                                        RMProxy_RequestResource, 
                                                        OMX_NBAMR_Encoder_COMPONENT, NBAMRENC_CPU_LOAD, 
                                                        3456, &(pComponentPrivate->rmproxyCallback));   

                    if(rm_error == OMX_ErrorNone) {
                        /* resource is available */
#ifdef __PERF_INSTRUMENTATION__
                        PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySetup);
#endif
                        pComponentPrivate->curState = OMX_StateIdle;
                        pComponentPrivate->cbInfo.EventHandler(pHandle,
                                pHandle->pApplicationPrivate,
                                OMX_EventCmdComplete, 
                                OMX_CommandStateSet,
                                pComponentPrivate->curState, 
                                NULL);

                        rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_NBAMR_Encoder_COMPONENT, OMX_StateIdle, 3456, NULL);
                    }
                    else if(rm_error == OMX_ErrorInsufficientResources) {
                        /* resource is not available, need set state to OMX_StateWaitForResources */
                        pComponentPrivate->curState = OMX_StateWaitForResources;
                        pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventCmdComplete,
                                                                OMX_CommandStateSet,
                                                                pComponentPrivate->curState,
                                                                NULL);
                        AMRENC_EPRINT("%d :: Comp: OMX_ErrorInsufficientResources\n", __LINE__);
                    }
            }
            else {
                    pComponentPrivate->curState = OMX_StateIdle;
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventCmdComplete, 
                                                            OMX_CommandStateSet,
                                                            pComponentPrivate->curState, 
                                                            NULL);

                rm_error = RMProxy_NewSendCommand(pHandle,
                                                       RMProxy_StateSet,
                                                       OMX_NBAMR_Encoder_COMPONENT,
                                                       OMX_StateIdle,
                                                       3456,
                                                       NULL);

            }
#else           
                pComponentPrivate->curState = OMX_StateIdle;
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandStateSet,
                                                        pComponentPrivate->curState,
                                                        NULL);

#endif
                
            }
            else if (pComponentPrivate->curState == OMX_StateExecuting) {
                AMRENC_DPRINT("%d :: Setting Component to OMX_StateIdle\n",__LINE__);
                AMRENC_DPRINT("%d :: AMRENC: About to Call MMCodecControlStop\n", __LINE__);
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif
                pComponentPrivate->bIsStopping = 1;

                if (pComponentPrivate->codecStop_waitingsignal == 0){ 
                    pthread_mutex_lock(&pComponentPrivate->codecStop_mutex);    
                }
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                                    MMCodecControlStop,(void *)pArgs);

                if (pComponentPrivate->codecStop_waitingsignal == 0){
                    pthread_cond_wait(&pComponentPrivate->codecStop_threshold, &pComponentPrivate->codecStop_mutex);
                    pComponentPrivate->codecStop_waitingsignal = 0;
                    pthread_mutex_unlock(&pComponentPrivate->codecStop_mutex);
                }
                    
                pAlgParmTemp = (OMX_U8*)pComponentPrivate->pAlgParam;
                if (pAlgParmTemp != NULL)
                   pAlgParmTemp -= EXTRA_BYTES;
                pComponentPrivate->pAlgParam = (NBAMRENC_TALGCtrl*)pAlgParmTemp;
                OMX_NBMEMFREE_STRUCT(pComponentPrivate->pAlgParam);
                
                pComponentPrivate->nOutStandingFillDones = 0;
                                pComponentPrivate->nOutStandingEmptyDones = 0; 

    
                pParmsTemp = (OMX_U8*)pComponentPrivate->pParams;
                if (pParmsTemp != NULL){
                     pParmsTemp -= EXTRA_BYTES;
                }
                pComponentPrivate->pParams = (NBAMRENC_AudioCodecParams*)pParmsTemp;
                OMX_NBMEMFREE_STRUCT(pComponentPrivate->pParams);

                if(eError != OMX_ErrorNone) {
                    AMRENC_EPRINT("%d :: Error from LCML_ControlCodec MMCodecControlStop..\n",__LINE__);
                    goto EXIT;
                }
            }
            else if(pComponentPrivate->curState == OMX_StatePause) {

                pComponentPrivate->curState = OMX_StateIdle;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif
#ifdef RESOURCE_MANAGER_ENABLED
                rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_NBAMR_Encoder_COMPONENT, OMX_StateIdle, 3456, NULL);

#endif
                AMRENC_DPRINT ("%d :: The component is stopped\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventCmdComplete,
                                                         OMX_CommandStateSet,
                                                         pComponentPrivate->curState,
                                                         NULL);
            } else {    /* This means, it is invalid state from application */
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorIncorrectStateTransition,
                                                        OMX_TI_ErrorMinor,
                                                        "Invalid State");
                AMRENC_EPRINT("%d :: Comp: OMX_ErrorIncorrectStateTransition\n",__LINE__);
            }
            break;

        case OMX_StateExecuting:
            AMRENC_DPRINT("%d :: NBAMRENC_HandleCommand :: OMX_StateExecuting \n",__LINE__);
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventError,
                                                         OMX_ErrorSameState,
                                                         OMX_TI_ErrorMinor,
                                                         "Invalid State");
                AMRENC_EPRINT("%d :: Comp: OMX_ErrorSameState Given by Comp\n",__LINE__);
                break;
            }
            else if (pComponentPrivate->curState == OMX_StateIdle) {
                /* Sending commands to DSP via LCML_ControlCodec third argument
                is not used for time being */
                pComponentPrivate->nNumInputBufPending = 0;
                pComponentPrivate->nNumOutputBufPending = 0;
                pComponentPrivate->nNumOfFramesSent=0;
                pComponentPrivate->nEmptyBufferDoneCount = 0;
                pComponentPrivate->nEmptyThisBufferCount =0;

                                
                
                NBAMRENC_OMX_MALLOC_SIZE(pAlgParmTemp, sizeof(NBAMRENC_TALGCtrl) + DSP_CACHE_ALIGNMENT,OMX_U8);
                
                pComponentPrivate->pAlgParam = (NBAMRENC_TALGCtrl*)(pAlgParmTemp + EXTRA_BYTES);
                AMRENC_MEMPRINT("%d :: [ALLOC] %p\n",__LINE__,pComponentPrivate->pAlgParam);

                pComponentPrivate->pAlgParam->iBitrate = pComponentPrivate->amrParams->eAMRBandMode;
                if (pComponentPrivate->amrParams->eAMRDTXMode == OMX_AUDIO_AMRDTXModeOnAuto) {
                    pComponentPrivate->pAlgParam->iDTX = OMX_TRUE;
                }
                else {
                    pComponentPrivate->pAlgParam->iDTX = OMX_FALSE;
                }

                pComponentPrivate->pAlgParam->iSize = sizeof (NBAMRENC_TALGCtrl);

                AMRENC_DPRINT("%d :: pAlgParam->iBitrate = %d\n",__LINE__,pComponentPrivate->pAlgParam->iBitrate);
                AMRENC_DPRINT("%d :: pAlgParam->iDTX  = %d\n",__LINE__,pComponentPrivate->pAlgParam->iDTX);

                cmdValues[0] = ALGCMD_BITRATE;                  /*setting the bit-rate*/
                cmdValues[1] = (OMX_U32)pComponentPrivate->pAlgParam;
                cmdValues[2] = sizeof (NBAMRENC_TALGCtrl);
                p = (void *)&cmdValues;
                AMRENC_DPRINT("%d :: EMMCodecControlAlgCtrl-1 Sending...\n",__LINE__);
                /* Sending ALGCTRL MESSAGE DTX to DSP via LCML_ControlCodec*/
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                EMMCodecControlAlgCtrl, (void *)p);
                if (eError != OMX_ErrorNone) {
                    AMRENC_EPRINT("%d :: Error from LCML_ControlCodec EMMCodecControlAlgCtrl-1 failed = %x\n",__LINE__,eError);
                    goto EXIT;
                }
                cmdValues[0] = ALGCMD_DTX;                  /*setting DTX mode*/
                cmdValues[1] = (OMX_U32)pComponentPrivate->pAlgParam;
                cmdValues[2] = sizeof (NBAMRENC_TALGCtrl);
                p = (void *)&cmdValues;
                AMRENC_DPRINT("%d :: EMMCodecControlAlgCtrl-2 Sending...\n",__LINE__);
                /* Sending ALGCTRL MESSAGE BITRATE to DSP via LCML_ControlCodec*/
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                EMMCodecControlAlgCtrl, (void *)p);
                if (eError != OMX_ErrorNone) {
                    AMRENC_EPRINT("%d :: Error from LCML_ControlCodec EMMCodecControlAlgCtrl-2 failed = %x\n",__LINE__,eError);
                    goto EXIT;
                }
                if(pComponentPrivate->dasfMode == 1) {
                    AMRENC_DPRINT("%d :: ---- Comp: DASF Functionality is ON ---\n",__LINE__);

                    
                    NBAMRENC_OMX_MALLOC_SIZE(pParmsTemp, sizeof(NBAMRENC_AudioCodecParams) + DSP_CACHE_ALIGNMENT,OMX_U8);

                    pComponentPrivate->pParams = (NBAMRENC_AudioCodecParams*)(pParmsTemp + EXTRA_BYTES);
                    AMRENC_MEMPRINT("%d :: [ALLOC] %p\n",__LINE__,pComponentPrivate->pParams);

                    pComponentPrivate->pParams->iAudioFormat = 1;
                    pComponentPrivate->pParams->iStrmId = pComponentPrivate->streamID;
                    pComponentPrivate->pParams->iSamplingRate = NBAMRENC_SAMPLING_FREQUENCY;
                    pValues[0] = USN_STRMCMD_SETCODECPARAMS;
                    pValues[1] = (OMX_U32)pComponentPrivate->pParams;
                    pValues[2] = sizeof(NBAMRENC_AudioCodecParams);
                    /* Sending STRMCTRL MESSAGE to DSP via LCML_ControlCodec*/
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                              EMMCodecControlStrmCtrl,(void *)pValues);
                    if(eError != OMX_ErrorNone) {
                       AMRENC_EPRINT("%d :: Error from LCML_ControlCodec EMMCodecControlStrmCtrl = %x\n",__LINE__,eError);
                       goto EXIT;
                    }
                }
                /* Sending START MESSAGE to DSP via LCML_ControlCodec*/
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                EMMCodecControlStart, (void *)p);
                if(eError != OMX_ErrorNone) {
                    AMRENC_EPRINT("%d :: Error from LCML_ControlCodec EMMCodecControlStart = %x\n",__LINE__,eError);
                    goto EXIT;
                }

            } else if (pComponentPrivate->curState == OMX_StatePause) {
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                EMMCodecControlStart, (void *)p);
                if (eError != OMX_ErrorNone) {
                    AMRENC_EPRINT("%d :: Error While Resuming the codec = %x\n",__LINE__,eError);
                    goto EXIT;
                }

                for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                    if (pComponentPrivate->pInputBufHdrPending[i]) {
                        NBAMRENC_GetCorrespondingLCMLHeader(pComponentPrivate, pComponentPrivate->pInputBufHdrPending[i]->pBuffer, OMX_DirInput, &pLcmlHdr);
                        NBAMRENC_SetPending(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i],OMX_DirInput,__LINE__);

                        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                    EMMCodecInputBuffer,
                                                    pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                    pComponentPrivate->pInputBufHdrPending[i]->nAllocLen,
                                                    pComponentPrivate->pInputBufHdrPending[i]->nFilledLen,
                                                    (OMX_U8 *) pLcmlHdr->pBufferParam,
                                                    sizeof(NBAMRENC_ParamStruct),
                                                    NULL);
                    }
                }
                pComponentPrivate->nNumInputBufPending = 0;

                for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                    if (pComponentPrivate->pOutputBufHdrPending[i]) {
                        NBAMRENC_GetCorrespondingLCMLHeader(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i]->pBuffer, OMX_DirOutput, &pLcmlHdr);
                        NBAMRENC_SetPending(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i],OMX_DirOutput,__LINE__);
                        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                    EMMCodecOuputBuffer,
                                                    pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                    pComponentPrivate->pOutputBufHdrPending[i]->nAllocLen,
                                                    pComponentPrivate->pOutputBufHdrPending[i]->nFilledLen,
                                                    (OMX_U8 *) pLcmlHdr->pBufferParam,
                                                    sizeof(NBAMRENC_ParamStruct),
                                                    NULL);
                    }
                }
                pComponentPrivate->nNumOutputBufPending = 0;
            } else {
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorIncorrectStateTransition,
                                                        OMX_TI_ErrorMinor,
                                                        "Incorrect State Transition");
                AMRENC_EPRINT("%d :: Comp: OMX_ErrorIncorrectStateTransition Given by Comp\n",__LINE__);
                goto EXIT;

            }
#ifdef RESOURCE_MANAGER_ENABLED
             rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_NBAMR_Encoder_COMPONENT, OMX_StateExecuting, 3456, NULL);
#endif
            pComponentPrivate->curState = OMX_StateExecuting;
#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundarySteadyState);
#endif
            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventCmdComplete,
                                                    OMX_CommandStateSet,
                                                    pComponentPrivate->curState,
                                                    NULL);
            AMRENC_DPRINT("%d :: Comp: OMX_CommandStateSet Given by Comp\n",__LINE__);
            break;

        case OMX_StateLoaded:
            AMRENC_DPRINT("%d :: NBAMRENC_HandleCommand :: OMX_StateLoaded\n",__LINE__);
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventError,
                                                         OMX_ErrorSameState,
                                                         OMX_TI_ErrorMinor,
                                                         "Same State");
                AMRENC_EPRINT("%d :: Comp: OMX_ErrorSameState Given by Comp\n",__LINE__);
                break;
             }
            if (pComponentPrivate->curState == OMX_StateWaitForResources){
                AMRENC_DPRINT("%d :: NBAMRENC_HandleCommand :: OMX_StateWaitForResources\n",__LINE__);
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif
                pComponentPrivate->curState = OMX_StateLoaded;
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif
                pComponentPrivate->curState = OMX_StateLoaded;
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventCmdComplete,
                                                         OMX_CommandStateSet,
                                                         pComponentPrivate->curState,
                                                         NULL);
                AMRENC_DPRINT("%d :: Comp: OMX_CommandStateSet Given by Comp\n",__LINE__);
                break;
            }
            if (pComponentPrivate->curState != OMX_StateIdle &&
                pComponentPrivate->curState != OMX_StateWaitForResources) {
                AMRENC_DPRINT("%d :: NBAMRENC_HandleCommand :: OMX_StateIdle && OMX_StateWaitForResources\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventError,
                                                         OMX_ErrorIncorrectStateTransition,
                                                         OMX_TI_ErrorMinor,
                                                         "Incorrect State Transition");
                AMRENC_EPRINT("%d :: Error: OMX_ErrorIncorrectStateTransition Given by Comp\n",__LINE__);
                goto EXIT;
            }
#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif
            AMRENC_DPRINT("%d :: NBAMRENC_HandleCommand - evaluating if all buffers are free\n",__LINE__);
            
            if (pComponentPrivate->pInputBufferList->numBuffers ||
                pComponentPrivate->pOutputBufferList->numBuffers) {

                pthread_mutex_lock(&pComponentPrivate->ToLoaded_mutex);
                pComponentPrivate->InIdle_goingtoloaded = 1;
                pthread_mutex_unlock(&pComponentPrivate->ToLoaded_mutex);

            }

            if (!pComponentPrivate->pInputBufferList->numBuffers &&
                !pComponentPrivate->pOutputBufferList->numBuffers) {

            /* Now Deinitialize the component No error should be returned from
            * this function. It should clean the system as much as possible */
            NBAMRENC_CleanupInitParams(pComponentPrivate->pHandle);
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                        EMMCodecControlDestroy, (void *)p);
            if (eError != OMX_ErrorNone) {
                AMRENC_EPRINT("%d :: Error: LCML_ControlCodec EMMCodecControlDestroy = %x\n",__LINE__, eError);
                goto EXIT;
            }

    /*Closing LCML Lib*/
    if (pComponentPrivate->ptrLibLCML != NULL)
    {
        AMRENC_DPRINT("%d OMX_AmrEncoder.c Closing LCML library\n",__LINE__);
        dlclose( pComponentPrivate->ptrLibLCML);
        pComponentPrivate->ptrLibLCML = NULL;
    }   

            
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingCommand(pComponentPrivate->pPERF, -1, 0, PERF_ModuleComponent);
#endif
            eError = NBAMRENC_EXIT_COMPONENT_THRD;
            pComponentPrivate->bInitParamsInitialized = 0;
            pComponentPrivate->bLoadedCommandPending = OMX_FALSE;
            pComponentPrivate->bLoadedWaitingFreeBuffers = OMX_FALSE;
            
        }
        else {
            pComponentPrivate->bLoadedWaitingFreeBuffers = OMX_TRUE;
            AMRENC_DPRINT("Skipped this section because buffers not yet freed\n");
        }
            break;

        case OMX_StatePause:
            AMRENC_DPRINT("%d :: NBAMRENC_HandleCommand :: OMX_StatePause\n",__LINE__);
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventError,
                                                         OMX_ErrorSameState,
                                                         OMX_TI_ErrorMinor,
                                                         "Same State");
                AMRENC_EPRINT("%d :: Error: OMX_ErrorSameState Given by Comp\n",__LINE__);
                break;
            }
            if (pComponentPrivate->curState != OMX_StateExecuting &&
                pComponentPrivate->curState != OMX_StateIdle) {
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventError,
                                                         OMX_ErrorIncorrectStateTransition,
                                                         OMX_TI_ErrorMinor,
                                                         "Incorrect State Transition");
                AMRENC_EPRINT("%d :: Error: OMX_ErrorIncorrectStateTransition Given by Comp\n",__LINE__);
                goto EXIT;
            }
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                        EMMCodecControlPause, (void *)p);
            if (eError != OMX_ErrorNone) {
                AMRENC_EPRINT("%d :: Error: LCML_ControlCodec EMMCodecControlPause = %x\n",__LINE__,eError);
                goto EXIT;
            }
            AMRENC_DPRINT("%d :: Comp: OMX_CommandStateSet Given by Comp\n",__LINE__);
            break;

        case OMX_StateWaitForResources:
            if (pComponentPrivate->curState == commandedState) {
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventError,
                                                         OMX_ErrorSameState,
                                                         OMX_TI_ErrorMinor,
                                                         "Same State");
                AMRENC_EPRINT("%d :: Error: OMX_ErrorSameState Given by Comp\n",__LINE__);
            } else if (pComponentPrivate->curState == OMX_StateLoaded) {

#ifdef RESOURCE_MANAGER_ENABLED         
            rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_NBAMR_Encoder_COMPONENT, OMX_StateWaitForResources, 3456, NULL);
#endif 
            
                pComponentPrivate->curState = OMX_StateWaitForResources;
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandStateSet,
                                                        pComponentPrivate->curState,
                                                        NULL);
                AMRENC_DPRINT("%d :: Comp: OMX_CommandStateSet Given by Comp\n",__LINE__);
            } else {
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorIncorrectStateTransition,
                                                        OMX_TI_ErrorMinor,
                                                        "Incorrect State Transition");
                AMRENC_EPRINT("%d :: Error: OMX_ErrorIncorrectStateTransition Given by Comp\n",__LINE__);
            }
            break;

        case OMX_StateInvalid:
            if (pComponentPrivate->curState == commandedState) {
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventError,
                                                         OMX_ErrorSameState,
                                                         OMX_TI_ErrorSevere,
                                                         "Same State");
                AMRENC_EPRINT("%d :: Error: OMX_ErrorSameState Given by Comp\n",__LINE__);
            }
            else {
                AMRENC_DPRINT("%d: HandleCommand: Cmd OMX_StateInvalid:\n",__LINE__);
                if (pComponentPrivate->curState != OMX_StateWaitForResources && 
                    pComponentPrivate->curState != OMX_StateInvalid && 
                    pComponentPrivate->curState != OMX_StateLoaded) {

                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                EMMCodecControlDestroy, (void *)pArgs);
                }
                pComponentPrivate->curState = OMX_StateInvalid;
                pComponentPrivate->cbInfo.EventHandler( pHandle, 
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError, 
                                                    OMX_ErrorInvalidState,
                                                    OMX_TI_ErrorSevere, 
                                                    "Incorrect State Transition");

                AMRENC_EPRINT("%d :: Comp: OMX_ErrorInvalidState Given by Comp\n",__LINE__);
                NBAMRENC_CleanupInitParams(pHandle);
            }
            break;

        case OMX_StateMax:
            AMRENC_DPRINT("%d :: NBAMRENC_HandleCommand :: Cmd OMX_StateMax\n",__LINE__);
            break;
        } /* End of Switch */
    } else if (command == OMX_CommandMarkBuffer) {
        AMRENC_DPRINT("%d :: command OMX_CommandMarkBuffer received\n",__LINE__);
        if(!pComponentPrivate->pMarkBuf){
            /* TODO Need to handle multiple marks */
            pComponentPrivate->pMarkBuf = (OMX_MARKTYPE *)(commandData);
        }
    } else if (command == OMX_CommandPortDisable) {
        if (!pComponentPrivate->bDisableCommandPending) {
            AMRENC_DPRINT("I'm here Line %d\n",__LINE__);
            if(commandData == 0x0 || commandData == -1){
                pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bEnabled = OMX_FALSE;
            }
            if(commandData == 0x1 || commandData == -1){
                char *pArgs = "damedesuStr";

                pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bEnabled = OMX_FALSE;
                if (pComponentPrivate->curState == OMX_StateExecuting) {
                    pComponentPrivate->bNoIdleOnStop = OMX_TRUE;
                        pComponentPrivate->bIsStopping = 1;
                    if (pComponentPrivate->codecStop_waitingsignal == 0){ 
                        pthread_mutex_lock(&pComponentPrivate->codecStop_mutex);    
                    }
                    eError = LCML_ControlCodec(
                                  ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                  MMCodecControlStop,(void *)pArgs);
                    if (pComponentPrivate->codecStop_waitingsignal == 0){
                        pthread_cond_wait(&pComponentPrivate->codecStop_threshold, &pComponentPrivate->codecStop_mutex);
                        pComponentPrivate->codecStop_waitingsignal = 0;
                        pthread_mutex_unlock(&pComponentPrivate->codecStop_mutex);
                    }

                }
            }
        }

        AMRENC_DPRINT("commandData = %d\n",(int)commandData);
        AMRENC_DPRINT("pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bPopulated = %d\n",pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bPopulated);
        AMRENC_DPRINT("pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bPopulated = %d\n",pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bPopulated);

        if(commandData == 0x0) {
            if(!pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bPopulated){
                /* return cmdcomplete event if input unpopulated */
                pComponentPrivate->cbInfo.EventHandler(
                     pHandle, pHandle->pApplicationPrivate,
                     OMX_EventCmdComplete, OMX_CommandPortDisable,NBAMRENC_INPUT_PORT, NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else{
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }

        if(commandData == 0x1) {
            if (!pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bPopulated){
                /* return cmdcomplete event if output unpopulated */
                pComponentPrivate->cbInfo.EventHandler(
                    pHandle, pHandle->pApplicationPrivate,
                    OMX_EventCmdComplete, OMX_CommandPortDisable,NBAMRENC_OUTPUT_PORT, NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }

        if(commandData == -1) {
            if (!pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bPopulated &&
                !pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bPopulated){

                /* return cmdcomplete event if inout & output unpopulated */
                pComponentPrivate->cbInfo.EventHandler(
                     pHandle, pHandle->pApplicationPrivate,
                     OMX_EventCmdComplete, OMX_CommandPortDisable,NBAMRENC_INPUT_PORT, NULL);

                pComponentPrivate->cbInfo.EventHandler(
                     pHandle, pHandle->pApplicationPrivate,
                     OMX_EventCmdComplete, OMX_CommandPortDisable,NBAMRENC_OUTPUT_PORT, NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }
#ifndef UNDER_CE        
        sched_yield();
#endif        
    }
    else if (command == OMX_CommandPortEnable) {
        if(!pComponentPrivate->bEnableCommandPending) {
            if(commandData == 0x0 || commandData == -1){
                /* enable in port */
                AMRENC_DPRINT("setting input port to enabled\n");
                pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bEnabled = OMX_TRUE;
                if(pComponentPrivate->AlloBuf_waitingsignal)
                {
                     pComponentPrivate->AlloBuf_waitingsignal = 0;

                }
                AMRENC_DPRINT("pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bEnabled = %d\n",pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bEnabled);
            }
            if(commandData == 0x1 || commandData == -1){
                char *pArgs = "damedesuStr";
                /* enable out port */
                if(pComponentPrivate->AlloBuf_waitingsignal)
                {
                     pComponentPrivate->AlloBuf_waitingsignal = 0;
#ifndef UNDER_CE
                     pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex);
                     pthread_cond_signal(&pComponentPrivate->AlloBuf_threshold);
                     pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
#else
                     OMX_SignalEvent(&(pComponentPrivate->AlloBuf_event));
#endif
                }
                if (pComponentPrivate->curState == OMX_StateExecuting) {
                    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
                    eError = LCML_ControlCodec(
                                          ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                          EMMCodecControlStart,(void *)pArgs);
                }
                AMRENC_DPRINT("setting output port to enabled\n");
                pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bEnabled = OMX_TRUE;
                AMRENC_DPRINT("pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bEnabled = %d\n",pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bEnabled);
            }
        }
          if(commandData == 0x0 ){
                if (pComponentPrivate->curState == OMX_StateLoaded || pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bPopulated){
                pComponentPrivate->cbInfo.EventHandler(
                     pHandle, pHandle->pApplicationPrivate,
                     OMX_EventCmdComplete, OMX_CommandPortEnable,NBAMRENC_INPUT_PORT, NULL);
                     pComponentPrivate->bEnableCommandPending = 0;
                    }
                else{
                    pComponentPrivate->bEnableCommandPending = 1;
                    pComponentPrivate->bEnableCommandParam = commandData;
                }   
              } 
            else if(commandData == 0x1){
                if (pComponentPrivate->curState == OMX_StateLoaded || pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bPopulated){
                pComponentPrivate->cbInfo.EventHandler(
                     pHandle, pHandle->pApplicationPrivate,
                     OMX_EventCmdComplete, OMX_CommandPortEnable,NBAMRENC_OUTPUT_PORT, NULL);
                     pComponentPrivate->bEnableCommandPending = 0;
                }
                else{
                     pComponentPrivate->bEnableCommandPending = 1;
                     pComponentPrivate->bEnableCommandParam = commandData;
                }   
            }
            else if(commandData == -1){
                if(pComponentPrivate->curState == OMX_StateLoaded || (pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bPopulated
                    && pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bPopulated)){
                    pComponentPrivate->cbInfo.EventHandler(
                     pHandle, pHandle->pApplicationPrivate,
                     OMX_EventCmdComplete, OMX_CommandPortEnable,NBAMRENC_INPUT_PORT, NULL);

                    pComponentPrivate->cbInfo.EventHandler(
                     pHandle, pHandle->pApplicationPrivate,
                     OMX_EventCmdComplete, OMX_CommandPortEnable,NBAMRENC_OUTPUT_PORT, NULL);

                    pComponentPrivate->bEnableCommandPending = 0;
                    NBAMRENC_FillLCMLInitParamsEx(pComponentPrivate->pHandle);
                }
                else {
                    pComponentPrivate->bEnableCommandPending = 1;
                    pComponentPrivate->bEnableCommandParam = commandData;
                }
            }
#ifndef UNDER_CE
                    pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex); 
                    pthread_cond_signal(&pComponentPrivate->AlloBuf_threshold);
                    pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);    
#else
                    OMX_SignalEvent(&(pComponentPrivate->AlloBuf_event));
#endif

    } else if (command == OMX_CommandFlush) {
          if(commandData == 0x0 || commandData == -1){/*input*/
                    AMRENC_DPRINT("Flushing input port %d\n",__LINE__);
                    for (i=0; i < NBAMRENC_MAX_NUM_OF_BUFS; i++) {
                        pComponentPrivate->pInputBufHdrPending[i] = NULL;
                    }
                    pComponentPrivate->nNumInputBufPending=0;

            for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
                         

#ifdef __PERF_INSTRUMENTATION__
                            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                              pComponentPrivate->pInputBufferList->pBufHdr[i]->pBuffer,
                                              0,
                                              PERF_ModuleHLMM);
#endif
                       pComponentPrivate->cbInfo.EmptyBufferDone (
                                       pComponentPrivate->pHandle,
                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                       pComponentPrivate->pInputBufferList->pBufHdr[i]
                                       );
                    pComponentPrivate->nEmptyBufferDoneCount++;
                    pComponentPrivate->nOutStandingEmptyDones--;

                    }
                    pComponentPrivate->cbInfo.EventHandler(
                         pHandle, pHandle->pApplicationPrivate,
                         OMX_EventCmdComplete, OMX_CommandFlush,NBAMRENC_INPUT_PORT, NULL);
          }
      
          if(commandData == 0x1 || commandData == -1){/*output*/
                     for (i=0; i < NBAMRENC_MAX_NUM_OF_BUFS; i++) {
                        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
                    }
                    pComponentPrivate->nNumOutputBufPending=0;

                    for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      pComponentPrivate->pOutputBufferList->pBufHdr[i]->pBuffer,
                                      pComponentPrivate->pOutputBufferList->pBufHdr[i]->nFilledLen,
                                      PERF_ModuleHLMM);
#endif
                       pComponentPrivate->cbInfo.FillBufferDone (
                                       pComponentPrivate->pHandle,
                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                       pComponentPrivate->pOutputBufferList->pBufHdr[i]
                                       );
                        pComponentPrivate->nFillBufferDoneCount++;                                       
                        pComponentPrivate->nOutStandingFillDones--;

                    }
                     pComponentPrivate->cbInfo.EventHandler(
                             pHandle, pHandle->pApplicationPrivate,
                             OMX_EventCmdComplete, OMX_CommandFlush,NBAMRENC_OUTPUT_PORT, NULL);
          }
    }

EXIT:
    AMRENC_DPRINT("%d :: Exiting NBAMRENC_HandleCommand Function\n",__LINE__);
    AMRENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ========================================================================== */
/**
* @NBAMRENC_HandleDataBufFromApp() This function is called by the component when ever it
* receives the buffer from the application
*
* @param pComponentPrivate  Component private data
* @param pBufHeader Buffer from the application
*
* @pre
*
* @post
*
* @return none
*/
/* ========================================================================== */
OMX_ERRORTYPE NBAMRENC_HandleDataBufFromApp(OMX_BUFFERHEADERTYPE* pBufHeader,
                                    AMRENC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_DIRTYPE eDir;
    NBAMRENC_LCML_BUFHEADERTYPE *pLcmlHdr = NULL;
    LCML_DSP_INTERFACE *pLcmlHandle = (LCML_DSP_INTERFACE *)
                                              pComponentPrivate->pLcmlHandle;
    OMX_U32 frameLength, remainingBytes;
    OMX_U8* pExtraData = NULL;
    OMX_U8 nFrames=0,i;
    LCML_DSP_INTERFACE * phandle = NULL;
    OMX_U8* pBufParmsTemp = NULL;


    AMRENC_DPRINT ("%d :: Entering NBAMRENC_HandleDataBufFromApp Function\n",__LINE__);
    /*Find the direction of the received buffer from buffer list */
        eError = NBAMRENC_GetBufferDirection(pBufHeader, &eDir);
        if (eError != OMX_ErrorNone) {
                AMRENC_EPRINT ("%d :: The pBufHeader is not found in the list\n", __LINE__);
                goto EXIT;
        }

    if (eDir == OMX_DirInput) {
                pComponentPrivate->nEmptyThisBufferCount++;
                pComponentPrivate->nUnhandledEmptyThisBuffers--;
                if  (pBufHeader->nFilledLen > 0) {
                        if (pComponentPrivate->nHoldLength == 0) {
                                frameLength = NBAMRENC_INPUT_FRAME_SIZE;
                                nFrames = (OMX_U8)(pBufHeader->nFilledLen / frameLength);
                                if ( nFrames>=1 ) { /*At least there is 1 frame in the buffer*/
                                        pComponentPrivate->nHoldLength = pBufHeader->nFilledLen - frameLength*nFrames;
                                        if (pComponentPrivate->nHoldLength > 0) {/* something need to be hold in pHoldBuffer */
                                                if (pComponentPrivate->pHoldBuffer == NULL) {
                                                   NBAMRENC_OMX_MALLOC_SIZE(pComponentPrivate->pHoldBuffer, NBAMRENC_INPUT_FRAME_SIZE,OMX_U8);
                                                }
                                                
                                                /* Copy the extra data into pHoldBuffer. Size will be nHoldLength. */
                                                pExtraData = pBufHeader->pBuffer + frameLength*nFrames;
                                                
                        if(pComponentPrivate->nHoldLength <= NBAMRENC_INPUT_FRAME_SIZE) {
                            memcpy(pComponentPrivate->pHoldBuffer, pExtraData,  pComponentPrivate->nHoldLength);
                        }
                        else {
                            AMRENC_EPRINT ("%d :: Error: pHoldLenght is bigger than the input frame size\n", __LINE__);
                                    goto EXIT;
                        }
                                                
                                                pBufHeader->nFilledLen-=pComponentPrivate->nHoldLength;
                                        }
                                }
                                else {
                                        if( !pComponentPrivate->InBuf_Eos_alreadysent ){
                                                /* received buffer with less than 1 AMR frame length. Save the data in pHoldBuffer.*/
                                                pComponentPrivate->nHoldLength = pBufHeader->nFilledLen;
                                                /* save the data into pHoldBuffer */
                                                if (pComponentPrivate->pHoldBuffer == NULL) {
                                                    NBAMRENC_OMX_MALLOC_SIZE(pComponentPrivate->pHoldBuffer, NBAMRENC_INPUT_FRAME_SIZE,OMX_U8); 
                                                }
                                                /* Not enough data to be sent. Copy all received data into pHoldBuffer.*/
                                                /* Size to be copied will be iHoldLen == mmData->BufferSize() */
                                                memset(pComponentPrivate->pHoldBuffer, 0, NBAMRENC_INPUT_FRAME_SIZE);
                                                
                                                if(pComponentPrivate->nHoldLength <= NBAMRENC_INPUT_FRAME_SIZE) {
                                                    memcpy(pComponentPrivate->pHoldBuffer, pBufHeader->pBuffer, pComponentPrivate->nHoldLength);
                                                }
                                                else {
                                                    AMRENC_EPRINT ("%d :: Error: pHoldLenght is bigger than the input frame size\n", __LINE__);
                                                    goto EXIT;
                                                }  
                                        }
                                        /* since not enough data, we shouldn't send anything to SN, but instead request to EmptyBufferDone again.*/
                                        if (pComponentPrivate->curState != OMX_StatePause ) {
                                                AMRENC_DPRINT("%d :: Calling EmptyBufferDone\n",__LINE__);
                                                
#ifdef __PERF_INSTRUMENTATION__
                            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                              pBufHeader->pBuffer,
                                              0,
                                              PERF_ModuleHLMM);
#endif

                                              pComponentPrivate->cbInfo.EmptyBufferDone( pComponentPrivate->pHandle,
                                                                                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                                                                                pBufHeader);
                                              pComponentPrivate->nEmptyBufferDoneCount++;

                                        }
                                        else {
                                                pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
                                        }

                                        pComponentPrivate->ProcessingInputBuf--;
                                        goto EXIT;

                                }
                        }
                        else {
                                if((pComponentPrivate->nHoldLength+pBufHeader->nFilledLen) > pBufHeader->nAllocLen){
                                        /*means that a second Acumulator must be used to insert holdbuffer to pbuffer and save remaining bytes
                                                     into hold buffer*/
                                        remainingBytes = pComponentPrivate->nHoldLength+pBufHeader->nFilledLen-pBufHeader->nAllocLen;
                                        if (pComponentPrivate->pHoldBuffer2 == NULL) {
                                                        NBAMRENC_OMX_MALLOC_SIZE(pComponentPrivate->pHoldBuffer2, NBAMRENC_INPUT_FRAME_SIZE,OMX_U8);
                                        }
                                        pExtraData = (pBufHeader->pBuffer)+(pBufHeader->nFilledLen-remainingBytes);
                                        memcpy(pComponentPrivate->pHoldBuffer2,pExtraData,remainingBytes);
                                        pBufHeader->nFilledLen-=remainingBytes;
                                        memmove(pBufHeader->pBuffer+ pComponentPrivate->nHoldLength,pBufHeader->pBuffer,pBufHeader->nFilledLen);
                                        memcpy(pBufHeader->pBuffer,pComponentPrivate->pHoldBuffer,pComponentPrivate->nHoldLength);
                                        pBufHeader->nFilledLen+=pComponentPrivate->nHoldLength;
                                        memcpy(pComponentPrivate->pHoldBuffer, pComponentPrivate->pHoldBuffer2, remainingBytes);
                                        pComponentPrivate->nHoldLength=remainingBytes;
                                }
                                else{
                                        memmove(pBufHeader->pBuffer+pComponentPrivate->nHoldLength, pBufHeader->pBuffer, pBufHeader->nFilledLen);
                                        memcpy(pBufHeader->pBuffer,pComponentPrivate->pHoldBuffer, pComponentPrivate->nHoldLength);
                                        pBufHeader->nFilledLen+=pComponentPrivate->nHoldLength;
                                        pComponentPrivate->nHoldLength=0;
                                }
                                frameLength = NBAMRENC_INPUT_FRAME_SIZE;
                                nFrames = (OMX_U8)(pBufHeader->nFilledLen / frameLength);
                                pComponentPrivate->nHoldLength = pBufHeader->nFilledLen - frameLength*nFrames;
                                pExtraData = pBufHeader->pBuffer + pBufHeader->nFilledLen-pComponentPrivate->nHoldLength;
                                memcpy(pComponentPrivate->pHoldBuffer, pExtraData,  pComponentPrivate->nHoldLength);
                                pBufHeader->nFilledLen-=pComponentPrivate->nHoldLength;
                                if(nFrames < 1 ){
                                        if (pComponentPrivate->curState != OMX_StatePause ) {
                                                AMRENC_DPRINT("line %d:: Calling EmptyBufferDone\n",__LINE__);
                                                
#ifdef __PERF_INSTRUMENTATION__
                                                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                                                        pBufHeader->pBuffer,
                                                                        0,
                                                                        PERF_ModuleHLMM);
#endif

                                                pComponentPrivate->cbInfo.EmptyBufferDone( pComponentPrivate->pHandle,
                                                                                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                                                                                pBufHeader);
                                                pComponentPrivate->nEmptyBufferDoneCount++;

                                        }
                                        else {
                                                pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
                                        }
                                        goto EXIT;
                                }
                        }
                }else{
                        if((pBufHeader->nFlags&OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS){
                            if (pComponentPrivate->dasfMode == 0 && !pBufHeader->pMarkData) {
#ifdef __PERF_INSTRUMENTATION__
                                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                              pComponentPrivate->pInputBufferList->pBufHdr[0]->pBuffer,
                                              0,
                                              PERF_ModuleHLMM);
#endif

                                pComponentPrivate->cbInfo.EmptyBufferDone( pComponentPrivate->pHandle,
                                                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                                                pComponentPrivate->pInputBufferList->pBufHdr[0]);
                                pComponentPrivate->nEmptyBufferDoneCount++;
                                pComponentPrivate->ProcessingInputBuf--;

                                }
                        }
                        else{
               frameLength = NBAMRENC_INPUT_FRAME_SIZE;
                            nFrames=1;
                            }
                }
                if(nFrames >= 1){
                eError = NBAMRENC_GetCorrespondingLCMLHeader(pComponentPrivate,pBufHeader->pBuffer, OMX_DirInput, &pLcmlHdr);
                if (eError != OMX_ErrorNone) {
                        AMRENC_EPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                        goto EXIT;
                }

#ifdef __PERF_INSTRUMENTATION__
    /*For Steady State Instumentation*/
    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                      PREF(pBufHeader,pBuffer),
                      pComponentPrivate->pPortDef[OMX_DirInput]->nBufferSize,
                      PERF_ModuleCommonLayer);
#endif

/*---------------------------------------------------------------*/

                pComponentPrivate->nNumOfFramesSent = nFrames;

                phandle = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);

                if( (pLcmlHdr->pBufferParam->usNbFrames < nFrames) && (pLcmlHdr->pFrameParam!=NULL) ){
                        pBufParmsTemp = (OMX_U8*)pLcmlHdr->pFrameParam; /*This means that more memory need to be used*/
                        pBufParmsTemp -=EXTRA_BYTES;
                        OMX_NBMEMFREE_STRUCT(pBufParmsTemp);
                        pLcmlHdr->pFrameParam = NULL;
                        pBufParmsTemp =NULL;
                        OMX_DmmUnMap(phandle->dspCodec->hProc, /*Unmap DSP memory used*/
                                   (void*)pLcmlHdr->pBufferParam->pParamElem,
                                   pLcmlHdr->pDmmBuf->pReserved);
                        pLcmlHdr->pBufferParam->pParamElem = NULL;
                }

                if(pLcmlHdr->pFrameParam==NULL ){
                        NBAMRENC_OMX_MALLOC_SIZE(pBufParmsTemp, (sizeof(NBAMRENC_FrameStruct)*nFrames) + DSP_CACHE_ALIGNMENT,OMX_U8);
                        pLcmlHdr->pFrameParam =  (NBAMRENC_FrameStruct*)(pBufParmsTemp + EXTRA_BYTES);
                        eError = OMX_DmmMap(phandle->dspCodec->hProc,
                                        nFrames*sizeof(NBAMRENC_FrameStruct),
                                        (void*)pLcmlHdr->pFrameParam,
                                        (pLcmlHdr->pDmmBuf));
                        if (eError != OMX_ErrorNone){
                                AMRENC_EPRINT("OMX_DmmMap ERRROR!!!!\n\n");
                                goto EXIT;
                        }
                        pLcmlHdr->pBufferParam->pParamElem = (NBAMRENC_FrameStruct *)pLcmlHdr->pDmmBuf->pMapped; /*DSP Address*/
                }

                for(i=0;i<nFrames;i++){
                        (pLcmlHdr->pFrameParam+i)->usLastFrame = 0;
                }

                if((pBufHeader->nFlags & OMX_BUFFERFLAG_EOS)  == OMX_BUFFERFLAG_EOS) {
                        (pLcmlHdr->pFrameParam+(nFrames-1))->usLastFrame = OMX_BUFFERFLAG_EOS;
                        pComponentPrivate->InBuf_Eos_alreadysent = 1; /*TRUE*/
                        if(pComponentPrivate->dasfMode == 0) {
                               if(!pBufHeader->nFilledLen){
                                     pComponentPrivate->pOutputBufferList->pBufHdr[0]->nFlags |= OMX_BUFFERFLAG_EOS;
                               }
                                pComponentPrivate->cbInfo.EventHandler( pComponentPrivate->pHandle,
                                                                                        pComponentPrivate->pHandle->pApplicationPrivate,
                                                                                        OMX_EventBufferFlag,
                                                                                        pComponentPrivate->pOutputBufferList->pBufHdr[0]->nOutputPortIndex,
                                                                                        pComponentPrivate->pOutputBufferList->pBufHdr[0]->nFlags, NULL);
                        }
                        pBufHeader->nFlags = 0;
                }
                pLcmlHdr->pBufferParam->usNbFrames = nFrames;                
/*---------------------------------------------------------------*/
            /* Store time stamp information */
                /*pComponentPrivate->arrBufIndex[pComponentPrivate->IpBufindex] = pBufHeader->nTimeStamp;*/
                if (!pComponentPrivate->bFirstInputBufReceived) {
                    /* Reset TimeStamp when first input buffer received */
                    pComponentPrivate->TimeStamp = 0;
                    /* First Input buffer received */
                    pComponentPrivate->bFirstInputBufReceived = OMX_TRUE;
                }
            /* Store nTickCount information */
                        pComponentPrivate->arrTickCount[pComponentPrivate->IpBufindex] = pBufHeader->nTickCount;
                        pComponentPrivate->IpBufindex++;
                        pComponentPrivate->IpBufindex %= pComponentPrivate->pPortDef[OMX_DirOutput]->nBufferCountActual;

                        if (pComponentPrivate->curState == OMX_StateExecuting) {
                                if(!pComponentPrivate->bDspStoppedWhileExecuting) 
                                {
                                        if (!NBAMRENC_IsPending(pComponentPrivate,pBufHeader,OMX_DirInput)) {
                                                NBAMRENC_SetPending(pComponentPrivate,pBufHeader,OMX_DirInput,__LINE__);
/*              if (pLcmlHdr->buffer->nFilledLen != 0)
                    fwrite(pLcmlHdr->buffer->pBuffer, 1, pLcmlHdr->buffer->nFilledLen, fOut);
*/              
                /* fflush(fOut); */

/*              
                fclose(fOut);
*/

                                                eError = LCML_QueueBuffer( pLcmlHandle->pCodecinterfacehandle,
                                                                                                EMMCodecInputBuffer,
                                                                                                (OMX_U8 *)pBufHeader->pBuffer,
                                                                                                pBufHeader->nAllocLen,
                                                                                                pBufHeader->nFilledLen,
                                                                                                (OMX_U8 *) pLcmlHdr->pBufferParam,
                                                                                                sizeof(NBAMRENC_ParamStruct),
                                                                                                NULL);
                                                if (eError != OMX_ErrorNone) {
                                                        eError = OMX_ErrorHardware;
                                                        goto EXIT;
                                                }
                                                pComponentPrivate->lcml_nIpBuf++;
                                        }
                                    }
                        }
                        else if(pComponentPrivate->curState == OMX_StatePause){
                                pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
                        }
                        pComponentPrivate->ProcessingInputBuf--;
          }
/******************************************************************************/
        if(pBufHeader->pMarkData){
                if(pComponentPrivate->pOutputBufferList->pBufHdr[0]!=NULL) {
                       /* copy mark to output buffer header */
                       pComponentPrivate->pOutputBufferList->pBufHdr[0]->pMarkData = pBufHeader->pMarkData;
                       pComponentPrivate->pOutputBufferList->pBufHdr[0]->hMarkTargetComponent = pBufHeader->hMarkTargetComponent;
                }
                /* trigger event handler if we are supposed to */
                if(pBufHeader->hMarkTargetComponent == pComponentPrivate->pHandle && pBufHeader->pMarkData){
                        pComponentPrivate->cbInfo.EventHandler( pComponentPrivate->pHandle,
                                                                                        pComponentPrivate->pHandle->pApplicationPrivate,
                                                                                        OMX_EventMark,
                                                                                        0,
                                                                                        0,
                                                                                        pBufHeader->pMarkData);
                }
                if (pComponentPrivate->curState != OMX_StatePause && !NBAMRENC_IsPending(pComponentPrivate,pBufHeader,OMX_DirInput)) {
                    AMRENC_DPRINT("line %d:: Calling EmptyBufferDone\n",__LINE__);
                    
#ifdef __PERF_INSTRUMENTATION__
                            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                            pBufHeader->pBuffer,
                                            0,
                                            PERF_ModuleHLMM);
#endif

                    pComponentPrivate->cbInfo.EmptyBufferDone( pComponentPrivate->pHandle,
                                                                                    pComponentPrivate->pHandle->pApplicationPrivate,
                                                                                    pBufHeader);
                    pComponentPrivate->nEmptyBufferDoneCount++;

                }                
        }

        if (pComponentPrivate->bFlushInputPortCommandPending) {
                OMX_SendCommand(pComponentPrivate->pHandle,OMX_CommandFlush,0,NULL);
            }

    } else if (eDir == OMX_DirOutput) {
                /* Make sure that output buffer is issued to output stream only when
                * there is an outstanding input buffer already issued on input stream
                */

/***--------------------------------------****/
     pComponentPrivate->nUnhandledFillThisBuffers--;
     nFrames = pComponentPrivate->nNumOfFramesSent;
     if(nFrames == 0)
           nFrames = 1;

     eError = NBAMRENC_GetCorrespondingLCMLHeader(pComponentPrivate,pBufHeader->pBuffer, OMX_DirOutput, &pLcmlHdr);

     phandle = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);

     if( (pLcmlHdr->pBufferParam->usNbFrames < nFrames) && (pLcmlHdr->pFrameParam!=NULL) ){
                   pBufParmsTemp = (OMX_U8*)pLcmlHdr->pFrameParam; /*This means that more memory need to be used*/
                   pBufParmsTemp -=EXTRA_BYTES;
                   OMX_NBMEMFREE_STRUCT(pBufParmsTemp);
                   pLcmlHdr->pFrameParam = NULL;
                   pBufParmsTemp =NULL;
#ifndef UNDER_CE
                  OMX_DmmUnMap(phandle->dspCodec->hProc,
                                   (void*)pLcmlHdr->pBufferParam->pParamElem,
                                   pLcmlHdr->pDmmBuf->pReserved);
#endif

                   pLcmlHdr->pBufferParam->pParamElem = NULL;
         }

     if(pLcmlHdr->pFrameParam==NULL ){
                    NBAMRENC_OMX_MALLOC_SIZE(pBufParmsTemp, (sizeof(NBAMRENC_FrameStruct)*nFrames ) + DSP_CACHE_ALIGNMENT,OMX_U8);
                   pLcmlHdr->pFrameParam =  (NBAMRENC_FrameStruct*)(pBufParmsTemp + EXTRA_BYTES);
                   pLcmlHdr->pBufferParam->pParamElem = NULL;
#ifndef UNDER_CE
                 eError = OMX_DmmMap(phandle->dspCodec->hProc,
                                       nFrames*sizeof(NBAMRENC_FrameStruct),
                                       (void*)pLcmlHdr->pFrameParam,
                                       (pLcmlHdr->pDmmBuf));

                   if (eError != OMX_ErrorNone)
                   {
                               AMRENC_EPRINT("OMX_DmmMap ERRROR!!!!\n");
                               goto EXIT;
                   }
                   pLcmlHdr->pBufferParam->pParamElem = (NBAMRENC_FrameStruct *)pLcmlHdr->pDmmBuf->pMapped; /*DSP Address*/
#endif
         }

     pLcmlHdr->pBufferParam->usNbFrames = nFrames;

                        if (pComponentPrivate->curState == OMX_StateExecuting) {
                                if (!NBAMRENC_IsPending(pComponentPrivate,pBufHeader,OMX_DirOutput)) {
                                                NBAMRENC_SetPending(pComponentPrivate,pBufHeader,OMX_DirOutput,__LINE__);
                                                eError = LCML_QueueBuffer( pLcmlHandle->pCodecinterfacehandle,
                                                                                                EMMCodecOuputBuffer,
                                                                                                (OMX_U8 *)pBufHeader->pBuffer,
                                                                                                NBAMRENC_OUTPUT_FRAME_SIZE * nFrames,
                                                                                                0,
                                                                                                (OMX_U8 *) pLcmlHdr->pBufferParam,
                                                                                                sizeof(NBAMRENC_ParamStruct),
                                                                                                NULL);
                                                AMRENC_DPRINT("After QueueBuffer Line %d\n",__LINE__);
                                                if (eError != OMX_ErrorNone ) {
                                                        AMRENC_EPRINT ("%d :: Issuing DSP OP: Error Occurred\n",__LINE__);
                                                        eError = OMX_ErrorHardware;
                                                        goto EXIT;
                                                }
                                                pComponentPrivate->lcml_nOpBuf++;
                                                pComponentPrivate->num_Op_Issued++;
                                        }
                                }
                        else  if (pComponentPrivate->curState == OMX_StatePause){
                                pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
                        }
                        pComponentPrivate->ProcessingOutputBuf--;

                        if (pComponentPrivate->bFlushOutputPortCommandPending) {
                                  OMX_SendCommand( pComponentPrivate->pHandle,
                                  OMX_CommandFlush,
                                  1,NULL);
        }
        
    }
    else {
        eError = OMX_ErrorBadParameter;
    }

EXIT:
    AMRENC_DPRINT("%d :: Exiting from  NBAMRENC_HandleDataBufFromApp \n",__LINE__);
    AMRENC_DPRINT("%d :: Returning error %d\n",__LINE__,eError);

    return eError;
}

/*-------------------------------------------------------------------*/
/**
* NBAMRENC_GetBufferDirection () This function is used by the component
* to get the direction of the buffer
* @param eDir pointer will be updated with buffer direction
* @param pBufHeader pointer to the buffer to be requested to be filled
*
* @retval none
**/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE NBAMRENC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
                                                         OMX_DIRTYPE *eDir)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    AMRENC_COMPONENT_PRIVATE *pComponentPrivate = pBufHeader->pPlatformPrivate;
    OMX_U32 nBuf = 0;
    OMX_BUFFERHEADERTYPE *pBuf = NULL;
    OMX_U16 flag = 1,i = 0;
    AMRENC_DPRINT("%d :: Entering NBAMRENC_GetBufferDirection Function\n",__LINE__);
    /*Search this buffer in input buffers list */
    nBuf = pComponentPrivate->pInputBufferList->numBuffers;
    for(i=0; i<nBuf; i++) {
        pBuf = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf) {
            *eDir = OMX_DirInput;
            AMRENC_DPRINT("%d :: pBufHeader = %p is INPUT BUFFER pBuf = %p\n",__LINE__,pBufHeader,pBuf);
            flag = 0;
            goto EXIT;
        }
    }
    /*Search this buffer in output buffers list */
    nBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    for(i=0; i<nBuf; i++) {
        pBuf = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf) {
            *eDir = OMX_DirOutput;
            AMRENC_DPRINT("%d :: pBufHeader = %p is OUTPUT BUFFER pBuf = %p\n",__LINE__,pBufHeader,pBuf);
            flag = 0;
            goto EXIT;
        }
    }

    if (flag == 1) {
        AMRENC_EPRINT("%d :: Buffer %p is Not Found in the List\n",__LINE__, pBufHeader);
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }
EXIT:
    AMRENC_DPRINT("%d :: Exiting NBAMRENC_GetBufferDirection Function\n",__LINE__);
    AMRENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* -------------------------------------------------------------------*/
/**
  * NBAMRENC_GetCorrespondingLCMLHeader() function will be called by LCML_Callback
  * component to write the msg
  * @param *pBuffer,          Event which gives to details about USN status
  * @param NBAMRENC_LCML_BUFHEADERTYPE **ppLcmlHdr
  * @param  OMX_DIRTYPE eDir this gives direction of the buffer
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
 **/
/* -------------------------------------------------------------------*/
OMX_ERRORTYPE NBAMRENC_GetCorrespondingLCMLHeader(AMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                                                  OMX_U8 *pBuffer,
                                                  OMX_DIRTYPE eDir,
                                                  NBAMRENC_LCML_BUFHEADERTYPE **ppLcmlHdr)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    NBAMRENC_LCML_BUFHEADERTYPE *pLcmlBufHeader;
    OMX_S16 nIpBuf;
    OMX_S16 nOpBuf;
    OMX_S16 i;

    AMRENC_COMPONENT_PRIVATE *pComponentPrivate_CC;

    pComponentPrivate_CC = (AMRENC_COMPONENT_PRIVATE*) pComponentPrivate;
    nIpBuf = pComponentPrivate_CC->pInputBufferList->numBuffers;
    nOpBuf = pComponentPrivate_CC->pOutputBufferList->numBuffers;

    AMRENC_DPRINT("%d :: Entering NBAMRENC_GetCorrespondingLCMLHeader..\n",__LINE__);
    while (!pComponentPrivate_CC->bInitParamsInitialized) {
        AMRENC_DPRINT("%d :: Waiting for init to complete........\n",__LINE__);
#ifndef UNDER_CE
        sched_yield();
#else
        Sleep(1);
#endif
    }
    if(eDir == OMX_DirInput) {
        AMRENC_DPRINT("%d :: Entering NBAMRENC_GetCorrespondingLCMLHeader..\n",__LINE__);
        pLcmlBufHeader = pComponentPrivate_CC->pLcmlBufHeader[NBAMRENC_INPUT_PORT];
        for(i = 0; i < nIpBuf; i++) {
            AMRENC_DPRINT("%d :: pBuffer = %p\n",__LINE__,pBuffer);
            AMRENC_DPRINT("%d :: pLcmlBufHeader->buffer->pBuffer = %p\n",__LINE__,pLcmlBufHeader->buffer->pBuffer);
            if(pBuffer == pLcmlBufHeader->buffer->pBuffer) {
                *ppLcmlHdr = pLcmlBufHeader;
                 AMRENC_DPRINT("%d :: Corresponding Input LCML Header Found = %p\n",__LINE__,pLcmlBufHeader);
                 eError = OMX_ErrorNone;
                 goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } else if (eDir == OMX_DirOutput) {
        AMRENC_DPRINT("%d :: Entering NBAMRENC_GetCorrespondingLCMLHeader..\n",__LINE__);
        pLcmlBufHeader = pComponentPrivate_CC->pLcmlBufHeader[NBAMRENC_OUTPUT_PORT];
        for(i = 0; i < nOpBuf; i++) {
            AMRENC_DPRINT("%d :: pBuffer = %p\n",__LINE__,pBuffer);
            AMRENC_DPRINT("%d :: pLcmlBufHeader->buffer->pBuffer = %p\n",__LINE__,pLcmlBufHeader->buffer->pBuffer);
            if(pBuffer == pLcmlBufHeader->buffer->pBuffer) {
                *ppLcmlHdr = pLcmlBufHeader;
                 AMRENC_DPRINT("%d :: Corresponding Output LCML Header Found = %p\n",__LINE__,pLcmlBufHeader);
                 eError = OMX_ErrorNone;
                 goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } else {
      AMRENC_EPRINT("%d :: Invalid Buffer Type :: exiting...\n",__LINE__);
      eError = OMX_ErrorUndefined;
    }

EXIT:
    AMRENC_DPRINT("%d :: Exiting NBAMRENC_GetCorrespondingLCMLHeader..\n",__LINE__);
    AMRENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* -------------------------------------------------------------------*/
/**
  *  NBAMRENC_LCMLCallback() will be called LCML component to write the msg
  *
  * @param event                 Event which gives to details about USN status
  * @param void * args        //    args [0] //bufType;
                              //    args [1] //arm address fpr buffer
                              //    args [2] //BufferSize;
                              //    args [3]  //arm address for param
                              //    args [4] //ParamSize;
                              //    args [6] //LCML Handle
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
 **/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE NBAMRENC_LCMLCallback (TUsnCodecEvent event,void * args[10])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8 *pBuffer = args[1];
    NBAMRENC_LCML_BUFHEADERTYPE *pLcmlHdr;
    OMX_U16 i,index,frameLength, length;
    OMX_COMPONENTTYPE *pHandle;
    LCML_DSP_INTERFACE *pLcmlHandle;
    OMX_U8 nFrames;
    double num_samples = 0;
    OMX_U32 buffer_duration = 0;

    NBAMRENC_BUFDATA* OutputFrames = NULL;

    AMRENC_COMPONENT_PRIVATE* pComponentPrivate_CC = NULL;
    pComponentPrivate_CC = (AMRENC_COMPONENT_PRIVATE*)((LCML_DSP_INTERFACE*)args[6])->pComponentPrivate;
    pHandle = pComponentPrivate_CC->pHandle;


#ifdef AMRENC_DEBUG
    switch(event) {

        case EMMCodecDspError:
            AMRENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecDspError\n");
            break;

        case EMMCodecInternalError:
            AMRENC_EPRINT("[LCML CALLBACK EVENT]  EMMCodecInternalError\n");
            break;

        case EMMCodecInitError:
            AMRENC_EPRINT("[LCML CALLBACK EVENT]  EMMCodecInitError\n");
            break;

        case EMMCodecDspMessageRecieved:
            AMRENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecDspMessageRecieved\n");
            break;

        case EMMCodecBufferProcessed:
            AMRENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecBufferProcessed\n");
            break;

        case EMMCodecProcessingStarted:
            AMRENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingStarted\n");
            break;

        case EMMCodecProcessingPaused:
            AMRENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingPaused\n");
            break;

        case EMMCodecProcessingStoped:
            AMRENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingStoped\n");
            break;

        case EMMCodecProcessingEof:
            AMRENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingEof\n");
            break;

        case EMMCodecBufferNotProcessed:
            AMRENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecBufferNotProcessed\n");
            break;

        case EMMCodecAlgCtrlAck:
            AMRENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecAlgCtrlAck\n");
            break;

        case EMMCodecStrmCtrlAck:
            AMRENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecStrmCtrlAck\n");
            break;
    }
#endif

    if(event == EMMCodecBufferProcessed)
    {
    
        if((OMX_U32)args[0] == EMMCodecInputBuffer) {
                AMRENC_DPRINT("%d :: INPUT: pBuffer = %p\n",__LINE__, pBuffer);
            eError = NBAMRENC_GetCorrespondingLCMLHeader(pComponentPrivate_CC,pBuffer, OMX_DirInput, &pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                AMRENC_EPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }
#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate_CC->pPERFcomp,
                               PREF(pLcmlHdr->buffer,pBuffer),
                               0,
                               PERF_ModuleCommonLayer);
#endif

            NBAMRENC_ClearPending(pComponentPrivate_CC,pLcmlHdr->buffer,OMX_DirInput,__LINE__);

#ifdef __PERF_INSTRUMENTATION__
                                PERF_SendingFrame(pComponentPrivate_CC->pPERFcomp,
                                              pLcmlHdr->buffer->pBuffer,
                                              0,
                                              PERF_ModuleHLMM);
#endif
                pComponentPrivate_CC->cbInfo.EmptyBufferDone( pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           pLcmlHdr->buffer);
                pComponentPrivate_CC->nEmptyBufferDoneCount++;
                pComponentPrivate_CC->nOutStandingEmptyDones--;
                pComponentPrivate_CC->lcml_nIpBuf--;
                pComponentPrivate_CC->app_nBuf++;
                
        pComponentPrivate_CC->nOutStandingEmptyDones++;
            
        } else if((OMX_U32)args[0] == EMMCodecOuputBuffer) {

            if (!NBAMRENC_IsValid(pComponentPrivate_CC,pBuffer,OMX_DirOutput)) {

                for (i=0; i < pComponentPrivate_CC->pOutputBufferList->numBuffers; i++) {
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate_CC->pPERFcomp,
                                  pComponentPrivate_CC->pOutputBufferList->pBufHdr[i]->pBuffer,
                                  pComponentPrivate_CC->pOutputBufferList->pBufHdr[i]->nFilledLen,
                                  PERF_ModuleHLMM);
#endif
                    pComponentPrivate_CC->cbInfo.FillBufferDone (pComponentPrivate_CC->pHandle,
                                                          pComponentPrivate_CC->pHandle->pApplicationPrivate,
                                                          pComponentPrivate_CC->pOutputBufferList->pBufHdr[i++]
                                                          );
                }
            } else {

            AMRENC_DPRINT("%d :: OUTPUT: pBuffer = %p\n",__LINE__, pBuffer);

            pComponentPrivate_CC->nOutStandingFillDones++;

            eError = NBAMRENC_GetCorrespondingLCMLHeader(pComponentPrivate_CC, pBuffer, OMX_DirOutput, &pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                AMRENC_EPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }

            AMRENC_DPRINT("%d :: Output: pLcmlHdr->buffer->pBuffer = %p\n",__LINE__, pLcmlHdr->buffer->pBuffer);
            pLcmlHdr->buffer->nFilledLen = (OMX_U32)args[8];
            AMRENC_DPRINT("%d :: Output: pBuffer = %ld\n",__LINE__, pLcmlHdr->buffer->nFilledLen);
#if 0
            pLcmlHdr->buffer->nFilledLen = 32;
            AMRENC_DPRINT("%d :: Output: ::RESET:: pBuffer->nFilledLen = %ld\n",__LINE__, pLcmlHdr->buffer->nFilledLen);
#endif

#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate_CC->pPERFcomp,
                               PREF(pLcmlHdr->buffer,pBuffer),
                               PREF(pLcmlHdr->buffer,nFilledLen),
                               PERF_ModuleCommonLayer);

            pComponentPrivate_CC->nLcml_nCntOpReceived++;

            if ((pComponentPrivate_CC->nLcml_nCntIp >= 1) && (pComponentPrivate_CC->nLcml_nCntOpReceived == 1)) {
                PERF_Boundary(pComponentPrivate_CC->pPERFcomp,
                              PERF_BoundaryStart | PERF_BoundarySteadyState);
            }
#endif

                NBAMRENC_ClearPending(pComponentPrivate_CC,pLcmlHdr->buffer,OMX_DirOutput,__LINE__);

                pComponentPrivate_CC->LastOutbuf = pLcmlHdr->buffer;
                if(!pLcmlHdr->pBufferParam->usNbFrames){
                        pLcmlHdr->pBufferParam->usNbFrames++;
                }
                if((pComponentPrivate_CC->frameMode == NBAMRENC_MIMEMODE)&&(pLcmlHdr->buffer->nFilledLen)) {
                       nFrames = (OMX_U8)( pLcmlHdr->buffer->nFilledLen / NBAMRENC_OUTPUT_BUFFER_SIZE_MIME);
                       frameLength=0;
                       length=0;
                       for(i=0;i<nFrames;i++){
                             index = (pLcmlHdr->buffer->pBuffer[i*NBAMRENC_OUTPUT_BUFFER_SIZE_MIME] >> 3) & 0x0F;
                             if(pLcmlHdr->buffer->nFilledLen == 0)
                                  length = 0;
                             else
                                  length = (OMX_U16)pComponentPrivate_CC->amrMimeBytes[index];
                             if (i){
                                   memmove( pLcmlHdr->buffer->pBuffer + frameLength,
                                            pLcmlHdr->buffer->pBuffer + (i * NBAMRENC_OUTPUT_BUFFER_SIZE_MIME),
                                            length);
                             }
                             frameLength += length;
                        }
                        pLcmlHdr->buffer->nFilledLen= frameLength;
                }
                 else if((pComponentPrivate_CC->frameMode == NBAMRENC_IF2)&&(pLcmlHdr->buffer->nFilledLen)) {
                       nFrames = (OMX_U8)( pLcmlHdr->buffer->nFilledLen / NBAMRENC_OUTPUT_BUFFER_SIZE_IF2);
                       frameLength=0;
                       length=0;
                       for(i=0;i<nFrames;i++){
                             index = (pLcmlHdr->buffer->pBuffer[i*NBAMRENC_OUTPUT_BUFFER_SIZE_IF2]) & 0x0F;
                             if(pLcmlHdr->buffer->nFilledLen == 0)
                                  length = 0;
                             else
                                  length = (OMX_U16)pComponentPrivate_CC->amrIf2Bytes[index];
                             if (i){
                                   memmove( pLcmlHdr->buffer->pBuffer + frameLength,
                                            pLcmlHdr->buffer->pBuffer + (i * NBAMRENC_OUTPUT_BUFFER_SIZE_IF2),
                                            length);
                             }
                             frameLength += length;
                        }
                        pLcmlHdr->buffer->nFilledLen= frameLength;
                } else{
                        if ((pComponentPrivate_CC->efrMode == 1)&&(pLcmlHdr->buffer->nFilledLen)){
                           nFrames = pLcmlHdr->buffer->nFilledLen/NBAMRENC_OUTPUT_BUFFER_SIZE_EFR;
                            }
                        else    
                           nFrames = pLcmlHdr->buffer->nFilledLen/NBAMRENC_OUTPUT_FRAME_SIZE;
                      }

                OutputFrames = pLcmlHdr->buffer->pOutputPortPrivate;
                OutputFrames->nFrames = nFrames;

                
                if( !pComponentPrivate_CC->dasfMode){
                        /* Copying time stamp information to output buffer */
                        /*pLcmlHdr->buffer->nTimeStamp = (OMX_TICKS)pComponentPrivate_CC->arrBufIndex[pComponentPrivate_CC->OpBufindex];*/
                        pLcmlHdr->buffer->nTimeStamp = pComponentPrivate_CC->TimeStamp;
                        num_samples = pLcmlHdr->buffer->nFilledLen / (pComponentPrivate_CC->pcmParams->nChannels
                                                                      * (pComponentPrivate_CC->pcmParams->nBitPerSample / 8));
                        buffer_duration = (num_samples / pComponentPrivate_CC->pcmParams->nSamplingRate) * 10000;
                        /* Update time stamp information */
                        pComponentPrivate_CC->TimeStamp += (OMX_TICKS)buffer_duration;
                        /*pComponentPrivate_CC->TimeStamp += 20 * nFrames;*/
                        /* Copying nTickCount information to output buffer */
                        pLcmlHdr->buffer->nTickCount = pComponentPrivate_CC->arrTickCount[pComponentPrivate_CC->OpBufindex];
                        pComponentPrivate_CC->OpBufindex++;
                        pComponentPrivate_CC->OpBufindex %= pComponentPrivate_CC->pPortDef[OMX_DirOutput]->nBufferCountActual;           
                }
                
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingBuffer(pComponentPrivate_CC->pPERFcomp,
                                    pLcmlHdr->buffer->pBuffer,
                                    pLcmlHdr->buffer->nFilledLen,
                                    PERF_ModuleHLMM);
#endif
                /* Non Multi Frame Mode has been tested here */ 

                pComponentPrivate_CC->nFillBufferDoneCount++;
                pComponentPrivate_CC->nOutStandingFillDones--;
                pComponentPrivate_CC->lcml_nOpBuf--;
                pComponentPrivate_CC->app_nBuf++;

                pComponentPrivate_CC->cbInfo.FillBufferDone( pHandle,
                                            pHandle->pApplicationPrivate,
                                            pLcmlHdr->buffer);

                AMRENC_DPRINT("%d :: Incrementing app_nBuf = %ld\n",__LINE__,pComponentPrivate_CC->app_nBuf);
            }
        }
        }
    else if (event == EMMCodecStrmCtrlAck) {
        AMRENC_DPRINT("%d :: GOT MESSAGE USN_DSPACK_STRMCTRL \n",__LINE__);
        if (args[1] == (void *)USN_STRMCMD_FLUSH) {
            pHandle = pComponentPrivate_CC->pHandle;
            if ( args[2] == (void *)EMMCodecInputBuffer) {
                if (args[0] == (void *)USN_ERR_NONE ) {
                    AMRENC_DPRINT("Flushing input port %d\n",__LINE__);

                    for (i=0; i < pComponentPrivate_CC->nNumInputBufPending; i++) {

#ifdef __PERF_INSTRUMENTATION__
                            PERF_SendingFrame(pComponentPrivate_CC->pPERFcomp,
                                              pComponentPrivate_CC->pInputBufferList->pBufHdr[i]->pBuffer,
                                              0,
                                              PERF_ModuleHLMM);
#endif

                        pComponentPrivate_CC->cbInfo.EmptyBufferDone (
                                       pHandle,
                                       pHandle->pApplicationPrivate,
                                       pComponentPrivate_CC->pInputBufHdrPending[i]
                                       );
                    pComponentPrivate_CC->nEmptyBufferDoneCount++;
                    pComponentPrivate_CC->nOutStandingEmptyDones--;

                    }
                    pComponentPrivate_CC->nNumInputBufPending=0;
                    pComponentPrivate_CC->cbInfo.EventHandler(
                         pHandle, pHandle->pApplicationPrivate,
                         OMX_EventCmdComplete, OMX_CommandFlush,NBAMRENC_INPUT_PORT, NULL);
                } else {
                     AMRENC_EPRINT("LCML reported error while flushing input port\n");
                     goto EXIT;
                }
            }
            else if ( args[2] == (void *)EMMCodecOuputBuffer) {
                if (args[0] == (void *)USN_ERR_NONE ) {
                    AMRENC_DPRINT("Flushing output port %d\n",__LINE__);

                    for (i=0; i < pComponentPrivate_CC->nNumOutputBufPending; i++) {
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate_CC->pPERFcomp,
                                      pComponentPrivate_CC->pOutputBufferList->pBufHdr[i]->pBuffer,
                                      pComponentPrivate_CC->pOutputBufferList->pBufHdr[i]->nFilledLen,
                                      PERF_ModuleHLMM);
#endif
                        pComponentPrivate_CC->cbInfo.FillBufferDone (
                                       pHandle,
                                       pHandle->pApplicationPrivate,
                                       pComponentPrivate_CC->pOutputBufHdrPending[i]
                                       );
                    pComponentPrivate_CC->nFillBufferDoneCount++; 
                    pComponentPrivate_CC->nOutStandingFillDones--;

                    }
                    pComponentPrivate_CC->nNumOutputBufPending=0;
                    pComponentPrivate_CC->cbInfo.EventHandler(
                             pHandle, pHandle->pApplicationPrivate,
                             OMX_EventCmdComplete, OMX_CommandFlush,NBAMRENC_OUTPUT_PORT, NULL);
                } else {
                    AMRENC_EPRINT("LCML reported error while flushing output port\n");
                    goto EXIT;
                }
            }
        }
    }
    else if(event == EMMCodecProcessingStoped) {

        pthread_mutex_lock(&pComponentPrivate_CC->codecStop_mutex);
        if(pComponentPrivate_CC->codecStop_waitingsignal == 0){
            pComponentPrivate_CC->codecStop_waitingsignal = 1;             
            pthread_cond_signal(&pComponentPrivate_CC->codecStop_threshold);
            AMRENC_EPRINT("stop ack. received. stop waiting for sending disable command completed\n");
        }
        pthread_mutex_unlock(&pComponentPrivate_CC->codecStop_mutex);
        
        AMRENC_DPRINT("%d :: GOT MESSAGE USN_DSPACK_STOP \n",__LINE__);
        
        for (i=0; i < pComponentPrivate_CC->pInputBufferList->numBuffers; i++) {

            if (pComponentPrivate_CC->pInputBufferList->bBufferPending[i]) {

#ifdef __PERF_INSTRUMENTATION__
                            PERF_SendingFrame(pComponentPrivate_CC->pPERFcomp,
                                              pComponentPrivate_CC->pInputBufferList->pBufHdr[i]->pBuffer,
                                              0,
                                              PERF_ModuleHLMM);
#endif
                    
                   pComponentPrivate_CC->cbInfo.EmptyBufferDone (
                                       pComponentPrivate_CC->pHandle,
                                       pComponentPrivate_CC->pHandle->pApplicationPrivate,
                                       pComponentPrivate_CC->pInputBufferList->pBufHdr[i]
                                       );
                   pComponentPrivate_CC->nEmptyBufferDoneCount++;
                   pComponentPrivate_CC->nOutStandingEmptyDones--;
                   NBAMRENC_ClearPending(pComponentPrivate_CC, pComponentPrivate_CC->pInputBufferList->pBufHdr[i], OMX_DirInput,__LINE__);
            }
        }

        for (i=0; i < pComponentPrivate_CC->pOutputBufferList->numBuffers; i++) {
            if (pComponentPrivate_CC->pOutputBufferList->bBufferPending[i]) {
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate_CC->pPERFcomp,
                                      pComponentPrivate_CC->pOutputBufferList->pBufHdr[i]->pBuffer,
                                      pComponentPrivate_CC->pOutputBufferList->pBufHdr[i]->nFilledLen,
                                      PERF_ModuleHLMM);
#endif
                pComponentPrivate_CC->cbInfo.FillBufferDone (
                                       pComponentPrivate_CC->pHandle,
                                       pComponentPrivate_CC->pHandle->pApplicationPrivate,
                                       pComponentPrivate_CC->pOutputBufferList->pBufHdr[i]
                                       );
                pComponentPrivate_CC->nFillBufferDoneCount++;     
                pComponentPrivate_CC->nOutStandingFillDones--;

                NBAMRENC_ClearPending(pComponentPrivate_CC, pComponentPrivate_CC->pOutputBufferList->pBufHdr[i], OMX_DirOutput,__LINE__);
            }
        }

        if (!pComponentPrivate_CC->bNoIdleOnStop) {
            
            pComponentPrivate_CC->nNumOutputBufPending=0;
    
            pComponentPrivate_CC->ProcessingInputBuf=0;
            pComponentPrivate_CC->ProcessingOutputBuf=0;
            
            pComponentPrivate_CC->nHoldLength = 0;
        pComponentPrivate_CC->InBuf_Eos_alreadysent  =0;
        
            OMX_NBMEMFREE_STRUCT(pComponentPrivate_CC->pHoldBuffer);
            OMX_NBMEMFREE_STRUCT(pComponentPrivate_CC->iMMFDataLastBuffer);
        
            pComponentPrivate_CC->curState = OMX_StateIdle;
#ifdef RESOURCE_MANAGER_ENABLED
            eError = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_NBAMR_Encoder_COMPONENT, OMX_StateIdle, 3456, NULL);
#endif

            if (pComponentPrivate_CC->bPreempted == 0) {
                pComponentPrivate_CC->cbInfo.EventHandler(pComponentPrivate_CC->pHandle,
                                                        pComponentPrivate_CC->pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandStateSet,
                                                        pComponentPrivate_CC->curState,
                                                        NULL);
            }
            else{
                                pComponentPrivate_CC->cbInfo.EventHandler(pComponentPrivate_CC->pHandle,
                                                        pComponentPrivate_CC->pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorResourcesPreempted,
                                                        OMX_TI_ErrorSevere,
                                                        NULL);
            }   
            
            
        }
        else {
            pComponentPrivate_CC->bNoIdleOnStop= OMX_FALSE;
            pComponentPrivate_CC->bDspStoppedWhileExecuting = OMX_TRUE; 
        }
    }
    else if(event == EMMCodecDspMessageRecieved) {
        AMRENC_DPRINT("%d :: commandedState  = %ld\n",__LINE__,(OMX_U32)args[0]);
        AMRENC_DPRINT("%d :: arg1 = %ld\n",__LINE__,(OMX_U32)args[1]);
        AMRENC_DPRINT("%d :: arg2 = %ld\n",__LINE__,(OMX_U32)args[2]);

        if(0x0500 == (OMX_U32)args[2]) {
            AMRENC_DPRINT("%d :: EMMCodecDspMessageRecieved\n",__LINE__);
        }
    }
    else if(event == EMMCodecAlgCtrlAck) {
        AMRENC_DPRINT("%d :: GOT MESSAGE USN_DSPACK_ALGCTRL \n",__LINE__);
    }
        else if (event == EMMCodecDspError) {
#ifdef _ERROR_PROPAGATION__
            /* Cheking for MMU_fault */
            if(((int)args[4] == USN_ERR_UNKNOWN_MSG) && (args[5] ==(void*) NULL)) {
                pComponentPrivate_CC->bIsInvalidState=OMX_TRUE;
                pComponentPrivate_CC->curState = OMX_StateInvalid;
                pHandle = pComponentPrivate_CC->pHandle;
                pComponentPrivate_CC->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorInvalidState,
                                                           OMX_TI_ErrorSevere,
                                                           NULL);
            }
#endif
                if(((int)args[4] == USN_ERR_WARNING) && ((int)args[5] == IUALG_WARN_PLAYCOMPLETED)) {
                        pHandle = pComponentPrivate_CC->pHandle;
                        AMRENC_DPRINT("%d :: GOT MESSAGE IUALG_WARN_PLAYCOMPLETED\n",__LINE__);
                        AMRENC_DPRINT("IUALG_WARN_PLAYCOMPLETED Received\n");
                        if(pComponentPrivate_CC->LastOutbuf!=NULL){
                                    pComponentPrivate_CC->LastOutbuf->nFlags |= OMX_BUFFERFLAG_EOS;
                        }
                        pComponentPrivate_CC->cbInfo.EventHandler(pComponentPrivate_CC->pHandle,
                                                                pComponentPrivate_CC->pHandle->pApplicationPrivate,
                                                                OMX_EventBufferFlag,
                                                                (OMX_U32)NULL,
                                                                OMX_BUFFERFLAG_EOS,
                                                                NULL);
                }
                if((int)args[5] == IUALG_ERR_GENERAL) {
                        char *pArgs = "damedesuStr";
                        AMRENC_EPRINT( "Algorithm error. Cannot continue" );
                        AMRENC_EPRINT("%d :: arg5 = %p\n",__LINE__,args[5]);                        
                        AMRENC_EPRINT("%d :: LCML_Callback: IUALG_ERR_GENERAL\n",__LINE__);
                        pHandle = pComponentPrivate_CC->pHandle;
                        pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate_CC->pLcmlHandle;
#ifndef UNDER_CE

                        pComponentPrivate_CC->bIsStopping = 1;
                        if (pComponentPrivate_CC->codecStop_waitingsignal == 0){ 
                            pthread_mutex_lock(&pComponentPrivate_CC->codecStop_mutex);     
                        }

                        eError = LCML_ControlCodec(
                                ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                MMCodecControlStop,(void *)pArgs);

                        if (pComponentPrivate_CC->codecStop_waitingsignal == 0){
                            pthread_cond_wait(&pComponentPrivate_CC->codecStop_threshold, &pComponentPrivate_CC->codecStop_mutex);
                            pComponentPrivate_CC->codecStop_waitingsignal = 0;
                            pthread_mutex_unlock(&pComponentPrivate_CC->codecStop_mutex);
                        }

                        if(eError != OMX_ErrorNone) {
                                AMRENC_EPRINT("%d: Error Occurred in Codec Stop..\n",
                                __LINE__);
                                goto EXIT;
                        }
                        AMRENC_DPRINT("%d :: AMRENC: Codec has been Stopped here\n",__LINE__);
                        pComponentPrivate_CC->curState = OMX_StateIdle;
                        pComponentPrivate_CC->cbInfo.EventHandler(
                                        pHandle, pHandle->pApplicationPrivate,
                                        OMX_EventCmdComplete, OMX_ErrorNone,0, NULL);
#else
                        pComponentPrivate_CC->cbInfo.EventHandler(pHandle, 
                                                                    pHandle->pApplicationPrivate,
                                                                    OMX_EventError, 
                                                                    OMX_ErrorUndefined,
                                                                    OMX_TI_ErrorSevere, 
                                                                    NULL);
#endif
                }
                if( (int)args[5] == IUALG_ERR_DATA_CORRUPT ){
                        char *pArgs = "damedesuStr";
                        AMRENC_EPRINT( "Algorithm error. Corrupt data" );
                        AMRENC_EPRINT("%d :: arg5 = %p\n",__LINE__,args[5]);
                        AMRENC_EPRINT("%d :: LCML_Callback: IUALG_ERR_DATA_CORRUPT\n",__LINE__);
                        pHandle = pComponentPrivate_CC->pHandle;
                        pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate_CC->pLcmlHandle;
#ifndef UNDER_CE
                        pComponentPrivate_CC->bIsStopping = 1;
                        if (pComponentPrivate_CC->codecStop_waitingsignal == 0){ 
                            pthread_mutex_lock(&pComponentPrivate_CC->codecStop_mutex);     
                        }

                        eError = LCML_ControlCodec(
                                ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                        MMCodecControlStop,(void *)pArgs);

                        if (pComponentPrivate_CC->codecStop_waitingsignal == 0){
                            pthread_cond_wait(&pComponentPrivate_CC->codecStop_threshold, &pComponentPrivate_CC->codecStop_mutex);
                            pComponentPrivate_CC->codecStop_waitingsignal = 0;
                            pthread_mutex_unlock(&pComponentPrivate_CC->codecStop_mutex);
                        }

                        if(eError != OMX_ErrorNone) {
                                AMRENC_EPRINT("%d: Error Occurred in Codec Stop..\n",__LINE__);
                                goto EXIT;
                        }
                        AMRENC_DPRINT("%d :: AMRENC: Codec has been Stopped here\n",__LINE__);
                        pComponentPrivate_CC->curState = OMX_StateIdle;
                        pComponentPrivate_CC->cbInfo.EventHandler(
                                pHandle, pHandle->pApplicationPrivate,
                                OMX_EventCmdComplete, OMX_ErrorNone,0, NULL);
#else
                        pComponentPrivate_CC->cbInfo.EventHandler(pHandle, 
                                                                    pHandle->pApplicationPrivate,
                                                                    OMX_EventError, 
                                                                    OMX_ErrorUndefined,
                                                                    OMX_TI_ErrorSevere, 
                                                                    NULL);
#endif
                }
        }
        else if (event == EMMCodecProcessingPaused) {
            pComponentPrivate_CC->nUnhandledEmptyThisBuffers = 0;
            pComponentPrivate_CC->nUnhandledFillThisBuffers = 0;
            pComponentPrivate_CC->curState = OMX_StatePause;
            pComponentPrivate_CC->cbInfo.EventHandler( pComponentPrivate_CC->pHandle,
                                                    pComponentPrivate_CC->pHandle->pApplicationPrivate,
                                                    OMX_EventCmdComplete,
                                                    OMX_CommandStateSet,
                                                    pComponentPrivate_CC->curState,
                                                    NULL);
        }
#ifdef _ERROR_PROPAGATION__
        else if (event ==EMMCodecInitError){
             /* Cheking for MMU_fault */
             if(((int)args[4] == USN_ERR_UNKNOWN_MSG) && (args[5] == (void*)NULL)) {
                    pComponentPrivate_CC->bIsInvalidState=OMX_TRUE;
                    pComponentPrivate_CC->curState = OMX_StateInvalid;
                    pHandle = pComponentPrivate_CC->pHandle;
                    pComponentPrivate_CC->cbInfo.EventHandler(pHandle,
                                   pHandle->pApplicationPrivate,
                                   OMX_EventError,
                                   OMX_ErrorInvalidState,
                                   OMX_TI_ErrorSevere,
                                   NULL);
        }
    }
    else if (event ==EMMCodecInternalError){
        /* Cheking for MMU_fault */
        if(((int)args[4] == USN_ERR_UNKNOWN_MSG) && (args[5] == (void*)NULL)) {
            AMRENC_EPRINT("%d :: UTIL: MMU_Fault \n",__LINE__);
            pComponentPrivate_CC->bIsInvalidState=OMX_TRUE;
            pComponentPrivate_CC->curState = OMX_StateInvalid;
            pHandle = pComponentPrivate_CC->pHandle;
            pComponentPrivate_CC->cbInfo.EventHandler(pHandle,
                                   pHandle->pApplicationPrivate,
                                   OMX_EventError,
                                   OMX_ErrorInvalidState,
                                   OMX_TI_ErrorSevere,
                                   NULL);
        }

    }
#endif
EXIT:
    AMRENC_DPRINT("%d :: Exiting the NBAMRENC_LCMLCallback Function\n",__LINE__);
    AMRENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);

    return eError;
}

/* ================================================================================= */
/**
  *  NBAMRENC_GetLCMLHandle()
  *
  * @retval OMX_HANDLETYPE
  */
/* ================================================================================= */
#ifndef UNDER_CE
OMX_HANDLETYPE NBAMRENC_GetLCMLHandle(AMRENC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE (*fpGetHandle)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    void *handle;
    char *error;
    AMRENC_COMPONENT_PRIVATE* pComponentPrivate_CC = NULL;

    AMRENC_DPRINT("%d :: Entering NBAMRENC_GetLCMLHandle..\n",__LINE__);
    handle = dlopen("libLCML.so", RTLD_LAZY);
    if (!handle) {
        fputs(dlerror(), stderr);
        goto EXIT;
    }
    fpGetHandle = dlsym (handle, "GetHandle");
    if ((error = dlerror()) != NULL) {
        fputs(error, stderr);
        goto EXIT;
    }
    eError = (*fpGetHandle)(&pHandle);
    if(eError != OMX_ErrorNone) {
        eError = OMX_ErrorUndefined;
        AMRENC_EPRINT("%d :: OMX_ErrorUndefined...\n",__LINE__);
        pHandle = NULL;
        goto EXIT;
    }
    pComponentPrivate_CC = (AMRENC_COMPONENT_PRIVATE*)pComponentPrivate;
    ((LCML_DSP_INTERFACE*)pHandle)->pComponentPrivate = pComponentPrivate;
    
    pComponentPrivate->ptrLibLCML=handle;           /* saving LCML lib pointer  */
    
EXIT:
    AMRENC_DPRINT("%d :: Exiting NBAMRENC_GetLCMLHandle..\n",__LINE__);
    AMRENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return pHandle;
}

#else
/*WINDOWS Explicit dll load procedure*/
OMX_HANDLETYPE NBAMRENC_GetLCMLHandle(AMRENC_COMPONENT_PRIVATE *pComponentPrivate)
{
    typedef OMX_ERRORTYPE (*LPFNDLLFUNC1)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    OMX_ERRORTYPE eError;
    LPFNDLLFUNC1 fpGetHandle1;
        

    if (!fpGetHandle1) {
      // handle the error
      return pHandle;
    }
    // call the function
    eError = fpGetHandle1(&pHandle);
    if(eError != OMX_ErrorNone) {
        eError = OMX_ErrorUndefined;
        AMRENC_EPRINT("eError != OMX_ErrorNone...\n");
        pHandle = NULL;
        return pHandle;
    }

    ((LCML_DSP_INTERFACE*)pHandle)->pComponentPrivate = pComponentPrivate;
    return pHandle;
}
#endif

/* ================================================================================= */
/**
* @fn NBAMRENC_SetPending() description for NBAMRENC_SetPending
NBAMRENC_SetPending().
This component is called when a buffer is queued to the LCML
* @param pComponent  handle for this instance of the component
*
* @pre
*
* @post
*
* @return OMX_ERRORTYPE
*/
/* ================================================================================ */
void NBAMRENC_SetPending(AMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                         OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber)
{
    OMX_U16 i;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 1;
                AMRENC_DPRINT("****INPUT BUFFER %d IS PENDING Line %ld******\n",i,lineNumber);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 1;
                AMRENC_DPRINT("****OUTPUT BUFFER %d IS PENDING Line %ld*****\n",i,lineNumber);
            }
        }
    }
}
/* ================================================================================= */
/**
* @fn NBAMRENC_ClearPending() description for NBAMRENC_ClearPending
NBAMRENC_ClearPending().
This component is called when a buffer is returned from the LCML
* @param pComponent  handle for this instance of the component
*
* @pre
*
* @post
*
* @return OMX_ERRORTYPE
*/
/* ================================================================================ */
void NBAMRENC_ClearPending(AMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                           OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber)
{
    OMX_U16 i;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 0;
                AMRENC_DPRINT("****INPUT BUFFER %d IS RECLAIMED Line %ld*****\n",i,lineNumber);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 0;
                AMRENC_DPRINT("****OUTPUT BUFFER %d IS RECLAIMED Line %ld*****\n",i,lineNumber);
            }
        }
    }
}
/* ================================================================================= */
/**
* @fn NBAMRENC_IsPending() description for NBAMRENC_IsPending
NBAMRENC_IsPending().
This method returns the pending status to the buffer
* @param pComponent  handle for this instance of the component
*
* @pre
*
* @post
*
* @return OMX_ERRORTYPE
*/
/* ================================================================================ */
OMX_U32 NBAMRENC_IsPending(AMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                           OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir)
{
    OMX_U16 i;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                return pComponentPrivate->pInputBufferList->bBufferPending[i];
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            AMRENC_DPRINT("pBufHdr = %p\n",pBufHdr);
            AMRENC_DPRINT("pOutputBufferList->pBufHdr[i] = %p\n",pComponentPrivate->pOutputBufferList->pBufHdr[i]);
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                AMRENC_DPRINT("returning %lx\n",pComponentPrivate->pOutputBufferList->bBufferPending[i]);
                return pComponentPrivate->pOutputBufferList->bBufferPending[i];
            }
        }
    }
    return -1;
}
/* ================================================================================= */
/**
* @fn NBAMRENC_IsValid() description for NBAMRENC_IsValid
NBAMRENC_IsValid().
This method checks to see if a buffer returned from the LCML is valid.
* @param pComponent  handle for this instance of the component
*
* @pre
*
* @post
*
* @return OMX_ERRORTYPE
*/
/* ================================================================================ */
OMX_U32 NBAMRENC_IsValid(AMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                         OMX_U8 *pBuffer, OMX_DIRTYPE eDir)
{
    OMX_U16 i;
    int found=0;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBuffer == pComponentPrivate->pInputBufferList->pBufHdr[i]->pBuffer) {
                found = 1;
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBuffer == pComponentPrivate->pOutputBufferList->pBufHdr[i]->pBuffer) {
                found = 1;
            }
        }
    }
    return found;
}
/* ========================================================================== */
/**
* @NBAMRENC_FillLCMLInitParamsEx() This function is used by the component thread to
* fill the all of its initialization parameters, buffer deatils  etc
* to LCML structure,
*
* @param pComponent  handle for this instance of the component
* @param plcml_Init  pointer to LCML structure to be filled
*
* @pre
*
* @post
*
* @return none
*/
/* ========================================================================== */
OMX_ERRORTYPE NBAMRENC_FillLCMLInitParamsEx(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf,nIpBufSize,nOpBuf,nOpBufSize;
    OMX_BUFFERHEADERTYPE *pTemp;
    char *ptr;
    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    AMRENC_COMPONENT_PRIVATE *pComponentPrivate = pHandle->pComponentPrivate;
    NBAMRENC_LCML_BUFHEADERTYPE *pTemp_lcml = NULL;
    OMX_U16 i;
    OMX_U32 size_lcml;
    OMX_U8  *pBufferParamTemp;
    AMRENC_DPRINT("%d :: NBAMRENC_FillLCMLInitParamsEx\n",__LINE__);
    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nIpBufSize = pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->nBufferSize;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nOpBufSize = pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->nBufferSize;
    AMRENC_DPRINT("%d :: ------ Buffer Details -----------\n",__LINE__);
    AMRENC_DPRINT("%d :: Input  Buffer Count = %ld\n",__LINE__,nIpBuf);
    AMRENC_DPRINT("%d :: Input  Buffer Size = %ld\n",__LINE__,nIpBufSize);
    AMRENC_DPRINT("%d :: Output Buffer Count = %ld\n",__LINE__,nOpBuf);
    AMRENC_DPRINT("%d :: Output Buffer Size = %ld\n",__LINE__,nOpBufSize);
    AMRENC_DPRINT("%d :: ------ Buffer Details ------------\n",__LINE__);
    /* Allocate memory for all input buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nIpBuf * sizeof(NBAMRENC_LCML_BUFHEADERTYPE);
    NBAMRENC_OMX_MALLOC_SIZE(pTemp_lcml, size_lcml,NBAMRENC_LCML_BUFHEADERTYPE);
    
    pComponentPrivate->pLcmlBufHeader[NBAMRENC_INPUT_PORT] = pTemp_lcml;
    for (i=0; i<nIpBuf; i++) {
        AMRENC_DPRINT("%d :: INPUT--------- Inside Ip Loop\n",__LINE__);
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = NBAMRENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = NBAMRENC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = NBAMRENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        AMRENC_DPRINT("%d :: pTemp_lcml->buffer->pBuffer = %p \n",__LINE__,pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirInput;

        NBAMRENC_OMX_MALLOC_SIZE(pBufferParamTemp, sizeof(NBAMRENC_ParamStruct) + DSP_CACHE_ALIGNMENT,OMX_U8);

        pTemp_lcml->pBufferParam =  (NBAMRENC_ParamStruct*)(pBufferParamTemp + EXTRA_BYTES);
        
        pTemp_lcml->pBufferParam->usNbFrames=0;
        pTemp_lcml->pBufferParam->pParamElem=NULL;
        pTemp_lcml->pFrameParam=NULL;
        NBAMRENC_OMX_MALLOC(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);

        pTemp->nFlags = NBAMRENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nOpBuf * sizeof(NBAMRENC_LCML_BUFHEADERTYPE);
    NBAMRENC_OMX_MALLOC_SIZE(pTemp_lcml, size_lcml,NBAMRENC_LCML_BUFHEADERTYPE);

    pComponentPrivate->pLcmlBufHeader[NBAMRENC_OUTPUT_PORT] = pTemp_lcml;
    for (i=0; i<nOpBuf; i++) {
        AMRENC_DPRINT("%d :: OUTPUT--------- Inside Op Loop\n",__LINE__);
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
         pTemp->nFilledLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = NBAMRENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = NBAMRENC_MINOR_VER;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = NBAMRENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        AMRENC_DPRINT("%d :: pTemp_lcml->buffer->pBuffer = %p \n",__LINE__,pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirOutput;

        NBAMRENC_OMX_MALLOC_SIZE(pTemp_lcml->pBufferParam,
                                (sizeof(NBAMRENC_ParamStruct)+DSP_CACHE_ALIGNMENT),
                                NBAMRENC_ParamStruct); 
                                
        ptr = (char*)pTemp_lcml->pBufferParam;
        ptr += EXTRA_BYTES;
        pTemp_lcml->pBufferParam = (NBAMRENC_ParamStruct*)ptr;
        
        pTemp_lcml->pBufferParam->usNbFrames=0;
        pTemp_lcml->pBufferParam->pParamElem=NULL;
        pTemp_lcml->pFrameParam=NULL;
        NBAMRENC_OMX_MALLOC(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);


        pTemp->nFlags = NBAMRENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }

    pComponentPrivate->bInitParamsInitialized = 1;
EXIT:
    AMRENC_DPRINT("%d :: Exiting NBAMRENC_FillLCMLInitParamsEx\n",__LINE__);
    AMRENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/** ========================================================================
*  OMX_DmmMap () method is used to allocate the memory using DMM.
*
*  @param ProcHandle -  Component identification number
*  @param size  - Buffer header address, that needs to be sent to codec
*  @param pArmPtr - Message used to send the buffer to codec
*  @param pDmmBuf - buffer id
*
*  @retval OMX_ErrorNone  - Success
*          OMX_ErrorHardware  -  Hardware Error
** ==========================================================================*/
OMX_ERRORTYPE OMX_DmmMap(DSP_HPROCESSOR ProcHandle,
                     int size,
                     void* pArmPtr,
                     DMM_BUFFER_OBJ* pDmmBuf)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    DSP_STATUS status;
    int nSizeReserved = 0;

    if(pDmmBuf == NULL)
    {
        AMRENC_EPRINT("pBuf is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if(pArmPtr == NULL)
    {
        AMRENC_EPRINT("pBuf is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if(ProcHandle == NULL)
    {
        AMRENC_EPRINT("ProcHandle is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    /* Allocate */
    pDmmBuf->pAllocated = pArmPtr;

    /* Reserve */
    nSizeReserved = ROUND_TO_PAGESIZE(size) + 2*DMM_PAGE_SIZE ;


    status = DSPProcessor_ReserveMemory(ProcHandle, nSizeReserved, &(pDmmBuf->pReserved));
 
    if(DSP_FAILED(status))
    {
        AMRENC_EPRINT("DSPProcessor_ReserveMemory() failed - error 0x%x", (int)status);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    pDmmBuf->nSize = size;
    AMRENC_DPRINT(" DMM MAP Reserved: %p, size 0x%x (%d)\n", pDmmBuf->pReserved,nSizeReserved,nSizeReserved);

    /* Map */
    status = DSPProcessor_Map(ProcHandle,
                              pDmmBuf->pAllocated,/* Arm addres of data to Map on DSP*/
                              size , /* size to Map on DSP*/
                              pDmmBuf->pReserved, /* reserved space */
                              &(pDmmBuf->pMapped), /* returned map pointer */
                              0); /* final param is reserved.  set to zero. */
    if(DSP_FAILED(status))
    {
        AMRENC_EPRINT("DSPProcessor_Map() failed - error 0x%x", (int)status);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    AMRENC_DPRINT("DMM Mapped: %p, size 0x%x (%d)\n",pDmmBuf->pMapped, size,size);

    /* Issue an initial memory flush to ensure cache coherency */
    status = DSPProcessor_FlushMemory(ProcHandle, pDmmBuf->pAllocated, size, 0);
    if(DSP_FAILED(status))
    {
        AMRENC_EPRINT("Unable to flush mapped buffer: error 0x%x",(int)status);
        goto EXIT;
    }
    eError = OMX_ErrorNone;

EXIT:
   return eError;
}

/** ========================================================================
*  OMX_DmmUnMap () method is used to de-allocate the memory using DMM.
*
*  @param ProcHandle -  Component identification number
*  @param pMapPtr  - Map address
*  @param pResPtr - reserve adress
*
*  @retval OMX_ErrorNone  - Success
*          OMX_ErrorHardware  -  Hardware Error
** ==========================================================================*/
OMX_ERRORTYPE OMX_DmmUnMap(DSP_HPROCESSOR ProcHandle, void* pMapPtr, void* pResPtr)
{
    DSP_STATUS status = DSP_SOK;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
/*    printf("OMX UnReserve DSP: %p\n",pResPtr);*/
    if(pMapPtr == NULL)
    {
        AMRENC_EPRINT("pMapPtr is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if(pResPtr == NULL)
    {
        AMRENC_EPRINT("pResPtr is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if(ProcHandle == NULL)
    {
        AMRENC_EPRINT("--ProcHandle is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    status = DSPProcessor_UnMap(ProcHandle,pMapPtr);
    if(DSP_FAILED(status))
    {
        AMRENC_EPRINT("DSPProcessor_UnMap() failed - error 0x%x",(int)status);
    }

    AMRENC_DPRINT("unreserving  structure =0x%p\n",pResPtr );
    status = DSPProcessor_UnReserveMemory(ProcHandle,pResPtr);
    if(DSP_FAILED(status))
    {
        AMRENC_EPRINT("DSPProcessor_UnReserveMemory() failed - error 0x%x", (int)status);
    }

EXIT:
    return eError;
}



/* void NBAMRENC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData)
{
    OMX_COMMANDTYPE Cmd = OMX_CommandStateSet;
    OMX_STATETYPE state = OMX_StateIdle;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)cbData.hComponent;
    AMRENC_COMPONENT_PRIVATE *pCompPrivate = NULL;

    pCompPrivate = (AMRENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    if (*(cbData.RM_Error) == OMX_RmProxyCallback_ResourcesPreempted) {
        if (pCompPrivate->curState == OMX_StateExecuting || 
            pCompPrivate->curState == OMX_StatePause) {
            write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
            write (pCompPrivate->cmdDataPipe[1], &state ,sizeof(OMX_U32));

            pCompPrivate->bPreempted = 1;
        }
    }
    else if (*(cbData.RM_Error) == OMX_RmProxyCallback_ResourcesAcquired){
        pCompPrivate->cbInfo.EventHandler (
                            pHandle, pHandle->pApplicationPrivate,
                            OMX_EventResourcesAcquired, 0,0,
                            NULL);
            
        
    }

} */
