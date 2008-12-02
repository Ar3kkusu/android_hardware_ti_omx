
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
*             Texas Instruments OMAP(TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found 
*  in the license agreement under which this software has been supplied.
* =========================================================================== */
/**
* @file OMX_Video_Dec_Thread.c
*
* This file implements OMX Component for MPEG-4 decoder that 
* is fully compliant with the Khronos OMX specification 1.0.
*
* @path  $(CSLPATH)\src
*
* @rev  0.1
*/
/* -------------------------------------------------------------------------- */
/* ============================================================================= 
*! 
*! Revision History 
*! =============================================================================
*!
*! 02-Feb-2006 mf: Revisions appear in reverse chronological order; 
*! that is, newest first.  The date format is dd-Mon-yyyy.  
* =========================================================================== */

/* ------compilation control switches ----------------------------------------*/
/*******************************************************************************
*  INCLUDE FILES                                                 
*******************************************************************************/
/* ----- system and platform files -------------------------------------------*/
#ifdef UNDER_CE
    #include <windows.h>
    #include <oaf_osal.h>
    #include <omx_core.h>
#else
#define _XOPEN_SOURCE 600
    #include <sys/select.h>
    #include <signal.h>   
    #include <unistd.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <fcntl.h>
    #include <errno.h>
     
#endif 

#include <dbapi.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "OMX_VideoDecoder.h"
#include "OMX_VideoDec_Utils.h"
#include "OMX_VideoDec_Thread.h"
#include "OMX_VideoDec_DSP.h"


extern OMX_ERRORTYPE VIDDEC_HandleCommand (OMX_HANDLETYPE pHandle, OMX_U32 nParam1);
extern OMX_ERRORTYPE VIDDEC_DisablePort(VIDDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1);
extern OMX_ERRORTYPE VIDDEC_EnablePort(VIDDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1);
extern OMX_ERRORTYPE VIDDEC_HandleDataBuf_FromApp( VIDDEC_COMPONENT_PRIVATE *pComponentPrivate);
extern OMX_ERRORTYPE VIDDEC_HandleDataBuf_FromDsp( VIDDEC_COMPONENT_PRIVATE *pComponentPrivate );
extern OMX_ERRORTYPE VIDDEC_HandleFreeDataBuf( VIDDEC_COMPONENT_PRIVATE *pComponentPrivate );
extern OMX_ERRORTYPE VIDDEC_HandleFreeOutputBufferFromApp(VIDDEC_COMPONENT_PRIVATE *pComponentPrivate) ;
extern OMX_ERRORTYPE VIDDEC_Start_ComponentThread(OMX_HANDLETYPE pHandle);
extern OMX_ERRORTYPE VIDDEC_Stop_ComponentThread(OMX_HANDLETYPE pComponent);
extern OMX_ERRORTYPE VIDDEC_HandleCommandMarkBuffer(VIDDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1, OMX_PTR pCmdData);
extern OMX_ERRORTYPE VIDDEC_HandleCommandFlush(VIDDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1, OMX_BOOL bPass);
extern OMX_ERRORTYPE VIDDEC_Handle_InvalidState (VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);

/*----------------------------------------------------------------------------*/
/**
  * OMX_VidDec_Thread() is the open max thread. This method is in charge of
  * listening to the buffers coming from DSP, application or commands through the pipes
  **/
/*----------------------------------------------------------------------------*/

/** Default timeout used to come out of blocking calls*/
#define VIDD_TIMEOUT (1000) /* milliseconds */

void* OMX_VidDec_Thread (void* pThreadData)
{
    int status;
#ifdef UNDER_CE
    struct timeval tv;
#else
    sigset_t set;
    struct timespec tv;
#endif
    int fdmax;
    fd_set rfds;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMMANDTYPE eCmd;
    OMX_U32 nParam1;
    OMX_PTR pCmdData;
    VIDDEC_COMPONENT_PRIVATE* pComponentPrivate;
    LCML_DSP_INTERFACE *pLcmlHandle;
    OMX_U32 aParam[4];
#ifndef UNDER_CE
    OMX_BOOL bFlag = OMX_FALSE;
#endif
    /*OMX_U32 timeout = 0;*/

    pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)pThreadData;

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERFcomp = PERF_Create(PERF_FOURS("VD T"),
                                               PERF_ModuleComponent | PERF_ModuleVideoDecode);
#endif

    pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate->pLCML;

    /**Looking for highest number of file descriptor for pipes in order to put in select loop */
    fdmax = pComponentPrivate->cmdPipe[VIDDEC_PIPE_READ];

    if (pComponentPrivate->free_inpBuf_Q[VIDDEC_PIPE_READ] > fdmax) {
        fdmax = pComponentPrivate->free_inpBuf_Q[VIDDEC_PIPE_READ];
    }

    if (pComponentPrivate->free_outBuf_Q[VIDDEC_PIPE_READ] > fdmax) {
        fdmax = pComponentPrivate->free_outBuf_Q[VIDDEC_PIPE_READ];
    }

    if (pComponentPrivate->filled_inpBuf_Q[VIDDEC_PIPE_READ] > fdmax) {
        fdmax = pComponentPrivate->filled_inpBuf_Q[VIDDEC_PIPE_READ];
    }

    if (pComponentPrivate->filled_outBuf_Q[VIDDEC_PIPE_READ] > fdmax) {
        fdmax = pComponentPrivate->filled_outBuf_Q[VIDDEC_PIPE_READ];
    }

    while (1) {
#if 1
 #ifdef VIDDEC_FLAGGED_EOS
        if (!pComponentPrivate->bUseFlaggedEos) { /*OMX_TRUE*/
 #endif
            if(pComponentPrivate->bPlayCompleted == OMX_TRUE && pComponentPrivate->bBuffFound == OMX_FALSE
                && pComponentPrivate->bFlushOut == OMX_FALSE){
                OMX_VidDec_Return(pComponentPrivate);
                OMX_VidDec_Return(pComponentPrivate);
                pComponentPrivate->bPipeCleaned = 0;
                aParam[0] = USN_STRMCMD_FLUSH;
                aParam[1] = VIDDEC_OUTPUT_PORT;
                aParam[2] = 0;
                VIDDEC_PTHREAD_MUTEX_LOCK(pComponentPrivate->sMutex);
                if(pComponentPrivate->eLCMLState != VidDec_LCML_State_Unload && 
                    pComponentPrivate->eLCMLState != VidDec_LCML_State_Destroy && 
                    pComponentPrivate->pLCML != NULL){
                    pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,EMMCodecControlStrmCtrl, (void*)aParam);
                    if (eError != OMX_ErrorNone) {
                        eError = OMX_ErrorHardware;
                        VIDDECODER_EPRINT("Error on flush at out\n");
                    }
                    VIDDEC_PTHREAD_MUTEX_WAIT(pComponentPrivate->sMutex);
                    VIDDEC_PTHREAD_MUTEX_UNLOCK(pComponentPrivate->sMutex);
               }
               pComponentPrivate->bFlushOut = OMX_TRUE;
            }
 #ifdef VIDDEC_FLAGGED_EOS
        }
 #endif
#endif

        FD_ZERO (&rfds);
        FD_SET(pComponentPrivate->cmdPipe[VIDDEC_PIPE_READ], &rfds);
        FD_SET(pComponentPrivate->filled_outBuf_Q[VIDDEC_PIPE_READ], &rfds);
        FD_SET(pComponentPrivate->free_inpBuf_Q[VIDDEC_PIPE_READ], &rfds);
        FD_SET(pComponentPrivate->free_outBuf_Q[VIDDEC_PIPE_READ], &rfds);
        FD_SET(pComponentPrivate->filled_inpBuf_Q[VIDDEC_PIPE_READ], &rfds);

#ifdef UNDER_CE
        tv.tv_sec = 0;
        tv.tv_usec = VIDD_TIMEOUT * 30; 
#else
        tv.tv_sec = 0;
        tv.tv_nsec = 30000;
#endif


#ifdef UNDER_CE
        status = select (fdmax+1, &rfds, NULL, NULL, NULL);
#else
        sigemptyset (&set);
        sigaddset (&set, SIGALRM);
        status = pselect (fdmax+1, &rfds, NULL, NULL, NULL, &set);
        sigdelset (&set, SIGALRM);
#endif
        
        if (0 == status) {
            ;
        } 
        else if (-1 == status) {
            VIDDECODER_EPRINT ("%d :: Error in Select\n", __LINE__);
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorInsufficientResources, 
                                                   OMX_TI_ErrorSevere,
                                                   "Error from Component Thread in select");
            exit(1);
        }
        else {
            if (FD_ISSET(pComponentPrivate->cmdPipe[VIDDEC_PIPE_READ], &rfds)) {
#ifndef UNDER_CE
            if(!bFlag) {
            
                bFlag = OMX_TRUE;
#endif
                read(pComponentPrivate->cmdPipe[VIDDEC_PIPE_READ], &eCmd, sizeof(eCmd));
                read(pComponentPrivate->cmdDataPipe[VIDDEC_PIPE_READ], &nParam1, sizeof(nParam1));
                
#ifdef __PERF_INSTRUMENTATION__
                PERF_ReceivedCommand(pComponentPrivate->pPERFcomp,
                                     eCmd, nParam1, PERF_ModuleLLMM);
#endif
                if (eCmd == OMX_CommandStateSet) {
                    if ((OMX_S32)nParam1 < -2) {
                        VIDDECODER_EPRINT ("Incorrect variable value used\n");
                    }
                    if ((OMX_S32)nParam1 != -1 && (OMX_S32)nParam1 != -2) {
                        eError = VIDDEC_HandleCommand(pComponentPrivate, nParam1);
                        if (eError != OMX_ErrorNone) {
                            /*pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   OMX_EventError,
                                                                   eError, 
                                                                   0,
                                                                   "Error in HadleCommand function");*/
                        }
                    } 
                    else if ((OMX_S32)nParam1 == -1) {
                        break;
                    }
                    else if ((OMX_S32)nParam1 == -2) {
                        OMX_VidDec_Return(pComponentPrivate);
                        OMX_VidDec_Return(pComponentPrivate);
                        VIDDEC_Handle_InvalidState( pComponentPrivate);
                    }
                } 
                else if (eCmd == OMX_CommandPortDisable) {
                    eError = VIDDEC_DisablePort(pComponentPrivate, nParam1);
                    if (eError != OMX_ErrorNone) {
                        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                                               OMX_EventError,
                                                               eError, 
                                                               OMX_TI_ErrorSevere,
                                                               "Error in DisablePort function");
                    }
                }
                else if (eCmd == OMX_CommandPortEnable) {
                    eError = VIDDEC_EnablePort(pComponentPrivate, nParam1);
                    if (eError != OMX_ErrorNone) {
                        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                                               OMX_EventError,
                                                               eError, 
                                                               OMX_TI_ErrorSevere,
                                                               "Error in EnablePort function");
                    }
                } else if (eCmd == OMX_CommandFlush) {
                    VIDDEC_HandleCommandFlush (pComponentPrivate, nParam1, OMX_TRUE);
                }
                else if (eCmd == OMX_CommandMarkBuffer)    {
                    read(pComponentPrivate->cmdDataPipe[VIDDEC_PIPE_READ], &pCmdData, sizeof(pCmdData));
                    eError = VIDDEC_Queue_Add(&pComponentPrivate->qCmdMarkData, pCmdData, VIDDEC_QUEUE_OMX_MARKTYPE);
                    if(eError != OMX_ErrorNone)
                    {
                        VIDDECODER_EPRINT("Error in queue resources\n");
                    }
                }
#ifndef UNDER_CE
    bFlag = OMX_FALSE;
#endif

#ifndef UNDER_CE
            }
#endif
        }
            if(pComponentPrivate->bPipeCleaned){
                pComponentPrivate->bPipeCleaned =0;
            }
            else{
                if (FD_ISSET(pComponentPrivate->filled_outBuf_Q[VIDDEC_PIPE_READ], &rfds)) {
                    if(pComponentPrivate->bDynamicConfigurationInProgress){
                        VIDDEC_WAIT_CODE();
                        continue;
                    }
                    eError = VIDDEC_HandleDataBuf_FromDsp(pComponentPrivate);
                    if (eError != OMX_ErrorNone) {
                        VIDDECODER_EPRINT ("%d :: Error while handling filled DSP output buffer\n",__LINE__);
                        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                                               OMX_EventError,
                                                               eError,
                                                               OMX_TI_ErrorSevere,
                                                               "Error from Component Thread while processing dsp Responses");
                    }
                    pComponentPrivate->nOutputBCountDsp--;
                }
                if ((FD_ISSET(pComponentPrivate->filled_inpBuf_Q[VIDDEC_PIPE_READ], &rfds))){
                    VIDDEC_BUFFERPRINT("eExecuteToIdle 0x%x\n",pComponentPrivate->eExecuteToIdle);
                    /* When doing a reconfiguration, don't send input buffers to SN & wait for SN to be ready*/
                    if(pComponentPrivate->bDynamicConfigurationInProgress == OMX_TRUE || 
                            pComponentPrivate->eLCMLState != VidDec_LCML_State_Start){
                        VIDDEC_WAIT_CODE();
                         continue;
                    }
                    else{
                   eError = VIDDEC_HandleDataBuf_FromApp (pComponentPrivate);     
                    if (eError != OMX_ErrorNone) {
                        VIDDECODER_EPRINT ("%d :: Error while handling filled input buffer\n",__LINE__);
                        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                                               OMX_EventError,
                                                               eError, 
                                                               OMX_TI_ErrorSevere,
                                                               "Error from Component Thread while processing input buffer");
                    }
                    pComponentPrivate->nInputBCountApp--;
                }
                }
                if (FD_ISSET(pComponentPrivate->free_inpBuf_Q[VIDDEC_PIPE_READ], &rfds)) {
                    if(pComponentPrivate->bDynamicConfigurationInProgress){
                        VIDDEC_WAIT_CODE();
                        continue;
                    }
                    eError = VIDDEC_HandleFreeDataBuf(pComponentPrivate);
                    if (eError != OMX_ErrorNone) {
                        VIDDECODER_EPRINT ("%d :: Error while processing free input buffers\n",__LINE__);
                        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                                               OMX_EventError,
                                                               eError,  
                                                               OMX_TI_ErrorSevere,
                                                               "Error from Component Thread while processing free input buffer");
                    }
                    pComponentPrivate->nInputBCountDsp--;
                }
                if (FD_ISSET(pComponentPrivate->free_outBuf_Q[VIDDEC_PIPE_READ], &rfds)) {
                     if(pComponentPrivate->bDynamicConfigurationInProgress){
                        VIDDEC_WAIT_CODE();
                          continue;
                     }
                    VIDDEC_BUFFERPRINT("eExecuteToIdle 0x%x\n",pComponentPrivate->eExecuteToIdle);
                    eError = VIDDEC_HandleFreeOutputBufferFromApp(pComponentPrivate);
                    if (eError != OMX_ErrorNone) {
                        VIDDECODER_EPRINT ("%d :: Error while processing free output buffer\n",__LINE__);
                        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                                               OMX_EventError,
                                                               eError, 
                                                               OMX_TI_ErrorSevere,
                                                               "Error from Component Thread while processing free output buffer");
                    }
                    pComponentPrivate->nOutputBCountApp--;
                }
            }
        }
    }

#ifdef __PERF_INSTRUMENTATION__
    PERF_Done(pComponentPrivate->pPERFcomp);
#endif

    return (void*)OMX_ErrorNone;
}

void* OMX_VidDec_Return (void* pThreadData)
{
    int status = 0;
    struct timeval tv1;
#ifdef UNDER_CE
    struct timeval tv;
#else
    sigset_t set;
    struct timespec tv;
#endif
    int fdmax = 0;
    OMX_U32 iLock = 0;
    fd_set rfds;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    VIDDEC_COMPONENT_PRIVATE* pComponentPrivate = NULL;

    pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)pThreadData;
    gettimeofday(&tv1, NULL);
    /**Looking for highest number of file descriptor for pipes in order to put in select loop */
    fdmax = pComponentPrivate->free_inpBuf_Q[VIDDEC_PIPE_READ];

    if (pComponentPrivate->free_outBuf_Q[VIDDEC_PIPE_READ] > fdmax) {
        fdmax = pComponentPrivate->free_outBuf_Q[VIDDEC_PIPE_READ];
    }

    if (pComponentPrivate->filled_inpBuf_Q[VIDDEC_PIPE_READ] > fdmax) {
        fdmax = pComponentPrivate->filled_inpBuf_Q[VIDDEC_PIPE_READ];
    }

    if (pComponentPrivate->filled_outBuf_Q[VIDDEC_PIPE_READ] > fdmax) {
        fdmax = pComponentPrivate->filled_outBuf_Q[VIDDEC_PIPE_READ];
    }
    while ((pComponentPrivate->nInputBCountApp != 0 && 
                (pComponentPrivate->eLCMLState == VidDec_LCML_State_Start && pComponentPrivate->bDynamicConfigurationInProgress == OMX_FALSE)) || 
            pComponentPrivate->nOutputBCountApp != 0 ||
            pComponentPrivate->nInputBCountDsp != 0 || pComponentPrivate->nOutputBCountDsp != 0) {
        FD_ZERO (&rfds);
        FD_SET(pComponentPrivate->filled_outBuf_Q[VIDDEC_PIPE_READ], &rfds);
        FD_SET(pComponentPrivate->free_inpBuf_Q[VIDDEC_PIPE_READ], &rfds);
        FD_SET(pComponentPrivate->free_outBuf_Q[VIDDEC_PIPE_READ], &rfds);
        FD_SET(pComponentPrivate->filled_inpBuf_Q[VIDDEC_PIPE_READ], &rfds);

#ifdef UNDER_CE
        tv.tv_sec = 0;
        tv.tv_usec = VIDD_TIMEOUT * 30; 
#else
    tv.tv_sec = 0;
    tv.tv_nsec = 10000;
#endif

        
#ifdef UNDER_CE
        status = select (fdmax+1, &rfds, NULL, NULL, &tv);
#else
        sigemptyset (&set);
        sigaddset (&set, SIGALRM);
        status = pselect (fdmax+1, &rfds, NULL, NULL, &tv, &set);
        sigdelset (&set, SIGALRM);
#endif
        if (0 == status) {
            iLock++;
            if (iLock > 2){
                pComponentPrivate->bPipeCleaned = 1;
                break;
            }
        } 
        else if (-1 == status) {
            VIDDECODER_EPRINT ("%d :: Error in Select\n", __LINE__);
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorInsufficientResources, 
                                                   OMX_TI_ErrorSevere,
                                                   "Error from Component Thread in select");
            exit(1);
        } 
        else {
            if (FD_ISSET(pComponentPrivate->filled_outBuf_Q[VIDDEC_PIPE_READ], &rfds)) {
                eError = VIDDEC_HandleDataBuf_FromDsp(pComponentPrivate);
                if (eError != OMX_ErrorNone) {
                    VIDDECODER_EPRINT ("%d :: Error while handling filled DSP output buffer\n",__LINE__);
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           eError, 
                                                           OMX_TI_ErrorSevere,
                                                           "Error from Component Thread while processing dsp Responses");
                }
                pComponentPrivate->nOutputBCountDsp--;
            }
            if ((FD_ISSET(pComponentPrivate->filled_inpBuf_Q[VIDDEC_PIPE_READ], &rfds))){
                VIDDEC_BUFFERPRINT("eExecuteToIdle 0x%x\n",pComponentPrivate->eExecuteToIdle);
                if(!(pComponentPrivate->bDynamicConfigurationInProgress == OMX_TRUE && pComponentPrivate->bInPortSettingsChanged == OMX_FALSE)){
                eError = VIDDEC_HandleDataBuf_FromApp (pComponentPrivate);     
                if (eError != OMX_ErrorNone) {
                    VIDDECODER_EPRINT ("%d :: Error while handling filled input buffer\n",__LINE__);
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           eError, 
                                                           OMX_TI_ErrorSevere,
                                                           "Error from Component Thread while processing input buffer");
                }
                pComponentPrivate->nInputBCountApp--;
                }
            }
            if (FD_ISSET(pComponentPrivate->free_inpBuf_Q[VIDDEC_PIPE_READ], &rfds)) {
                eError = VIDDEC_HandleFreeDataBuf(pComponentPrivate);
                if (eError != OMX_ErrorNone) {
                    VIDDECODER_EPRINT ("%d :: Error while processing free input buffers\n",__LINE__);
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           eError,  
                                                           OMX_TI_ErrorSevere,
                                                           "Error from Component Thread while processing free input buffer");
                }
                pComponentPrivate->nInputBCountDsp--;
            }
            if (FD_ISSET(pComponentPrivate->free_outBuf_Q[VIDDEC_PIPE_READ], &rfds)) {
                VIDDEC_BUFFERPRINT("eExecuteToIdle 0x%x\n",pComponentPrivate->eExecuteToIdle);
                eError = VIDDEC_HandleFreeOutputBufferFromApp(pComponentPrivate);
                if (eError != OMX_ErrorNone) {
                    VIDDECODER_EPRINT ("%d :: Error while processing free output buffer\n",__LINE__);
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           eError, 
                                                           OMX_TI_ErrorSevere,
                                                           "Error from Component Thread while processing free output buffer");
                }
                pComponentPrivate->nOutputBCountApp--;
            }
        }
    }
    
    pComponentPrivate->bPipeCleaned = OMX_TRUE;
    return (void*)OMX_ErrorNone;
}

