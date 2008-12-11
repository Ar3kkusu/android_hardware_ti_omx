
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
* @file OMX_AmrDecoder.c
*
* This file implements OMX Component for AMR decoder that
* is fully compliant with the OMX Audio specification 1.5.
*
* @path  $(CSLPATH)\
*
* @rev  0.1
*/
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! ===================================
*! 12-Sept-2005 mf:  Initial Version. Change required per OMAPSWxxxxxxxxx
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

#ifdef DSP_RENDERING_ON
#include <AudioManagerAPI.h>
#endif

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

/*-------program files ----------------------------------------*/
#include <OMX_Component.h>
#include <TIDspOmx.h>

#include "OMX_WbAmrDecoder.h"
#include "OMX_WbAmrDec_Utils.h"

#ifdef WBAMRDEC_DEBUGMEM

void *arr[500];
int lines[500];
int bytes[500];
char file[500][50];

void * mymalloc(int line, char *s, int size);
int myfree(void *dp, int line, char *s);
#define newmalloc(x) mymalloc(__LINE__,__FILE__,x)
#define newfree(z) myfree(z,__LINE__,__FILE__)

#else
#define newmalloc(x) malloc(x)
#define newfree(z) free(z)
#endif
#define AMRWB_DEC_ROLE "audio_decoder.amrwb"


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

static OMX_ERRORTYPE SetCallbacks (OMX_HANDLETYPE hComp,
        OMX_CALLBACKTYPE* pCallBacks, OMX_PTR pAppData);
static OMX_ERRORTYPE GetComponentVersion (OMX_HANDLETYPE hComp,
										  OMX_STRING pComponentName,
										  OMX_VERSIONTYPE* pComponentVersion,
										  OMX_VERSIONTYPE* pSpecVersion,
										  OMX_UUIDTYPE* pComponentUUID);

static OMX_ERRORTYPE SendCommand (OMX_HANDLETYPE hComp, OMX_COMMANDTYPE nCommand,
								  OMX_U32 nParam, OMX_PTR pCmdData);

static OMX_ERRORTYPE GetParameter(OMX_HANDLETYPE hComp, OMX_INDEXTYPE nParamIndex,
								  OMX_PTR ComponentParamStruct);
static OMX_ERRORTYPE SetParameter (OMX_HANDLETYPE hComp,
								   OMX_INDEXTYPE nParamIndex,
								   OMX_PTR ComponentParamStruct);
static OMX_ERRORTYPE GetConfig (OMX_HANDLETYPE hComp,
								OMX_INDEXTYPE nConfigIndex,
								OMX_PTR pComponentConfigStructure);
static OMX_ERRORTYPE SetConfig (OMX_HANDLETYPE hComp,
								OMX_INDEXTYPE nConfigIndex,
								OMX_PTR pComponentConfigStructure);

static OMX_ERRORTYPE EmptyThisBuffer (OMX_HANDLETYPE hComp, OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE FillThisBuffer (OMX_HANDLETYPE hComp, OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE GetState (OMX_HANDLETYPE hComp, OMX_STATETYPE* pState);
static OMX_ERRORTYPE ComponentTunnelRequest (OMX_HANDLETYPE hComp,
											 OMX_U32 nPort, OMX_HANDLETYPE hTunneledComp,
											 OMX_U32 nTunneledPort,
											 OMX_TUNNELSETUPTYPE* pTunnelSetup);

static OMX_ERRORTYPE ComponentDeInit(OMX_HANDLETYPE pHandle);
static OMX_ERRORTYPE AllocateBuffer (OMX_IN OMX_HANDLETYPE hComponent,
			       OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
			       OMX_IN OMX_U32 nPortIndex,
			       OMX_IN OMX_PTR pAppPrivate,
			       OMX_IN OMX_U32 nSizeBytes);

static OMX_ERRORTYPE FreeBuffer(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_U32 nPortIndex,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE UseBuffer (
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes,
            OMX_IN OMX_U8* pBuffer);

static OMX_ERRORTYPE GetExtensionIndex(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_STRING cParameterName,
            OMX_OUT OMX_INDEXTYPE* pIndexType);

static OMX_ERRORTYPE ComponentRoleEnum(
        OMX_IN OMX_HANDLETYPE hComponent,
		OMX_OUT OMX_U8 *cRole,
		OMX_IN OMX_U32 nIndex);   
#ifdef DSP_RENDERING_ON         
#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"
#define PERMS 0666


   AM_COMMANDDATATYPE cmd_data;
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
  *         OMX_ErrorInsufficientResources If the newmalloc fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp)
{
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef_ip, *pPortDef_op;
    WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate;
    OMX_AUDIO_PARAM_AMRTYPE *amr_ip;
    OMX_AUDIO_PARAM_PCMMODETYPE *amr_op;
    OMX_ERRORTYPE error = OMX_ErrorNone;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
	OMX_S16 i;

    WBAMR_DEC_DPRINT ("%d ::OMX_ComponentInit\n", __LINE__);
    WBAMR_DEC_DPRINT ("%d ::LINE %d\n", __LINE__);

    /*Set the all component function pointer to the handle */
    pHandle->SetCallbacks = SetCallbacks;
    pHandle->GetComponentVersion = GetComponentVersion;
    pHandle->SendCommand = SendCommand;
    pHandle->GetParameter = GetParameter;
    pHandle->SetParameter = SetParameter;
    pHandle->GetConfig = GetConfig;
    pHandle->SetConfig = SetConfig;
    pHandle->GetState = GetState;
    pHandle->EmptyThisBuffer = EmptyThisBuffer;
    pHandle->FillThisBuffer = FillThisBuffer;
    pHandle->ComponentTunnelRequest = ComponentTunnelRequest;
    pHandle->ComponentDeInit = ComponentDeInit;
    pHandle->AllocateBuffer = AllocateBuffer;
    pHandle->FreeBuffer = FreeBuffer;
    pHandle->UseBuffer = UseBuffer;
    pHandle->GetExtensionIndex= GetExtensionIndex;
    pHandle->ComponentRoleEnum = ComponentRoleEnum;

    WBAMR_DEC_DPRINT ("%d ::LINE %d\n", __LINE__);
    /*Allocate the memory for Component private data area */
    // pHandle->pComponentPrivate = (WBAMR_DEC_COMPONENT_PRIVATE *)
                                     // newmalloc (sizeof(WBAMR_DEC_COMPONENT_PRIVATE));
	WBAMR_DEC_OMX_MALLOC(pHandle->pComponentPrivate, WBAMR_DEC_COMPONENT_PRIVATE); 

    WBAMR_DEC_MEMPRINT("%d:[ALLOC] %p\n",__LINE__,pHandle->pComponentPrivate);
    WBAMR_DEC_DPRINT ("%d ::LINE %d\n", __LINE__);

    if(pHandle->pComponentPrivate == NULL) {
        error = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
	memset(pHandle->pComponentPrivate, 0x0, sizeof(WBAMR_DEC_COMPONENT_PRIVATE));
    WBAMR_DEC_DPRINT ("%d ::LINE %d\n", __LINE__);

    WBAMR_DEC_DPRINT ("%d ::LINE %d\n", __LINE__);
    ((WBAMR_DEC_COMPONENT_PRIVATE *)
                pHandle->pComponentPrivate)->pHandle = pHandle;

    WBAMR_DEC_DPRINT ("%d ::LINE %d\n", __LINE__);
   /* Initialize component data structures to default values */
    ((WBAMR_DEC_COMPONENT_PRIVATE *)
                 pHandle->pComponentPrivate)->sPortParam.nPorts = 0x2;
    ((WBAMR_DEC_COMPONENT_PRIVATE *)
                 pHandle->pComponentPrivate)->sPortParam.nStartPortNumber = 0x0;

    WBAMR_DEC_DPRINT ("%d ::LINE %d\n", __LINE__);
    error = OMX_ErrorNone;

    WBAMR_DEC_DPRINT ("%d ::LINE %d\n", __LINE__);
    //amr_ip = (OMX_AUDIO_PARAM_AMRTYPE *)newmalloc(sizeof(OMX_AUDIO_PARAM_AMRTYPE));
	WBAMR_DEC_OMX_MALLOC(amr_ip , OMX_AUDIO_PARAM_AMRTYPE); 
    // WBAMR_DEC_MEMPRINT("%d:[ALLOC] %p\n",__LINE__,amr_ip);
    // WBAMR_DEC_DPRINT ("%d ::LINE %d\n", __LINE__);
    // if(NULL == amr_ip) {
        // WBAMR_DEC_EPRINT("%d :: newmalloc failed\n",__LINE__);
        // /* Free previously allocated memory before bailing */
        // if (pHandle->pComponentPrivate) {
            // WBAMR_DEC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pHandle->pComponentPrivate);
            // OMX_WBDECMEMFREE_STRUCT(pHandle->pComponentPrivate);
            // pHandle->pComponentPrivate = NULL;
        // }
        // error = OMX_ErrorInsufficientResources;
        // goto EXIT;
    // }
	// memset(amr_ip, 0x0, sizeof(OMX_AUDIO_PARAM_AMRTYPE));
    WBAMR_DEC_DPRINT ("%d ::LINE %d\n", __LINE__);
    //amr_op = (OMX_AUDIO_PARAM_PCMMODETYPE *)newmalloc(sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	WBAMR_DEC_OMX_MALLOC(amr_op , OMX_AUDIO_PARAM_PCMMODETYPE);
    // WBAMR_DEC_MEMPRINT("%d:[ALLOC] %p\n",__LINE__,amr_op);
    // WBAMR_DEC_DPRINT ("%d ::LINE %d\n", __LINE__);
    // if(NULL == amr_op) {
        // WBAMR_DEC_EPRINT("%d :: newmalloc failed\n",__LINE__);
        // /* Free previously allocated memory before bailing */
        // if (pHandle->pComponentPrivate) {
            // WBAMR_DEC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pHandle->pComponentPrivate);
            // OMX_WBDECMEMFREE_STRUCT(pHandle->pComponentPrivate);
            // pHandle->pComponentPrivate = NULL;
        // }

		// if (amr_ip) {
			// OMX_WBDECMEMFREE_STRUCT(amr_ip);
			// amr_ip = NULL;
		// }
        // error = OMX_ErrorInsufficientResources;
        // goto EXIT;
    // }
	// memset(amr_op, 0x0, sizeof(OMX_AUDIO_PARAM_AMRTYPE));
    WBAMR_DEC_DPRINT ("%d ::LINE %d\n", __LINE__);

    ((WBAMR_DEC_COMPONENT_PRIVATE *)
                 pHandle->pComponentPrivate)->wbamrParams[WBAMR_DEC_INPUT_PORT] = amr_ip;
    ((WBAMR_DEC_COMPONENT_PRIVATE *)
                 pHandle->pComponentPrivate)->wbamrParams[WBAMR_DEC_OUTPUT_PORT] = (OMX_AUDIO_PARAM_AMRTYPE*)amr_op;

        WBAMR_DEC_DPRINT("%d Malloced wbamrParams[WBAMR_DEC_INPUT_PORT] = 0x%x\n",__LINE__,((WBAMR_DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->wbamrParams[WBAMR_DEC_INPUT_PORT]);
        WBAMR_DEC_DPRINT("%d Malloced wbamrParams[WBAMR_DEC_OUTPUT_PORT] = 0x%x\n",__LINE__,((WBAMR_DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->wbamrParams[WBAMR_DEC_OUTPUT_PORT]);


	pComponentPrivate = pHandle->pComponentPrivate;
#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERF = PERF_Create(PERF_FOURCC('W','B','D','_'),
                                           PERF_ModuleLLMM |
                                           PERF_ModuleAudioDecode);
#endif

#if 1 /* currently using default values until more is understood */
			pComponentPrivate->iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_TRUE; /* this should be true always for TI components */
			pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_FALSE;
			pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_FALSE;
			pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_TRUE; /* experiment with this */
			pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsPartialFrames = OMX_FALSE; /* experiment with this */
			pComponentPrivate->iPVCapabilityFlags.iOMXComponentNeedsNALStartCode = OMX_FALSE; /* used only for H.264, leave this as false */
			pComponentPrivate->iPVCapabilityFlags.iOMXComponentCanHandleIncompleteFrames = OMX_TRUE; /* experiment with this */
#endif

	//pComponentPrivate->pInputBufferList = newmalloc(sizeof(WBAMR_DEC_BUFFERLIST));
	WBAMR_DEC_OMX_MALLOC(pComponentPrivate->pInputBufferList, WBAMR_DEC_BUFFERLIST); 
	// if (pComponentPrivate->pInputBufferList == NULL) {
        // WBAMR_DEC_EPRINT("%d :: newmalloc failed\n",__LINE__);
        // /* Free previously allocated memory before bailing */
        // if (pHandle->pComponentPrivate) {
            // WBAMR_DEC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pHandle->pComponentPrivate);
            // OMX_WBDECMEMFREE_STRUCT(pHandle->pComponentPrivate);
            // pHandle->pComponentPrivate = NULL;
        // }

		// if (amr_ip) {
			// OMX_WBDECMEMFREE_STRUCT(amr_ip);
			// amr_ip = NULL;
		// }

		// if (amr_op) {
			// OMX_WBDECMEMFREE_STRUCT(amr_op);
			// amr_op = NULL;
		// }
        // error = OMX_ErrorInsufficientResources;
        // goto EXIT;
    // }
	// memset(pComponentPrivate->pInputBufferList, 0x0, sizeof(WBAMR_DEC_BUFFERLIST));
	pComponentPrivate->pInputBufferList->numBuffers = 0; /* initialize number of buffers */
	//pComponentPrivate->pOutputBufferList = newmalloc(sizeof(WBAMR_DEC_BUFFERLIST));
	WBAMR_DEC_OMX_MALLOC(pComponentPrivate->pOutputBufferList, WBAMR_DEC_BUFFERLIST); 
	// if (pComponentPrivate->pOutputBufferList == NULL) {
        // WBAMR_DEC_EPRINT("%d :: newmalloc failed\n",__LINE__);
        // /* Free previously allocated memory before bailing */
        // if (pHandle->pComponentPrivate) {
            // WBAMR_DEC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pHandle->pComponentPrivate);
            // OMX_WBDECMEMFREE_STRUCT(pHandle->pComponentPrivate);
            // pHandle->pComponentPrivate = NULL;
        // }

		// if (amr_ip) {
			// OMX_WBDECMEMFREE_STRUCT(amr_ip);
			// amr_ip = NULL;
		// }

		// if (amr_op) {
			// OMX_WBDECMEMFREE_STRUCT(amr_op);
			// amr_op = NULL;
		// }

		// if (pComponentPrivate->pInputBufferList) {
			// OMX_WBDECMEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
			// pComponentPrivate->pInputBufferList = NULL;
		// }

        // error = OMX_ErrorInsufficientResources;
        // goto EXIT;
    // }
	// memset(pComponentPrivate->pOutputBufferList, 0x0, sizeof(WBAMR_DEC_BUFFERLIST));
    //pComponentPrivate->pPriorityMgmt = newmalloc(sizeof(OMX_PRIORITYMGMTTYPE));
	WBAMR_DEC_OMX_MALLOC(pComponentPrivate->pPriorityMgmt, OMX_PRIORITYMGMTTYPE); 
	pComponentPrivate->pOutputBufferList->numBuffers = 0; /* initialize number of buffers */
	for (i=0; i < WBAMR_DEC_MAX_NUM_OF_BUFS; i++) {
		pComponentPrivate->pOutputBufferList->pBufHdr[i] = NULL;
		pComponentPrivate->pInputBufferList->pBufHdr[i] = NULL;
		pComponentPrivate->arrTickCount[i] = 0;
  	  	pComponentPrivate->arrBufIndex[i] = 0;
	}

	pComponentPrivate->IpBufindex = 0;
    pComponentPrivate->OpBufindex = 0;
	
    WBAMR_DEC_DPRINT("Setting dasfmode and mimemode to 0\n");
    pComponentPrivate->dasfmode = 0;
    pComponentPrivate->mimemode = 0;
    pComponentPrivate->bPortDefsAllocated = 0;
    pComponentPrivate->bCompThreadStarted = 0;
    pComponentPrivate->nHoldLength = 0;
	pComponentPrivate->pHoldBuffer = NULL;
	pComponentPrivate->bInitParamsInitialized = 0;
    pComponentPrivate->wbamrMimeBytes[0] =  WBAMR_DEC_FRAME_SIZE_18;
    pComponentPrivate->wbamrMimeBytes[1] =  WBAMR_DEC_FRAME_SIZE_24;
    pComponentPrivate->wbamrMimeBytes[2] =  WBAMR_DEC_FRAME_SIZE_33;
    pComponentPrivate->wbamrMimeBytes[3] =  WBAMR_DEC_FRAME_SIZE_37;
    pComponentPrivate->wbamrMimeBytes[4] =  WBAMR_DEC_FRAME_SIZE_41;
    pComponentPrivate->wbamrMimeBytes[5] =  WBAMR_DEC_FRAME_SIZE_47;
    pComponentPrivate->wbamrMimeBytes[6] =  WBAMR_DEC_FRAME_SIZE_51;
    pComponentPrivate->wbamrMimeBytes[7] =  WBAMR_DEC_FRAME_SIZE_59;
    pComponentPrivate->wbamrMimeBytes[8] =  WBAMR_DEC_FRAME_SIZE_61;
    pComponentPrivate->wbamrMimeBytes[9] =  WBAMR_DEC_FRAME_SIZE_6;
    pComponentPrivate->wbamrMimeBytes[10] = WBAMR_DEC_FRAME_SIZE_0;
    pComponentPrivate->wbamrMimeBytes[11] = WBAMR_DEC_FRAME_SIZE_0;
    pComponentPrivate->wbamrMimeBytes[12] = WBAMR_DEC_FRAME_SIZE_0;
    pComponentPrivate->wbamrMimeBytes[13] = WBAMR_DEC_FRAME_SIZE_0;
    pComponentPrivate->wbamrMimeBytes[14] = WBAMR_DEC_FRAME_SIZE_1;
    pComponentPrivate->wbamrMimeBytes[15] = WBAMR_DEC_FRAME_SIZE_1;
    
    pComponentPrivate->wbamrIf2Bytes[0] = WBAMR_DEC_FRAME_SIZE_18;
	pComponentPrivate->wbamrIf2Bytes[1] = WBAMR_DEC_FRAME_SIZE_23;
	pComponentPrivate->wbamrIf2Bytes[2] = WBAMR_DEC_FRAME_SIZE_33;
	pComponentPrivate->wbamrIf2Bytes[3] = WBAMR_DEC_FRAME_SIZE_37;
	pComponentPrivate->wbamrIf2Bytes[4] = WBAMR_DEC_FRAME_SIZE_41;
	pComponentPrivate->wbamrIf2Bytes[5] = WBAMR_DEC_FRAME_SIZE_47;
	pComponentPrivate->wbamrIf2Bytes[6] = WBAMR_DEC_FRAME_SIZE_51;
	pComponentPrivate->wbamrIf2Bytes[7] = WBAMR_DEC_FRAME_SIZE_59;
	pComponentPrivate->wbamrIf2Bytes[8] = WBAMR_DEC_FRAME_SIZE_61;
	pComponentPrivate->wbamrIf2Bytes[9] = WBAMR_DEC_FRAME_SIZE_6;
	pComponentPrivate->wbamrIf2Bytes[10] = WBAMR_DEC_FRAME_SIZE_0;
	pComponentPrivate->wbamrIf2Bytes[11] = WBAMR_DEC_FRAME_SIZE_0;
	pComponentPrivate->wbamrIf2Bytes[12] = WBAMR_DEC_FRAME_SIZE_0;
	pComponentPrivate->wbamrIf2Bytes[13] = WBAMR_DEC_FRAME_SIZE_0;
	pComponentPrivate->wbamrIf2Bytes[14] = WBAMR_DEC_FRAME_SIZE_1;
	pComponentPrivate->wbamrIf2Bytes[15] = WBAMR_DEC_FRAME_SIZE_1;
    
	pComponentPrivate->pMarkBuf = NULL;
	pComponentPrivate->pMarkData = NULL;
	pComponentPrivate->nEmptyBufferDoneCount = 0;
	pComponentPrivate->nEmptyThisBufferCount = 0;
	pComponentPrivate->nFillBufferDoneCount = 0;
	pComponentPrivate->nFillThisBufferCount = 0;
	pComponentPrivate->strmAttr = NULL;
/*	pComponentPrivate->bIdleCommandPending = 0; */
	pComponentPrivate->bDisableCommandParam = 0;
    pComponentPrivate->SendAfterEOS = 0;
    pComponentPrivate->PendingPausedBufs = 0;
	for (i=0; i < WBAMR_DEC_MAX_NUM_OF_BUFS; i++) {
		pComponentPrivate->pInputBufHdrPending[i] = NULL;
		pComponentPrivate->pOutputBufHdrPending[i] = NULL;
	}
	pComponentPrivate->nNumInputBufPending = 0;
	pComponentPrivate->nNumOutputBufPending = 0;
	pComponentPrivate->bDisableCommandPending = 0;
	pComponentPrivate->nOutStandingFillDones = 0;
	pComponentPrivate->bStopSent=0;
	pComponentPrivate->bNoIdleOnStop = OMX_FALSE;
	pComponentPrivate->pParams = NULL; /* Without this initialization repetition cases failed. */
   	pComponentPrivate->LastOutbuf=NULL;   	
    strcpy((char*)pComponentPrivate->componentRole.cRole, "audio_decoder.amrwb");    
    
    

/* Set input port format defaults */
    pComponentPrivate->sInPortFormat.nPortIndex         = WBAMR_DEC_INPUT_PORT;
    pComponentPrivate->sInPortFormat.nIndex             = OMX_IndexParamAudioAmr;    
    pComponentPrivate->sInPortFormat.eEncoding 			= OMX_AUDIO_CodingAMR;


/* Set output port format defaults */
    pComponentPrivate->sOutPortFormat.nPortIndex         = WBAMR_DEC_OUTPUT_PORT;
    pComponentPrivate->sOutPortFormat.nIndex             = OMX_IndexParamAudioPcm;
    pComponentPrivate->sOutPortFormat.eEncoding 		 = OMX_AUDIO_CodingPCM;

	pComponentPrivate->bPreempted = OMX_FALSE; 

    amr_ip->nPortIndex = OMX_DirInput;
    amr_ip->nChannels = 1;
    amr_ip->nBitRate = 8000;
    amr_ip->eAMRBandMode = OMX_AUDIO_AMRBandModeWB0;
    amr_ip->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOff;
    amr_ip->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatConformance;
    amr_ip->nSize = sizeof (OMX_AUDIO_PARAM_AMRTYPE);

    /* PCM format defaults - These values are required to pass StdAudioDecoderTest*/
    amr_op->nPortIndex = OMX_DirOutput;
    amr_op->nChannels = 2; 
    amr_op->eNumData= OMX_NumericalDataSigned;
    amr_op->nSamplingRate = 44100;           
    amr_op->nBitPerSample = 16;
    amr_op->ePCMMode = OMX_AUDIO_PCMModeLinear;

    strcpy((char*)pComponentPrivate->componentRole.cRole, "audio_decoder.amrwb");    
    //pComponentPrivate->sDeviceString = newmalloc(100*sizeof(OMX_STRING));
	WBAMR_DEC_OMX_MALLOC_SIZE(pComponentPrivate->sDeviceString, (100*sizeof(OMX_STRING)),OMX_STRING);
    pComponentPrivate->IpBufindex = 0;
	pComponentPrivate->OpBufindex = 0;
	pComponentPrivate->ptrLibLCML = NULL;
    
    strcpy((char*)pComponentPrivate->sDeviceString,"/eteedn:i0:o0/codec\0");
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
    // pPortDef_ip = (OMX_PARAM_PORTDEFINITIONTYPE *)
                 // newmalloc(sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	WBAMR_DEC_OMX_MALLOC(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);			 
	
    WBAMR_DEC_MEMPRINT("%d:[ALLOC] %p\n",__LINE__,pPortDef_ip);
    // if (pPortDef_ip == NULL) {
        // WBAMR_DEC_EPRINT("%d :: newmalloc failed\n",__LINE__);
        // if (pHandle->pComponentPrivate) {
            // WBAMR_DEC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pHandle->pComponentPrivate);
            // OMX_WBDECMEMFREE_STRUCT(pHandle->pComponentPrivate);
            // pHandle->pComponentPrivate = NULL;
        // }

        // if (amr_ip) {
            // WBAMR_DEC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,amr_ip);
            // OMX_WBDECMEMFREE_STRUCT(amr_ip);
            // amr_ip = NULL;
        // }

        // if (amr_op) {
            // WBAMR_DEC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,amr_op);
            // OMX_WBDECMEMFREE_STRUCT(amr_op);
            // amr_op = NULL;
        // }

        // if (pComponentPrivate->pInputBufferList) {
            // WBAMR_DEC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pComponentPrivate->pInputBufferList);
            // OMX_WBDECMEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
            // pComponentPrivate->pInputBufferList = NULL;
        // }

        // if (pComponentPrivate->pOutputBufferList) {
            // WBAMR_DEC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pComponentPrivate->pOutputBufferList);
            // OMX_WBDECMEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);
            // pComponentPrivate->pOutputBufferList = NULL;
        // }
        // error = OMX_ErrorInsufficientResources;
        // goto EXIT;
    // }
	// memset(pPortDef_ip, 0x0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	
    // pPortDef_op = (OMX_PARAM_PORTDEFINITIONTYPE *)
                 // newmalloc(sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	WBAMR_DEC_OMX_MALLOC(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);
				 
    WBAMR_DEC_MEMPRINT("%d:[ALLOC] %p\n",__LINE__,pPortDef_op);
    // if (pPortDef_op == NULL) {
        // WBAMR_DEC_EPRINT("%d :: newmalloc failed\n",__LINE__);
        // /* Free previously allocated memory before bailing */
        // if (pHandle->pComponentPrivate) {
            // WBAMR_DEC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pHandle->pComponentPrivate);
            // OMX_WBDECMEMFREE_STRUCT(pHandle->pComponentPrivate);
            // pHandle->pComponentPrivate = NULL;
        // }

		// if (amr_ip) {
			// OMX_WBDECMEMFREE_STRUCT(amr_ip);
			// amr_ip = NULL;
		// }

		// if (amr_op) {
			// OMX_WBDECMEMFREE_STRUCT(amr_op);
			// amr_op = NULL;
		// }

		// if (pComponentPrivate->pInputBufferList) {
			// OMX_WBDECMEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
			// pComponentPrivate->pInputBufferList = NULL;
		// }

		// if (pComponentPrivate->pOutputBufferList) {
			// OMX_WBDECMEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);
			// pComponentPrivate->pOutputBufferList = NULL;
		// }

        // error = OMX_ErrorInsufficientResources;
        // goto EXIT;
    // }

	// memset(pPortDef_op, 0x0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    WBAMR_DEC_DPRINT ("%d ::pPortDef_ip = 0x%x\n", __LINE__,pPortDef_ip);
    WBAMR_DEC_DPRINT ("%d ::pPortDef_op = 0x%x\n", __LINE__,pPortDef_op);

    ((WBAMR_DEC_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pPortDef[WBAMR_DEC_INPUT_PORT]
                                                              = pPortDef_ip;

    ((WBAMR_DEC_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pPortDef[WBAMR_DEC_OUTPUT_PORT]
                                                            = pPortDef_op;
    pPortDef_ip->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_ip->nPortIndex = 0x0;
    pPortDef_ip->nBufferCountActual = NUM_WBAMRDEC_INPUT_BUFFERS;
    pPortDef_ip->nBufferCountMin = NUM_WBAMRDEC_INPUT_BUFFERS;
    pPortDef_ip->eDir = OMX_DirInput;
    pPortDef_ip->bEnabled = OMX_TRUE;
    pPortDef_ip->nBufferSize = INPUT_WBAMRDEC_BUFFER_SIZE;
    pPortDef_ip->bPopulated = 0;
    pPortDef_ip->format.audio.eEncoding = OMX_AUDIO_CodingAMR;
    pPortDef_op->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_op->nPortIndex = 0x1;
    pPortDef_op->nBufferCountActual = NUM_WBAMRDEC_OUTPUT_BUFFERS;
    pPortDef_op->nBufferCountMin = NUM_WBAMRDEC_OUTPUT_BUFFERS;
    pPortDef_op->eDir = OMX_DirOutput;
    pPortDef_op->bEnabled = OMX_TRUE;
    pPortDef_op->nBufferSize = OUTPUT_WBAMRDEC_BUFFER_SIZE;
    pPortDef_op->bPopulated = 0;
    pPortDef_op->format.audio.eEncoding = OMX_AUDIO_CodingPCM;
    pComponentPrivate->bIsInvalidState = OMX_FALSE;

#ifdef RESOURCE_MANAGER_ENABLED
	error = RMProxy_NewInitalize();
    WBAMR_DEC_DPRINT ("%d ::OMX_ComponentInit\n", __LINE__);
    if (error != OMX_ErrorNone) {
        WBAMR_DEC_EPRINT ("%d ::Error returned from loading ResourceManagerProxy thread\n",
                                                        __LINE__);
        goto EXIT;
    }
#endif

    error = WBAMR_DEC_StartComponentThread(pHandle);
    WBAMR_DEC_DPRINT ("%d ::OMX_ComponentInit\n", __LINE__);
    if (error != OMX_ErrorNone) {
        WBAMR_DEC_EPRINT ("%d ::Error returned from the Component\n",
                                                     __LINE__);
        goto EXIT;
    }
    WBAMR_DEC_DPRINT ("%d ::OMX_ComponentInit\n", __LINE__);


#ifdef DSP_RENDERING_ON
    WBAMR_DEC_DPRINT ("%d ::OMX_ComponentInit\n", __LINE__);
    if((pComponentPrivate->fdwrite=open(FIFO1,O_WRONLY))<0) {
        WBAMR_DEC_EPRINT("[WBAMR Dec Component] - failure to open WRITE pipe\n");
    }

    WBAMR_DEC_DPRINT ("%d ::OMX_ComponentInit\n", __LINE__);
    if((pComponentPrivate->fdread=open(FIFO2,O_RDONLY))<0) {
        WBAMR_DEC_EPRINT("[WBAMR Dec Component] - failure to open READ pipe\n");
		goto EXIT;
    }

#endif
#ifdef __PERF_INSTRUMENTATION__
    PERF_ThreadCreated(pComponentPrivate->pPERF, pComponentPrivate->WBAMR_DEC_ComponentThread,
                       PERF_FOURCC('W','B','D','T'));
#endif
EXIT:
    WBAMR_DEC_DPRINT ("%d ::OMX_ComponentInit - returning %d\n", __LINE__,error);
    return error;
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

    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*)pComponent;

    WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate =
                    (WBAMR_DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    if (pCallBacks == NULL) {
		WBAMR_DEC_EPRINT("About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
        eError = OMX_ErrorBadParameter;
        WBAMR_DEC_DPRINT ("%d :: Received the empty callbacks from the \
                application\n",__LINE__);
        goto EXIT;
    }

    /*Copy the callbacks of the application to the component private */
    memcpy (&(pComponentPrivate->cbInfo), pCallBacks, sizeof(OMX_CALLBACKTYPE));

    /*copy the application private data to component memory */
    pHandle->pApplicationPrivate = pAppData;

    pComponentPrivate->curState = OMX_StateLoaded;
#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif

EXIT:
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

    eError = OMX_ErrorNotImplemented;
    WBAMR_DEC_DPRINT ("Inside the GetComponentVersion\n");
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
    int nRet;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)phandle;
    WBAMR_DEC_COMPONENT_PRIVATE *pCompPrivate =
             (WBAMR_DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

	WBAMR_DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);

	WBAMR_DEC_DPRINT("phandle = %p\n",phandle);
	WBAMR_DEC_DPRINT("pCompPrivate = %p\n",pCompPrivate);

	pCompPrivate->pHandle = phandle;

#ifdef _ERROR_PROPAGATION__
	if (pCompPrivate->curState == OMX_StateInvalid){
		eError = OMX_ErrorInvalidState;
		goto EXIT;
	}
#else
    if(pCompPrivate->curState == OMX_StateInvalid){
        WBAMR_DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
        eError = OMX_ErrorInvalidState;
        WBAMR_DEC_EPRINT("%d :: AMRDEC: Error NotIfication \
                                     Sent to App\n",__LINE__);
        pCompPrivate->cbInfo.EventHandler (pHandle, 
											pHandle->pApplicationPrivate,
									        OMX_EventError, 
											OMX_ErrorInvalidState,
											OMX_TI_ErrorMinor,
									        "Invalid State");
        goto EXIT;
    }
#endif
#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingCommand(pCompPrivate->pPERF, Cmd,
            (Cmd == OMX_CommandMarkBuffer) ? ((OMX_U32) pCmdData) : nParam,
            PERF_ModuleComponent);
#endif

    switch(Cmd) {
        case OMX_CommandStateSet:
    WBAMR_DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
    WBAMR_DEC_DPRINT ("%d:::pCompPrivate->curState = %d\n",__LINE__,pCompPrivate->curState);
			if (nParam == OMX_StateLoaded) {
				pCompPrivate->bLoadedCommandPending = OMX_TRUE;
			} 
            if(pCompPrivate->curState == OMX_StateLoaded) {
                if((nParam == OMX_StateExecuting) || (nParam == OMX_StatePause)) {
                    pCompPrivate->cbInfo.EventHandler (
                                     pHandle,
                                     pHandle->pApplicationPrivate,
                                     OMX_EventError,
                                     OMX_ErrorIncorrectStateTransition,
                                     OMX_TI_ErrorMinor,
                                     NULL);
                    goto EXIT;
                }

                if(nParam == OMX_StateInvalid) {
                    WBAMR_DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
                    pCompPrivate->curState = OMX_StateInvalid;
                    pCompPrivate->cbInfo.EventHandler (
                                     pHandle,
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
			WBAMR_DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
			if(nParam > 1 && nParam != -1) {
				eError = OMX_ErrorBadPortIndex;
				goto EXIT;
			}

            break;
        case OMX_CommandPortDisable:
    WBAMR_DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
            break;
        case OMX_CommandPortEnable:
    WBAMR_DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
            break;
        case OMX_CommandMarkBuffer:
    WBAMR_DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
            if (nParam > 0) {
                eError = OMX_ErrorBadPortIndex;
                goto EXIT;
            }
            break;
    default:
            WBAMR_DEC_DPRINT("%d :: AMRDEC: Command Received Default \
                                                      error\n",__LINE__);
            pCompPrivate->cbInfo.EventHandler (pHandle, 
												pHandle->pApplicationPrivate,
					                            OMX_EventError,
					                            OMX_ErrorUndefined, 
												OMX_TI_ErrorMinor,
					                            "Invalid Command");
            break;

    }

    WBAMR_DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
    nRet = write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
	if (nRet == -1) {
    WBAMR_DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
             eError = OMX_ErrorInsufficientResources;
             goto EXIT;
         }

    if (Cmd == OMX_CommandMarkBuffer) {
            nRet = write(pCompPrivate->cmdDataPipe[1], &pCmdData,
                            sizeof(OMX_PTR));
    }
    else {
            nRet = write(pCompPrivate->cmdDataPipe[1], &nParam,
                            sizeof(OMX_U32));
    }

    WBAMR_DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
    WBAMR_DEC_DPRINT ("%d:::nRet = %d\n",__LINE__,nRet);
         if (nRet == -1) {
    WBAMR_DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
             eError = OMX_ErrorInsufficientResources;
             goto EXIT;
         }
         
#ifdef DSP_RENDERING_ON
 if(Cmd == OMX_CommandStateSet && nParam == OMX_StateExecuting) {
                           /* enable Tee device command*/
                           cmd_data.hComponent = pHandle;
                           cmd_data.AM_Cmd = AM_CommandTDNDownlinkMode;
                           cmd_data.param1 = 0;
                           cmd_data.param2 = 0;
                           cmd_data.streamID = 0;
                           if((write(pCompPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0) {
                                  eError = OMX_ErrorHardware;
                                  goto EXIT;
                           }
              }
#endif

EXIT:
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

static OMX_ERRORTYPE GetParameter (OMX_HANDLETYPE hComp,
                                   OMX_INDEXTYPE nParamIndex,
                                   OMX_PTR ComponentParameterStructure)
{

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    WBAMR_DEC_COMPONENT_PRIVATE  *pComponentPrivate;
	OMX_PARAM_PORTDEFINITIONTYPE *pParameterStructure;

    WBAMR_DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

    WBAMR_DEC_DPRINT("%d :: Inside the GetParameter:: %x\n",__LINE__,nParamIndex);

    pComponentPrivate = (WBAMR_DEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
	pParameterStructure = (OMX_PARAM_PORTDEFINITIONTYPE*)ComponentParameterStructure;

	if (pParameterStructure == NULL) {
		eError = OMX_ErrorBadParameter;
		goto EXIT;

	}
	WBAMR_DEC_DPRINT("pParameterStructure = %p\n",pParameterStructure);


    WBAMR_DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
#ifdef _ERROR_PROPAGATION__
	if (pComponentPrivate->curState == OMX_StateInvalid){
		eError = OMX_ErrorInvalidState;
		goto EXIT;
	}
#else
    if(pComponentPrivate->curState == OMX_StateInvalid) {
        pComponentPrivate->cbInfo.EventHandler(
                            hComp,
                            ((OMX_COMPONENTTYPE *)hComp)->pApplicationPrivate,
                            OMX_EventError,
                            OMX_ErrorIncorrectStateOperation,
                            OMX_TI_ErrorMinor,
                            NULL);
    WBAMR_DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
    }
#endif
    WBAMR_DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
    switch(nParamIndex){

        case OMX_IndexParamAudioInit:
    WBAMR_DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
        WBAMR_DEC_DPRINT ("OMX_IndexParamAudioInit\n");
        memcpy(ComponentParameterStructure, &pComponentPrivate->sPortParam, sizeof(OMX_PORT_PARAM_TYPE));
        break;

        case OMX_IndexParamPortDefinition:
    WBAMR_DEC_DPRINT ("pParameterStructure->nPortIndex = %d\n",pParameterStructure->nPortIndex);
    WBAMR_DEC_DPRINT ("pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->nPortIndex = %d\n",pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->nPortIndex);
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
                                pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->nPortIndex) {
    WBAMR_DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

            memcpy(ComponentParameterStructure,
                       pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT],
                       sizeof(OMX_PARAM_PORTDEFINITIONTYPE)
                      );
    WBAMR_DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

            } else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
                              pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->nPortIndex) {
    WBAMR_DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

            memcpy(ComponentParameterStructure,
                       pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT],
                       sizeof(OMX_PARAM_PORTDEFINITIONTYPE)
                      );
    WBAMR_DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

            } else {
    WBAMR_DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

            eError = OMX_ErrorBadPortIndex;
            }
        break;

        case OMX_IndexParamAudioPortFormat:
			WBAMR_DEC_DPRINT ("((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex = %d\n",((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex);
			WBAMR_DEC_DPRINT ("pComponentPrivate->sInPortFormat.nPortIndex= %d\n",pComponentPrivate->sInPortFormat.nPortIndex);
			WBAMR_DEC_DPRINT ("pComponentPrivate->sOutPortFormat.nPortIndex= %d\n",pComponentPrivate->sOutPortFormat.nPortIndex);
			if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->nPortIndex) {

				if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
					pComponentPrivate->sInPortFormat.nPortIndex) {

				eError = OMX_ErrorNoMore;
                }
			else {
				memcpy(ComponentParameterStructure, &pComponentPrivate->sInPortFormat,
					sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
			}
        }
        else if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
            pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->nPortIndex){

			WBAMR_DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex > pComponentPrivate->sOutPortFormat.nPortIndex) {
				WBAMR_DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
				eError = OMX_ErrorNoMore;
			}
			else {
				WBAMR_DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
				memcpy(ComponentParameterStructure, &pComponentPrivate->sOutPortFormat,
                                              sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }
		else {
			WBAMR_DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
            eError = OMX_ErrorBadPortIndex;
        }
        break;

      case OMX_IndexParamAudioAmr:
                       if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex == 
                                pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->nPortIndex) {
                                        WBAMR_DEC_DPRINT ("%d ::OMX_AmrDecoder.c ::Inside the GetParameter\n",__LINE__);
                                        memcpy(ComponentParameterStructure,pComponentPrivate->wbamrParams[WBAMR_DEC_INPUT_PORT], 
                                                sizeof(OMX_AUDIO_PARAM_AMRTYPE));                                       
                        } 
                        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex == 
                                pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->nPortIndex) {
                                        WBAMR_DEC_DPRINT ("%d ::OMX_AmrDecoder.c ::Inside the GetParameter\n",__LINE__);
                                        memcpy(ComponentParameterStructure, pComponentPrivate->wbamrParams[WBAMR_DEC_OUTPUT_PORT], 
                                                sizeof(OMX_AUDIO_PARAM_AMRTYPE));
                                } 
                                else {
                                        eError = OMX_ErrorBadPortIndex;
                                }
                        break;
                        

    case OMX_IndexParamAudioPcm:
                if(((OMX_AUDIO_PARAM_AMRTYPE *)(ComponentParameterStructure))->nPortIndex == WBAMR_DEC_OUTPUT_PORT){
                      memcpy(ComponentParameterStructure, pComponentPrivate->wbamrParams[WBAMR_DEC_OUTPUT_PORT], sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
                }
                else {
                      eError = OMX_ErrorBadPortIndex;
               }
               break;
		case OMX_IndexParamPriorityMgmt:
             memcpy(ComponentParameterStructure, pComponentPrivate->pPriorityMgmt, sizeof(OMX_PRIORITYMGMTTYPE));
		break;

        case OMX_IndexParamCompBufferSupplier:
              if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirInput) {
          	        WBAMR_DEC_DPRINT(":: GetParameter OMX_IndexParamCompBufferSupplier \n");
                    /*  memcpy(ComponentParameterStructure, pBufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE)); */					
				}
				else if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirOutput) {
					WBAMR_DEC_DPRINT(":: GetParameter OMX_IndexParamCompBufferSupplier \n");
					/*memcpy(ComponentParameterStructure, pBufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE)); */
				} 
				else {
					WBAMR_DEC_DPRINT(":: OMX_ErrorBadPortIndex from GetParameter");
					eError = OMX_ErrorBadPortIndex;
				}
                break;

    	case OMX_IndexParamVideoInit:
                break;

         case OMX_IndexParamImageInit:
                break;

         case OMX_IndexParamOtherInit:
                break;
		case (OMX_INDEXTYPE) PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX:
		{
				WBAMR_DEC_DPRINT ("Entering PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX::%d\n", __LINE__);
				PV_OMXComponentCapabilityFlagsType* pCap_flags = (PV_OMXComponentCapabilityFlagsType *) ComponentParameterStructure;
				if (NULL == pCap_flags)
				{
					WBAMR_DEC_DPRINT ("%d :: ERROR PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX\n", __LINE__);
					eError =  OMX_ErrorBadParameter;
					goto EXIT;
				}
				WBAMR_DEC_DPRINT ("%d :: Copying PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX\n", __LINE__);
				memcpy(pCap_flags, &(pComponentPrivate->iPVCapabilityFlags), sizeof(PV_OMXComponentCapabilityFlagsType));
				eError = OMX_ErrorNone;
		}
				break;
				
        default:
            eError = OMX_ErrorUnsupportedIndex;
        break;
    }
EXIT:
    WBAMR_DEC_DPRINT("%d :: Exiting GetParameter:: %x\n",__LINE__,nParamIndex);
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

static OMX_ERRORTYPE SetParameter (OMX_HANDLETYPE hComp,
                                   OMX_INDEXTYPE nParamIndex,
                                   OMX_PTR pCompParam)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BOOL temp_bEnabled,temp_bPopulated;
    OMX_PARAM_COMPONENTROLETYPE  *pRole;
    OMX_AUDIO_PARAM_PCMMODETYPE *amr_op;
    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;

    OMX_COMPONENTTYPE* pHandle= (OMX_COMPONENTTYPE*)hComp;

    WBAMR_DEC_COMPONENT_PRIVATE  *pComponentPrivate;
    pComponentPrivate = (WBAMR_DEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);

	if (pCompParam == NULL) {
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}

#ifdef _ERROR_PROPAGATION__
	if (pComponentPrivate->curState == OMX_StateInvalid){
		eError = OMX_ErrorInvalidState;
		goto EXIT;
	}
#endif
    switch(nParamIndex) {
        case OMX_IndexParamAudioPortFormat:
            {
                OMX_PARAM_PORTDEFINITIONTYPE *pComponentParam =
                                     (OMX_PARAM_PORTDEFINITIONTYPE *)pCompParam;

					WBAMR_DEC_DPRINT("pComponentParam->nPortIndex = %d\n",pComponentParam->nPortIndex);
					WBAMR_DEC_DPRINT("pComponentParam->nBufferCountActual= %d\n",pComponentParam->nBufferCountActual);
					WBAMR_DEC_DPRINT("pComponentParam->nBufferSize= %d\n",pComponentParam->nBufferSize);

                /* 0 means Input port */
                if (pComponentParam->nPortIndex == 0) {
                  
                    memcpy(&pComponentPrivate->sInPortFormat, pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));

                } else if (pComponentParam->nPortIndex == 1) {
                    /* 1 means Output port */
                    memcpy(&pComponentPrivate->sOutPortFormat, pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
                    
               }else {
                   WBAMR_DEC_EPRINT ("%d :: Wrong Port Index Parameter\n", __LINE__);
					WBAMR_DEC_EPRINT("About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
                   eError = OMX_ErrorBadParameter;
                   goto EXIT;
               }
            }
        case OMX_IndexParamAudioAmr:
            {
                OMX_AUDIO_PARAM_AMRTYPE *pCompAmrParam =
                                         (OMX_AUDIO_PARAM_AMRTYPE *)pCompParam;

                 if (OMX_AUDIO_AMRFrameFormatConformance == pCompAmrParam->eAMRFrameFormat) 
                         pComponentPrivate->mimemode = 0;
                 else if (OMX_AUDIO_AMRFrameFormatFSF == pCompAmrParam->eAMRFrameFormat)
                         pComponentPrivate->mimemode = 1;
                else
					pComponentPrivate->mimemode = 2; /*IF2 Format*/
			    
                /* 0 means Input port */
                if(pCompAmrParam->nPortIndex == 0) {
                    memcpy(((WBAMR_DEC_COMPONENT_PRIVATE*)
                            pHandle->pComponentPrivate)->wbamrParams[WBAMR_DEC_INPUT_PORT],
                            pCompAmrParam, sizeof(OMX_AUDIO_PARAM_AMRTYPE));

                } else if (pCompAmrParam->nPortIndex == 1) {
                    /* 1 means Output port */
                    memcpy(((WBAMR_DEC_COMPONENT_PRIVATE *)
                            pHandle->pComponentPrivate)->wbamrParams[WBAMR_DEC_OUTPUT_PORT],
                            pCompAmrParam, sizeof(OMX_AUDIO_PARAM_AMRTYPE));
                }
				else {
					eError = OMX_ErrorBadPortIndex;
				}
            }
            break;

        case OMX_IndexParamPortDefinition:
			if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                                pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->nPortIndex) {
                temp_bEnabled = pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bEnabled;
                temp_bPopulated = pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bPopulated;
				memcpy(pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT],
						pCompParam,
						sizeof(OMX_PARAM_PORTDEFINITIONTYPE)
                      );
                pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bEnabled = temp_bEnabled;
                pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bPopulated = temp_bPopulated;
            }
			else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                              pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->nPortIndex) {
                temp_bEnabled = pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bEnabled;
                temp_bPopulated = pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bPopulated;
            	memcpy(pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT],
						pCompParam,
                       sizeof(OMX_PARAM_PORTDEFINITIONTYPE)
                      );
                pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bEnabled = temp_bEnabled;
                pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bPopulated = temp_bPopulated;

            }
			else {
				eError = OMX_ErrorBadPortIndex;
            }
        break;
		case OMX_IndexParamPriorityMgmt:
			if (pComponentPrivate->curState != OMX_StateLoaded) {
				eError = OMX_ErrorIncorrectStateOperation;
			}
        	memcpy(pComponentPrivate->pPriorityMgmt, (OMX_PRIORITYMGMTTYPE*)pCompParam, sizeof(OMX_PRIORITYMGMTTYPE));
			break;

       case OMX_IndexParamStandardComponentRole: 
		if (pCompParam) {
			   pRole = (OMX_PARAM_COMPONENTROLETYPE *)pCompParam;
		       memcpy(&(pComponentPrivate->componentRole), (void *)pRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
		} else {
			eError = OMX_ErrorBadParameter;
		}
		break;

        case OMX_IndexParamAudioPcm:
		if(pCompParam){
                 amr_op = (OMX_AUDIO_PARAM_PCMMODETYPE *)pCompParam;
                 memcpy(pComponentPrivate->wbamrParams[WBAMR_DEC_OUTPUT_PORT], amr_op, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
		}
		else{
			eError = OMX_ErrorBadParameter;
		}
		break;

        case OMX_IndexParamCompBufferSupplier:             
			if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
									pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->nPortIndex) {
					WBAMR_DEC_DPRINT(":: SetParameter OMX_IndexParamCompBufferSupplier \n");
                                   sBufferSupplier.eBufferSupplier = OMX_BufferSupplyInput;
                                   memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));								   
					
				}
				else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
								  pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->nPortIndex) {
					WBAMR_DEC_DPRINT(":: SetParameter OMX_IndexParamCompBufferSupplier \n");
					sBufferSupplier.eBufferSupplier = OMX_BufferSupplyOutput;
					memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
				} 
				else {
					WBAMR_DEC_DPRINT(":: OMX_ErrorBadPortIndex from SetParameter");
					eError = OMX_ErrorBadPortIndex;
				}
            break;

        default:
            break;

    }
EXIT:
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

static OMX_ERRORTYPE GetConfig (OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR ComponentConfigStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)hComp;
    WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate =
                          (WBAMR_DEC_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;
    TI_OMX_STREAM_INFO *streamInfo;
	//streamInfo = (TI_OMX_STREAM_INFO*)newmalloc(sizeof(TI_OMX_STREAM_INFO));
	WBAMR_DEC_OMX_MALLOC(streamInfo, TI_OMX_STREAM_INFO);
    // if(streamInfo == NULL){
        // eError = OMX_ErrorInsufficientResources;
        // goto EXIT;
    // }
#ifdef _ERROR_PROPAGATION__
	if (pComponentPrivate->curState == OMX_StateInvalid){
		eError = OMX_ErrorInvalidState;
		goto EXIT;
	}
#endif
    if(nConfigIndex == OMX_IndexCustomWbAmrDecStreamIDConfig){
		streamInfo->streamId = pComponentPrivate->streamID;
		memcpy(ComponentConfigStructure,streamInfo,sizeof(TI_OMX_STREAM_INFO));
	}
	OMX_WBDECMEMFREE_STRUCT(streamInfo);
EXIT:
	WBAMR_DEC_DPRINT("%d :: Exiting GetConfig. Returning = 0x%x\n",__LINE__,eError);
    return eError;
}
/*-------------------------------------------------------------------*/
/**
  *  SetConfig() Sets the configraiton to the component
  *
  * @param hComp         handle for this instance of the component
  * @param nConfigIndex
  * @param ComponentConfigStructure
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE SetConfig (OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR ComponentConfigStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)hComp;
    WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate =
                         (WBAMR_DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_S16 *customFlag = NULL;
    TI_OMX_DSP_DEFINITION *configData;
    TI_OMX_DATAPATH dataPath;
#ifdef DSP_RENDERING_ON
    OMX_AUDIO_CONFIG_MUTETYPE *pMuteStructure = NULL;
    OMX_AUDIO_CONFIG_VOLUMETYPE *pVolumeStructure = NULL;
#endif
	WBAMR_DEC_DPRINT("%d :: Entering SetConfig\n", __LINE__);
	if (pHandle == NULL) {
		WBAMR_DEC_DPRINT ("%d :: Invalid HANDLE OMX_ErrorBadParameter \n",__LINE__);
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}

#ifdef _ERROR_PROPAGATION__
	if (pComponentPrivate->curState == OMX_StateInvalid){
		eError = OMX_ErrorInvalidState;
		goto EXIT;
	}
#endif
	switch (nConfigIndex) {
        case OMX_IndexCustomWbAmrDecHeaderInfoConfig:
        {
            WBAMR_DEC_DPRINT("%d :: SetConfig OMX_IndexCustomWbAmrDecHeaderInfoConfig \n",__LINE__);
            configData = (TI_OMX_DSP_DEFINITION*)ComponentConfigStructure;
            if (configData == NULL) {
				eError = OMX_ErrorBadParameter;
				WBAMR_DEC_EPRINT("%d :: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
			 	goto EXIT;
			}
            pComponentPrivate->acdnmode = configData->acousticMode;
            pComponentPrivate->dasfmode = configData->dasfMode;
            if( 2 == pComponentPrivate->dasfmode ){
                   pComponentPrivate->dasfmode--;
            }

			if (pComponentPrivate->dasfmode ){
                    pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bEnabled = 0;
            }

            pComponentPrivate->streamID = configData->streamId;
            break;
        }
/************************/
case  OMX_IndexCustomWbAmrDecDataPath:
            customFlag = (OMX_S16*)ComponentConfigStructure;
            if (customFlag == NULL) {
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }

    /*        dataPath = *customFlag; */

            switch(dataPath) {
                case DATAPATH_APPLICATION:
                    /*strcpy((char*)pComponentPrivate->sDeviceString,(char*)ETEEDN_STRING);*/
					OMX_MMMIXER_DATAPATH(pComponentPrivate->sDeviceString, RENDERTYPE_DECODER, pComponentPrivate->streamID);
                break;

                case DATAPATH_APPLICATION_RTMIXER:
                    strcpy((char*)pComponentPrivate->sDeviceString,(char*)RTM_STRING);
                break;

                case DATAPATH_ACDN:
                    strcpy((char*)pComponentPrivate->sDeviceString,(char*)ACDN_STRING);
                break;

                default:
                break;
                
            }
        break;

/************************/        
		case OMX_IndexCustomModeDasfConfig_WBAMRDEC:
		{
			WBAMR_DEC_DPRINT("%d :: SetConfig OMX_IndexCustomModeDasfConfig \n",__LINE__);
			customFlag = (OMX_S16*)ComponentConfigStructure;
			if (customFlag == NULL) {
				eError = OMX_ErrorBadParameter;
				WBAMR_DEC_EPRINT("%d :: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
				goto EXIT;
			}
			pComponentPrivate->dasfmode = *customFlag;
			if( 2 == pComponentPrivate->dasfmode ){
                 pComponentPrivate->dasfmode--;
            }
			if (pComponentPrivate->dasfmode){
               pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bEnabled = 0;
            }
			break;
		}
		case OMX_IndexCustomModeMimeConfig_WBAMRDEC:
		{
			WBAMR_DEC_DPRINT("%d :: SetConfig OMX_IndexCustomModeMimeConfig \n",__LINE__);
			customFlag = (OMX_S16*)ComponentConfigStructure;
			if (customFlag == NULL) 
            {
				eError = OMX_ErrorBadParameter;
				WBAMR_DEC_EPRINT("%d :: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
				goto EXIT;
			}
			pComponentPrivate->mimemode = *customFlag;
			break;
		}
        case OMX_IndexCustomWbAmrDecNextFrameLost:
        {
  	  	    pComponentPrivate->bFrameLost=OMX_TRUE;
  	  	  	break;
        }
        case OMX_IndexConfigAudioMute:
        {
#ifdef DSP_RENDERING_ON
             pMuteStructure = (OMX_AUDIO_CONFIG_MUTETYPE *)ComponentConfigStructure;
             WBAMR_DEC_DPRINT("Set Mute/Unmute for playback stream\n");
             cmd_data.hComponent = hComp;
             if(pMuteStructure->bMute == OMX_TRUE)
             {
                 WBAMR_DEC_DPRINT("Mute the playback stream\n");
                 cmd_data.AM_Cmd = AM_CommandStreamMute;
             }
             else
             {
                 WBAMR_DEC_DPRINT("unMute the playback stream\n");
                 cmd_data.AM_Cmd = AM_CommandStreamUnMute;
             }
             cmd_data.param1 = 0;
             cmd_data.param2 = 0;
             cmd_data.streamID = pComponentPrivate->streamID;
             if((write(pComponentPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0)
             {
                 WBAMR_DEC_EPRINT("[WBAMR decoder] - fail to send Mute command to audio manager\n");
             }
#endif
             break;
        }
        case OMX_IndexConfigAudioVolume:
        {
#ifdef DSP_RENDERING_ON
             pVolumeStructure = (OMX_AUDIO_CONFIG_VOLUMETYPE *)ComponentConfigStructure;
             WBAMR_DEC_DPRINT("Set volume for playback stream\n");
             cmd_data.hComponent = hComp;
             cmd_data.AM_Cmd = AM_CommandSWGain;
             cmd_data.param1 = pVolumeStructure->sVolume.nValue;
             cmd_data.param2 = 0;
             cmd_data.streamID = pComponentPrivate->streamID;

             if((write(pComponentPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0)
             {
                 WBAMR_DEC_EPRINT("[WBAMR decoder] - fail to send Volume command to audio manager\n");
             }

#endif
             break;
        }

		default:
			eError = OMX_ErrorUnsupportedIndex;
		break;
	}
EXIT:
	WBAMR_DEC_DPRINT("%d :: Exiting SetConfig\n", __LINE__);
	WBAMR_DEC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    OMX_ERRORTYPE error = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;

    if (!pState) {
        error = OMX_ErrorBadParameter;
		WBAMR_DEC_EPRINT("About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
        goto EXIT;
    }

    if (pHandle && pHandle->pComponentPrivate) {
        *pState =  ((WBAMR_DEC_COMPONENT_PRIVATE*)
                                     pHandle->pComponentPrivate)->curState;
    } else {
        *pState = OMX_StateLoaded;
    }

    error = OMX_ErrorNone;

EXIT:
    return error;
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
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate =
                         (WBAMR_DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;

	int ret;
    pPortDef = ((WBAMR_DEC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[WBAMR_DEC_INPUT_PORT];
#ifdef _ERROR_PROPAGATION__
	if (pComponentPrivate->curState == OMX_StateInvalid){
		eError = OMX_ErrorInvalidState;
		goto EXIT;
	}
#endif

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                       pBuffer->pBuffer,
                       pBuffer->nFilledLen,
                       PERF_ModuleHLMM);
#endif

    if(!pPortDef->bEnabled) {
		eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
	}

    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
		WBAMR_DEC_EPRINT("About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
        goto EXIT;
    }

	if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}

	if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
		eError = OMX_ErrorVersionMismatch;
		goto EXIT;
	}

	if (pBuffer->nInputPortIndex != WBAMR_DEC_INPUT_PORT) {
		eError  = OMX_ErrorBadPortIndex;
		goto EXIT;
	}

    if(pComponentPrivate->curState != OMX_StateExecuting && pComponentPrivate->curState != OMX_StatePause) {
        eError = OMX_ErrorIncorrectStateOperation;
		goto EXIT;
	}



    WBAMR_DEC_DPRINT("\n------------------------------------------\n\n");
    WBAMR_DEC_DPRINT ("%d :: Component Sending Filled ip buff %p \
                             to Component Thread\n",__LINE__,pBuffer);
    WBAMR_DEC_DPRINT("\n------------------------------------------\n\n");


    pComponentPrivate->app_nBuf--;

    pComponentPrivate->pMarkData = pBuffer->pMarkData;
    pComponentPrivate->hMarkTargetComponent = pBuffer->hMarkTargetComponent;

    pComponentPrivate->nUnhandledEmptyThisBuffers++;

    ret = write (pComponentPrivate->dataPipe[1], &pBuffer,
                                       sizeof(OMX_BUFFERHEADERTYPE*));
    if (ret == -1) {
        WBAMR_DEC_EPRINT ("%d :: Error in Writing to the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
	pComponentPrivate->nEmptyThisBufferCount++;

EXIT:
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
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate =
                         (WBAMR_DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    WBAMR_DEC_DPRINT("\n------------------------------------------\n\n");
    WBAMR_DEC_DPRINT ("%d :: Component Sending Emptied op buff %p \
                             to Component Thread\n",__LINE__,pBuffer);
    WBAMR_DEC_DPRINT("\n------------------------------------------\n\n");

    pPortDef = ((WBAMR_DEC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[WBAMR_DEC_OUTPUT_PORT];
#ifdef _ERROR_PROPAGATION__
	if (pComponentPrivate->curState == OMX_StateInvalid){
		eError = OMX_ErrorInvalidState;
		goto EXIT;
	}
#endif
#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                       pBuffer->pBuffer,
                       0,
                       PERF_ModuleHLMM);
#endif

    if(!pPortDef->bEnabled) {
		eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
	}

    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
		WBAMR_DEC_EPRINT("About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
        goto EXIT;
    }

	if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}


	if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
		eError = OMX_ErrorVersionMismatch;
		goto EXIT;
	}

	if (pBuffer->nOutputPortIndex != WBAMR_DEC_OUTPUT_PORT) {
		eError  = OMX_ErrorBadPortIndex;
		goto EXIT;
	}

    if(pComponentPrivate->curState != OMX_StateExecuting && pComponentPrivate->curState != OMX_StatePause) {
        eError = OMX_ErrorIncorrectStateOperation;
		goto EXIT;
	}


	WBAMR_DEC_DPRINT("FillThisBuffer Line %d\n",__LINE__);
    pComponentPrivate->app_nBuf--;
    WBAMR_DEC_DPRINT("%d:Decrementing app_nBuf = %d\n",__LINE__,pComponentPrivate->app_nBuf);

    WBAMR_DEC_DPRINT("pComponentPrivate->pMarkBuf = 0x%x\n",pComponentPrivate->pMarkBuf);
    WBAMR_DEC_DPRINT("pComponentPrivate->pMarkData = 0x%x\n",pComponentPrivate->pMarkData);
    if(pComponentPrivate->pMarkBuf){
        WBAMR_DEC_DPRINT("FillThisBuffer Line %d\n",__LINE__);
        pBuffer->hMarkTargetComponent = pComponentPrivate->pMarkBuf->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkBuf->pMarkData;
        pComponentPrivate->pMarkBuf = NULL;
    }

    if (pComponentPrivate->pMarkData) {
        WBAMR_DEC_DPRINT("FillThisBuffer Line %d\n",__LINE__);
        pBuffer->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkData;
        pComponentPrivate->pMarkData = NULL;
    }

    pComponentPrivate->nUnhandledFillThisBuffers++;

    write (pComponentPrivate->dataPipe[1], &pBuffer,
                                      sizeof (OMX_BUFFERHEADERTYPE*));
	pComponentPrivate->nFillThisBufferCount++;

EXIT:
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

    /* inform audio manager to remove the streamID*/
    /* compose the data */
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
    WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate =
                         (WBAMR_DEC_COMPONENT_PRIVATE *)pComponent->pComponentPrivate;


    WBAMR_DEC_DPRINT ("%d ::ComponentDeInit\n",__LINE__);

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif
#ifdef DSP_RENDERING_ON 
    close(pComponentPrivate->fdwrite);
    close(pComponentPrivate->fdread);
#endif

#ifdef RESOURCE_MANAGER_ENABLED
/*RM*/
    /* eError = RMProxy_SendCommand(pHandle, RMProxy_FreeResource, OMX_WBAMR_Decoder_COMPONENT, 0, NULL); */
	eError = RMProxy_NewSendCommand(pHandle, RMProxy_FreeResource, OMX_WBAMR_Decoder_COMPONENT, 0, 3456, NULL);

    if (eError != OMX_ErrorNone) {
         WBAMR_DEC_EPRINT ("%d ::Error returned from destroy ResourceManagerProxy thread\n",
                                                        __LINE__);
    }
    eError = RMProxy_Deinitalize();
    if (eError != OMX_ErrorNone) {
         WBAMR_DEC_EPRINT ("%d ::Error from RMProxy_Deinitalize\n",
                                                        __LINE__);
    }
/*RM END*/
#endif

    WBAMR_DEC_DPRINT ("%d ::ComponentDeInit\n",__LINE__);
    WBAMR_DEC_DPRINT ("%d ::ComponentDeInit\n",__LINE__);
    pComponentPrivate->bIsStopping = 1;
    eError = WBAMR_DEC_StopComponentThread(pHandle);
    WBAMR_DEC_DPRINT ("%d ::ComponentDeInit\n",__LINE__);
    /* Wait for thread to exit so we can get the status into "error" */


    if(pComponentPrivate->pInputBufferList!=NULL){
                WBAMR_DEC_DPRINT("%d:[FREE] %p\n",__LINE__,pComponentPrivate->pInputBufferList);
                OMX_WBDECMEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
    }

    if(pComponentPrivate->pOutputBufferList!=NULL){
                 WBAMR_DEC_DPRINT("%d:[FREE] %p\n",__LINE__,pComponentPrivate->pOutputBufferList);
                 OMX_WBDECMEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);
    }


    /* close the pipe handles */
    WBAMR_DEC_FreeCompResources(pHandle);

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryComplete | PERF_BoundaryCleanup);
    PERF_Done(pComponentPrivate->pPERF);
#endif
    if (pComponentPrivate->sDeviceString != NULL) {
        OMX_WBDECMEMFREE_STRUCT(pComponentPrivate->sDeviceString);
    }
    WBAMR_DEC_DPRINT ("%d ::After WBAMR_DEC_FreeCompResources\n",__LINE__);

    OMX_WBDECMEMFREE_STRUCT(pComponentPrivate);
/*This previous MACRO replaces the next three lines of code and tests conditions in order to proceed with free()
Can be of good practice replacing all of them with the macro*/
/*    WBAMR_DEC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pComponentPrivate);
    free(pComponentPrivate);
    pComponentPrivate = NULL;*/

    WBAMR_DEC_DPRINT ("%d ::After newfree(pComponentPrivate)\n",__LINE__);

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
    WBAMR_DEC_DPRINT (stderr, "Inside the ComponentTunnelRequest\n");
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
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader;

    pComponentPrivate = (WBAMR_DEC_COMPONENT_PRIVATE *)
            (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pPortDef = ((WBAMR_DEC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[nPortIndex];
#ifdef _ERROR_PROPAGATION__
	if (pComponentPrivate->curState == OMX_StateInvalid){
		eError = OMX_ErrorInvalidState;
		goto EXIT;
	}
#endif
    WBAMR_DEC_DPRINT ("%d :: pPortDef = 0x%x\n", __LINE__,pPortDef);
    WBAMR_DEC_DPRINT ("%d :: pPortDef->bEnabled = %d\n", __LINE__,pPortDef->bEnabled);

    WBAMR_DEC_DPRINT ("pPortDef->bEnabled = %d\n", pPortDef->bEnabled);
	while (1) {
		if(pPortDef->bEnabled) {
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
    //pBufferHeader = (OMX_BUFFERHEADERTYPE*)newmalloc(sizeof(OMX_BUFFERHEADERTYPE));
	WBAMR_DEC_OMX_MALLOC(pBufferHeader, OMX_BUFFERHEADERTYPE);
	// if (pBufferHeader == NULL) {
		// eError = OMX_ErrorInsufficientResources;
		// goto EXIT;
	// }
	// memset(pBufferHeader, 0x0, sizeof(OMX_BUFFERHEADERTYPE));

	//pBufferHeader->pBuffer = (OMX_U8 *)newmalloc(nSizeBytes + 256);
	WBAMR_DEC_OMX_MALLOC_SIZE(pBufferHeader->pBuffer, (nSizeBytes + DSP_CACHE_ALIGNMENT),OMX_U8);
	
	// if (pBufferHeader->pBuffer == NULL) {
		// /* Free previously allocated memory before bailing */
		// if (pBufferHeader) {
			// OMX_WBDECMEMFREE_STRUCT(pBufferHeader);
			// pBufferHeader = NULL;
		// }
		// eError = OMX_ErrorInsufficientResources;
		// goto EXIT;
	// }
	// memset(pBufferHeader->pBuffer, 0x0, (nSizeBytes + 256));
    pBufferHeader->pBuffer += EXTRA_BYTES;

	if (nPortIndex == WBAMR_DEC_INPUT_PORT) {
        pBufferHeader->nInputPortIndex = nPortIndex;
		pBufferHeader->nOutputPortIndex = -1;
		pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
		pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;
		WBAMR_DEC_DPRINT("pComponentPrivate->pInputBufferList->pBufHdr[%d] = %p\n",pComponentPrivate->pInputBufferList->numBuffers,pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers]);
		pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 1;
		WBAMR_DEC_DPRINT("Allocate WBAMR_DEC_Buffer Line %d\n",__LINE__);
		WBAMR_DEC_DPRINT("pComponentPrivate->pInputBufferList->numBuffers = %d\n",pComponentPrivate->pInputBufferList->numBuffers);
		WBAMR_DEC_DPRINT("pPortDef->nBufferCountMin = %d\n",pPortDef->nBufferCountMin);
		if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) {
			WBAMR_DEC_DPRINT("Setting pPortDef->bPopulated = OMX_TRUE for input port\n");
			pPortDef->bPopulated = OMX_TRUE;
		}
	}
	else if (nPortIndex == WBAMR_DEC_OUTPUT_PORT) {
        pBufferHeader->nInputPortIndex = -1;
		pBufferHeader->nOutputPortIndex = nPortIndex;
        //pBufferHeader->pOutputPortPrivate = (WBAMRDEC_BUFDATA*) malloc(sizeof(WBAMRDEC_BUFDATA));
		WBAMR_DEC_OMX_MALLOC(pBufferHeader->pOutputPortPrivate, WBAMRDEC_BUFDATA); 		
		pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
		pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
		WBAMR_DEC_DPRINT("pComponentPrivate->pOutputBufferList->pBufHdr[%d] = %p\n",pComponentPrivate->pOutputBufferList->numBuffers,pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers]);
		pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 1;
		if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) {
			WBAMR_DEC_DPRINT("Setting pPortDef->bPopulated = OMX_TRUE for input port\n");
			pPortDef->bPopulated = OMX_TRUE;
		}
	}
	else {
		eError = OMX_ErrorBadPortIndex;
		goto EXIT;
	}
    if((pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bEnabled) &&
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
    pBufferHeader->nVersion.s.nVersionMajor = WBAMR_DEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = WBAMR_DEC_MINOR_VER;
	pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;


	pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    WBAMR_DEC_DPRINT("Line %d\n",__LINE__);
    *pBuffer = pBufferHeader;

	if (pComponentPrivate->bEnableCommandPending && pPortDef->bPopulated) {
        SendCommand (pComponentPrivate->pHandle,
                     OMX_CommandPortEnable,
                     pComponentPrivate->bEnableCommandParam,NULL);
    }
EXIT:
#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedBuffer(pComponentPrivate->pPERF,
                        (*pBuffer)->pBuffer, nSizeBytes,
                        PERF_ModuleMemory);
#endif
	WBAMR_DEC_DPRINT("AllocateBuffer returning %d\n",eError);
    return eError;
}

/* ================================================================================= */
/**
* @fn FreeBuffer() description for FreeBuffer
FreeBuffer().
Called by the OMX IL client to newfree a buffer.
*
*  @see         OMX_Core.h
*/
/* ================================================================================ */

static OMX_ERRORTYPE FreeBuffer(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_U32 nPortIndex,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    WBAMR_DEC_COMPONENT_PRIVATE * pComponentPrivate = NULL;
    OMX_BUFFERHEADERTYPE* buff;
	OMX_U8* tempBuff;
	int i;
	int inputIndex = -1;
	int outputIndex = -1;
    OMX_COMPONENTTYPE *pHandle;

    pComponentPrivate = (WBAMR_DEC_COMPONENT_PRIVATE *)
            (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

	pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
		for (i=0; i < WBAMR_DEC_MAX_NUM_OF_BUFS; i++) {
			buff = pComponentPrivate->pInputBufferList->pBufHdr[i];
			if (buff == pBuffer) {
				WBAMR_DEC_DPRINT("Found matching input buffer\n");
				WBAMR_DEC_DPRINT("buff = %p\n",buff);
				WBAMR_DEC_DPRINT("pBuffer = %p\n",pBuffer);
				inputIndex = i;
				break;
			}
			else {
				WBAMR_DEC_DPRINT("This is not a match\n");
				WBAMR_DEC_DPRINT("buff = %p\n",buff);
				WBAMR_DEC_DPRINT("pBuffer = %p\n",pBuffer);
			}
		}

		for (i=0; i < WBAMR_DEC_MAX_NUM_OF_BUFS; i++) {
			buff = pComponentPrivate->pOutputBufferList->pBufHdr[i];
			if (buff == pBuffer) {
				WBAMR_DEC_DPRINT("Found matching output buffer\n");
				WBAMR_DEC_DPRINT("buff = %p\n",buff);
				WBAMR_DEC_DPRINT("pBuffer = %p\n",pBuffer);
				outputIndex = i;
				break;
			}
			else {
				WBAMR_DEC_DPRINT("This is not a match\n");
				WBAMR_DEC_DPRINT("buff = %p\n",buff);
				WBAMR_DEC_DPRINT("pBuffer = %p\n",pBuffer);
			}
		}


		if (inputIndex != -1) {
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingBuffer(pComponentPrivate->pPERF,
                 pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer,
				 pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->nAllocLen,
                 PERF_ModuleMemory);
#endif
			if (pComponentPrivate->pInputBufferList->bufferOwner[inputIndex] == 1) {
				tempBuff = pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer;
				if (tempBuff != 0){
                   tempBuff -= EXTRA_BYTES;
				}
				WBAMR_DEC_MEMPRINT("%d:[FREE] %p\n",__LINE__,tempBuff);
                OMX_WBDECMEMFREE_STRUCT(tempBuff);
				tempBuff = NULL;
			}
			WBAMR_DEC_MEMPRINT("%d:[FREE] %p\n",__LINE__,pComponentPrivate->pBufHeader[WBAMR_DEC_INPUT_PORT]);
            OMX_WBDECMEMFREE_STRUCT(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);
			pComponentPrivate->pInputBufferList->pBufHdr[inputIndex] = NULL;
			pComponentPrivate->pInputBufferList->numBuffers--;
			if (pComponentPrivate->pInputBufferList->numBuffers <
				pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->nBufferCountMin) {

				pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bPopulated = OMX_FALSE;
			}
			if(pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bEnabled &&
                pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&			
				(pComponentPrivate->curState == OMX_StateIdle ||
				pComponentPrivate->curState == OMX_StateExecuting ||
				pComponentPrivate->curState == OMX_StatePause)) {
				pComponentPrivate->cbInfo.EventHandler(
						pHandle, pHandle->pApplicationPrivate,
						OMX_EventError, OMX_ErrorPortUnpopulated,OMX_TI_ErrorMinor, "Input Port Unpopulated");
			}
		}
		else if (outputIndex != -1) {
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingBuffer(pComponentPrivate->pPERF,
                 pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer,
				 pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->nAllocLen,
                 PERF_ModuleMemory);
#endif
			if (pComponentPrivate->pOutputBufferList->bufferOwner[outputIndex] == 1) {
				tempBuff = pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer;
				if (tempBuff != 0){
                                    tempBuff -= EXTRA_BYTES;
                                }
				WBAMR_DEC_MEMPRINT("%d:[FREE] %p\n",__LINE__,tempBuff);
				OMX_WBDECMEMFREE_STRUCT(tempBuff);
				tempBuff = NULL;
			}
            OMX_WBDECMEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pOutputPortPrivate);
            OMX_WBDECMEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);
			pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex] = NULL;
			pComponentPrivate->pOutputBufferList->numBuffers--;

			if (pComponentPrivate->pOutputBufferList->numBuffers <
				pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->nBufferCountMin) {
				pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bPopulated = OMX_FALSE;
			}
			if(pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bEnabled &&
    			pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
				(pComponentPrivate->curState == OMX_StateIdle ||
				pComponentPrivate->curState == OMX_StateExecuting ||
				pComponentPrivate->curState == OMX_StatePause)) {
				pComponentPrivate->cbInfo.EventHandler(
						pHandle, pHandle->pApplicationPrivate,
						OMX_EventError, OMX_ErrorPortUnpopulated,OMX_TI_ErrorMinor, "Output Port Unpopulated");
			}
		}
		else {
			WBAMR_DEC_EPRINT("%d::Returning OMX_ErrorBadParameter\n",__LINE__);
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
if (pComponentPrivate->bDisableCommandPending && (pComponentPrivate->pInputBufferList->numBuffers + pComponentPrivate->pOutputBufferList->numBuffers == 0)) {
				SendCommand (pComponentPrivate->pHandle,OMX_CommandPortDisable,pComponentPrivate->bDisableCommandParam,NULL);
			}

    WBAMR_DEC_DPRINT ("%d :: Exiting FreeBuffer\n", __LINE__);
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

static OMX_ERRORTYPE UseBuffer (
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes,
            OMX_IN OMX_U8* pBuffer)
{
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader;

    pComponentPrivate = (WBAMR_DEC_COMPONENT_PRIVATE *)
            (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

#ifdef _ERROR_PROPAGATION__
	if (pComponentPrivate->curState == OMX_StateInvalid){
		eError = OMX_ErrorInvalidState;
		goto EXIT;
	}
#endif
    pPortDef = ((WBAMR_DEC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[nPortIndex];
	WBAMR_DEC_DPRINT("pPortDef->bPopulated = %d\n",pPortDef->bPopulated);

    if(!pPortDef->bEnabled) {
        WBAMR_DEC_DPRINT ("%d :: In UseBuffer\n", __LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    /*if(nSizeBytes != pPortDef->nBufferSize || pPortDef->bPopulated) {
        WBAMR_DEC_DPRINT ("%d :: In UseBuffer\n", __LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }*/

    //pBufferHeader = (OMX_BUFFERHEADERTYPE*)newmalloc(sizeof(OMX_BUFFERHEADERTYPE));
	WBAMR_DEC_OMX_MALLOC(pBufferHeader, OMX_BUFFERHEADERTYPE); 
	
	// if (pBufferHeader == 0) {
		// WBAMR_DEC_EPRINT("newmalloc failed\n");
		// eError = OMX_ErrorInsufficientResources;
		// goto EXIT;
	// }
    // memset((pBufferHeader), 0x0, sizeof(OMX_BUFFERHEADERTYPE));
    if (nPortIndex == WBAMR_DEC_OUTPUT_PORT) {
        pBufferHeader->nInputPortIndex = -1;
		pBufferHeader->nOutputPortIndex = nPortIndex;
                //pBufferHeader->pOutputPortPrivate = (WBAMRDEC_BUFDATA*) malloc(sizeof(WBAMRDEC_BUFDATA));
				WBAMR_DEC_OMX_MALLOC(pBufferHeader->pOutputPortPrivate, WBAMRDEC_BUFDATA); 
		pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
		pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
		pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 0;
		if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) {
			pPortDef->bPopulated = OMX_TRUE;
		}
    }
    else {
        pBufferHeader->nInputPortIndex = nPortIndex;
		pBufferHeader->nOutputPortIndex = -1;
		pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
		pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;
		pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 0;
		if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) {
			pPortDef->bPopulated = OMX_TRUE;
		}
    }

    if((pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bEnabled) &&
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
    pBufferHeader->nVersion.s.nVersionMajor = WBAMR_DEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = WBAMR_DEC_MINOR_VER;
	pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
	pBufferHeader->pBuffer = pBuffer;
	pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *ppBufferHdr = pBufferHeader;
	WBAMR_DEC_DPRINT("pBufferHeader = %p\n",pBufferHeader);
EXIT:
#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedBuffer(pComponentPrivate->pPERF,
                        pBuffer, nSizeBytes,
                        PERF_ModuleHLMM);
#endif
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

	if(!(strcmp(cParameterName,"OMX.TI.index.config.wbamrheaderinfo"))) {
         *pIndexType = OMX_IndexCustomWbAmrDecHeaderInfoConfig;
         }
	else if(!(strcmp(cParameterName,"OMX.TI.index.config.wbamrstreamIDinfo"))) {
		*pIndexType = OMX_IndexCustomWbAmrDecStreamIDConfig;
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.wbamr.datapath"))) 
	{
		*pIndexType = OMX_IndexCustomWbAmrDecDataPath;
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.wbamr.framelost")))
  	{
  	    *pIndexType = OMX_IndexCustomWbAmrDecNextFrameLost;
  	}
    else {
		eError = OMX_ErrorBadParameter;
	}

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
    WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate;
    
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    pComponentPrivate = (WBAMR_DEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    if(nIndex == 0){
      memcpy(cRole, &pComponentPrivate->componentRole.cRole, sizeof(OMX_U8) * OMX_MAX_STRINGNAME_SIZE); 
      WBAMR_DEC_DPRINT("::::In ComponenetRoleEnum: cRole is set to %s\n",cRole);
    }
    else {
      eError = OMX_ErrorNoMore;
    	}
    return eError;
}

#ifdef WBAMRDEC_DEBUGMEM
void * mymalloc(int line, char *s, int size)
{
   void *p;
   int e=0;
   p = malloc(size);
   if(p==NULL){
       WBAMR_DEC_EPRINT("Memory not available\n");
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
         WBAMR_DEC_DPRINT("Allocating %d bytes on address %p, line %d file %s\n", size, p, line, s);
         return p;
   }
}

int myfree(void *dp, int line, char *s){
    int q;
    if (dp==NULL){
       WBAMR_DEC_EPRINT("Null Memory can not be deleted line: %d  file: %s\n", line, s);
       return 0;
    }

    for(q=0;q<500;q++){
        if(arr[q]==dp){
           WBAMR_DEC_DPRINT("Deleting %d bytes on address %p, line %d file %s\n", bytes[q],dp, line, s);
           free(dp);
           dp = NULL;
           lines[q]=0;
           strcpy(file[q],"");
           break;
        }
     }
     if(500==q)
         WBAMR_DEC_EPRINT("\n\nPointer not found. Line:%d    File%s!!\n\n",line, s);
}
#endif
