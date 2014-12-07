/*
 ============================================================================
 Name        : OMXsonien.c
 Author      : SonienTaegi ( https://github.com/SonienTaegi/rpi-omx-tutorial )
 Version     :
 Copyright   : GPLv2
 Description : OMXsonien is helper for OMX and make developer much easier to
               handle buffer and error cases.
 ============================================================================
 */

#include "OMXsonien.h"

OMXsonien_BUFFERMANAGER* bufferManagerRefs[256];
void (*OMXsonienErrorCallback)(OMX_ERRORTYPE);

void OMXsonienErrorCallbackDefault(OMX_ERRORTYPE err) {
	printf("OMX > ERROR [0x%08x]\n", err);
}

void OMXsonienInit() {
	memset(bufferManagerRefs, 0x00, sizeof(OMXsonien_BUFFERMANAGER*) * 256);
	OMXsonienSetErrorCallback(OMXsonienErrorCallbackDefault);
}

void OMXsonienDeinit() {
	for(int i = 0; i < 256; i++) {
		if(bufferManagerRefs[i] != NULL) {
			pthread_mutex_destroy(&(bufferManagerRefs[i]->mutex));
			free(bufferManagerRefs[i]);
			bufferManagerRefs[i] = NULL;
		}
	}
}

void OMXsonienSetErrorCallback(void (*callback)(OMX_ERRORTYPE)) {
	OMXsonienErrorCallback = callback;
}

OMX_ERRORTYPE OMXsonienCheckError(OMX_ERRORTYPE err) {
	if(err != OMX_ErrorNone) {
		OMXsonienErrorCallback(err);
	}

	return err;
}

OMXsonien_BUFFERMANAGER* OMXsonienAllocateBuffer(
		OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
		OMX_IN OMX_U32 nSize,
        OMX_IN OMX_U32 nCount) {

	int refID	= -1;
	for(int i = 0; i < 256; i++) {
		if(bufferManagerRefs[i] != NULL) {
			continue;
		}
		else {
			refID = i;
			break;
		}
	}
	bufferManagerRefs[refID] = malloc(sizeof(OMXsonien_BUFFERMANAGER));
	OMXsonien_BUFFERMANAGER* pBufferManager = bufferManagerRefs[refID];
	pBufferManager->hComponent 		= hComponent;
	pBufferManager->nPortIndex		= nPortIndex;
	pBufferManager->eBufferSetType	= AllocateBuffer;	// Currently the only supported type

	if(!nSize || !nCount) {
		OMX_PARAM_PORTDEFINITIONTYPE 	portDef;
		OMX_INIT_STRUCTURE(portDef);

		portDef.nPortIndex = nPortIndex;
		OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &portDef);

		if(!nSize) {
			nSize = portDef.nBufferSize;
		}

		if(!nCount) {
			nCount = portDef.nBufferCountActual;
		}
	}
	printf("0x%08x : Buffer Size = %d / Count = %d\n", hComponent, nSize, nCount);
	pBufferManager->pBufferPtrPool = malloc(sizeof(OMX_BUFFERHEADERTYPE*) * nCount);
	for(int i = 0; i < nCount; i++) {
		OMX_BUFFERHEADERTYPE*	pBufferHeader;	// pBufferManager->pBufferPtrPool + i
		printf("0x%08x : New Buffer #%d\n", hComponent, i);
		OMXsonienCheckError(OMX_AllocateBuffer(hComponent, &pBufferHeader, nPortIndex, pAppPrivate, nSize));
		printf("0x%08x : At 0x%08x\n", hComponent, pBufferHeader->pBuffer);
		pBufferManager->pBufferPtrPool[i] = pBufferHeader;
	}

	pBufferManager->pBufferPtrHead 	= pBufferManager->pBufferPtrPool;
	pBufferManager->pBufferPtrTail 	= pBufferManager->pBufferPtrPool+(nCount - 1);
	pBufferManager->pBufferPtrNow	= pBufferManager->pBufferPtrHead;
	pBufferManager->nBufferRemain	= nCount;
	pthread_mutex_init(&pBufferManager->mutex, NULL);

	return pBufferManager;
}

void OMXsonienFreeBuffer(
		OMX_IN OMXsonien_BUFFERMANAGER* pManager) {
	OMX_BUFFERHEADERTYPE** pBufferPtr = pManager->pBufferPtrHead;

	while(pBufferPtr <= pManager->pBufferPtrTail) {
		OMX_FreeBuffer(pManager->hComponent, pManager->nPortIndex, *pBufferPtr);
		pBufferPtr++;
	}

	pManager->pBufferPtrHead 	= NULL;
	pManager->pBufferPtrTail 	= NULL;
	pManager->pBufferPtrNow 	= NULL;
	pManager->nBufferRemain		= 0;
	free(pManager->pBufferPtrPool);
	pManager->pBufferPtrPool 	= NULL;
}

OMX_BUFFERHEADERTYPE* OMXsonienBufferGet(
		OMX_IN OMXsonien_BUFFERMANAGER* pManager) {

	OMX_BUFFERHEADERTYPE* pNextBuffer = NULL;

	pthread_mutex_lock(&(pManager->mutex));
	if(pManager->nBufferRemain == 0) {
		return NULL;
	}

	pNextBuffer = *(pManager->pBufferPtrNow);
	pManager->pBufferPtrNow++;
	if(pManager->pBufferPtrNow > pManager->pBufferPtrTail) {
		pManager->pBufferPtrNow = pManager->pBufferPtrHead;
	}
	--(pManager->nBufferRemain);
	pthread_mutex_unlock(&(pManager->mutex));
	return pNextBuffer;
}

void OMXsonienBufferPut(
		OMX_IN OMXsonien_BUFFERMANAGER* pManager,
		OMX_BUFFERHEADERTYPE* pBuffer) {
	pthread_mutex_lock(&(pManager->mutex));
	++(pManager->nBufferRemain);
	pthread_mutex_unlock(&(pManager->mutex));
}

OMX_BUFFERHEADERTYPE* OMXsonienBufferNow(
		OMX_IN OMXsonien_BUFFERMANAGER* pManager) {
	return *(pManager->pBufferPtrNow);
}

