/*
 ============================================================================
 Name        : OMXsonien.h
 Author      : SonienTaegi ( https://github.com/SonienTaegi/rpi-omx-tutorial )
 Version     :
 Copyright   : GPLv2
 Description : OMXsonien is helper for OMX and make developer much easier to
               handle buffer and error cases.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <bcm_host.h>

#include <IL/OMX_Core.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Video.h>
#include <IL/OMX_Broadcom.h>

#include "common.h"

typedef enum OMXsonien_BUFFERASSIGNTYPE {
	AllocateBuffer	= 0x00,
	UseBuffer
} OMXsonien_BUFFERASSIGNTYPE;

typedef struct OMXsonien_BUFFERMANAGER {
	OMX_HANDLETYPE 				hComponent;
	OMX_U32						nPortIndex;
	OMXsonien_BUFFERASSIGNTYPE 	eBufferSetType;
	OMX_BUFFERHEADERTYPE**		pBufferPtrHead;
	OMX_BUFFERHEADERTYPE**		pBufferPtrTail;
	OMX_BUFFERHEADERTYPE**		pBufferPtrNow;
	OMX_BUFFERHEADERTYPE**		pBufferPtrPool;
	unsigned int				nBufferRemain;
	pthread_mutex_t 			mutex;
} OMXsonien_BUFFERMANAGER;

/**
 * OMXsonien Helper 를 초기화 한다.
 */
void OMXsonienInit();

void OMXsonienDeinit();

void OMXsonienSetErrorCallback(void (*callback)(OMX_ERRORTYPE));

OMXsonien_BUFFERMANAGER* OMXsonienAllocateBuffer(
		OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
		OMX_IN OMX_U32 nSize,
        OMX_IN OMX_U32 nCount);

void OMXsonienFreeBuffer(
		OMX_IN OMXsonien_BUFFERMANAGER* pManager);

OMX_BUFFERHEADERTYPE* OMXsonienBufferGet(
		OMX_IN OMXsonien_BUFFERMANAGER* pManager);

void OMXsonienBufferPut(
		OMX_IN OMXsonien_BUFFERMANAGER* pManager,
		OMX_BUFFERHEADERTYPE* pBuffer);

OMX_BUFFERHEADERTYPE* OMXsonienBufferNow(
		OMX_IN OMXsonien_BUFFERMANAGER* pManager);


