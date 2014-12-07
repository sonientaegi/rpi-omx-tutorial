/*
 ============================================================================
 Name        : load_component.c
 Author      : SonienTaegi
 Version     :
 Copyright   : Free for non profit tutorials
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

#define	COMPONENT_CAMERA	"OMX.broadcom.camera"
#define COMPONENT_RENDER	"OMX.broadcom.video_render"

/* Application variant */
typedef struct {
	OMX_HANDLETYPE				pCamera;
	OMX_HANDLETYPE				pRender;
	OMX_BOOL					isCameraReady;

	unsigned int				nWidth;
	unsigned int				nHeight;
	unsigned int				nFramerate;

	OMX_BUFFERHEADERTYPE*		pBufferCameraOut;
	OMX_U8*						pSrcY;
	OMX_U8*						pSrcU;
	OMX_U8*						pSrcV;
	unsigned int				nSizeY, nSizeU, nSizeV;

	OMX_BUFFERHEADERTYPE**		pBufferPool;
	unsigned int				nBufferPoolSize;
	unsigned int				nBufferPoolIndex;

	OMX_BOOL					isFilled;
} CONTEXT;
CONTEXT mContext;

/* Event Handler : OMX Event */
OMX_ERRORTYPE onOMXevent (
		OMX_IN OMX_HANDLETYPE hComponent,
		OMX_IN OMX_PTR pAppData,
		OMX_IN OMX_EVENTTYPE eEvent,
		OMX_IN OMX_U32 nData1,
		OMX_IN OMX_U32 nData2,
		OMX_IN OMX_PTR pEventData) {

	print_event(hComponent, eEvent, nData1, nData2);

	switch(eEvent) {
	case OMX_EventParamOrConfigChanged :
		if(nData2 == OMX_IndexParamCameraDeviceNumber) {
			((CONTEXT*)pAppData)->isCameraReady = OMX_TRUE;
			print_log("Camera device is ready.");
		}
		break;
	default :
		break;
	}
	return OMX_ErrorNone;
}

/* Callback : Camera-out buffer is filled */
OMX_ERRORTYPE onFillCameraOut (
		OMX_OUT OMX_HANDLETYPE hComponent,
		OMX_OUT OMX_PTR pAppData,
		OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer) {
	mContext.isFilled = OMX_TRUE;
	return OMX_ErrorNone;
}

/* Callback : Render-in buffer is emptied */
OMX_ERRORTYPE onEmptyRenderIn(
		OMX_IN OMX_HANDLETYPE hComponent,
		OMX_IN OMX_PTR pAppData,
		OMX_IN OMX_BUFFERHEADERTYPE* pBuffer) {

	print_log("BUFFER 0x%08x emptied", pBuffer);
	return OMX_ErrorNone;
}

void terminate() {
	print_log("On terminating...");

	OMX_STATETYPE state;
	OMX_BOOL bWaitForCamera, bWaitForRender;

	// Execute -> Idle
	bWaitForCamera = bWaitForRender = OMX_FALSE;
	if(isState(mContext.pCamera, OMX_StateExecuting)) {
		OMX_SendCommand(mContext.pCamera, OMX_CommandStateSet, OMX_StateIdle, NULL);
		bWaitForCamera = OMX_TRUE;
	}
	if(isState(mContext.pRender, OMX_StateExecuting)) {
		OMX_SendCommand(mContext.pRender, OMX_CommandStateSet, OMX_StateIdle, NULL);
		bWaitForRender = OMX_TRUE;
	}
	if(bWaitForCamera) wait_for_state_change(OMX_StateIdle, mContext.pCamera, NULL);
	if(bWaitForRender) wait_for_state_change(OMX_StateIdle, mContext.pRender, NULL);

	// Idle -> Loaded
	bWaitForCamera = bWaitForRender = OMX_FALSE;
	if(isState(mContext.pCamera, OMX_StateIdle)) {
		OMX_SendCommand(mContext.pCamera, OMX_CommandStateSet, OMX_StateLoaded, NULL);
		bWaitForCamera = OMX_TRUE;
	}
	if(isState(mContext.pRender, OMX_StateIdle)) {
		OMX_SendCommand(mContext.pRender, OMX_CommandStateSet, OMX_StateLoaded, NULL);
		bWaitForRender = OMX_TRUE;
	}
	if(bWaitForCamera) wait_for_state_change(OMX_StateLoaded, mContext.pCamera, NULL);
	if(bWaitForRender) wait_for_state_change(OMX_StateLoaded, mContext.pRender, NULL);

	// Loaded -> Free
	if(isState(mContext.pCamera, OMX_StateLoaded)) OMX_FreeHandle(mContext.pCamera);
	if(isState(mContext.pRender, OMX_StateLoaded)) OMX_FreeHandle(mContext.pRender);

	OMX_Deinit();

	print_log("Press enter to terminate.");
	getchar();
}

void componentLoad(OMX_CALLBACKTYPE* pCallbackOMX) {
	OMX_ERRORTYPE err;

	// Loading component
	print_log("Load %s", COMPONENT_CAMERA);
	if((err = OMX_GetHandle(&mContext.pCamera, COMPONENT_CAMERA, &mContext, pCallbackOMX)) != OMX_ErrorNone ) {
		print_omx_error(err, "FAIL");
		terminate();
		exit(-1);
	}
	print_log("Handler address : 0x%08x", mContext.pCamera);

	print_log("Load %s", COMPONENT_RENDER);
	if((err = OMX_GetHandle(&mContext.pRender, COMPONENT_RENDER, &mContext, pCallbackOMX)) != OMX_ErrorNone ) {
		print_omx_error(err, "FAIL");
		terminate();
		exit(-1);
	}
	print_log("Handler address : 0x%08x", mContext.pRender);
}

void componentConfigure() {
	OMX_ERRORTYPE err;
	OMX_PARAM_PORTDEFINITIONTYPE portDef;
	OMX_VIDEO_PORTDEFINITIONTYPE* formatVideo;

	// Disable any unused ports
	OMX_SendCommand(mContext.pCamera, OMX_CommandPortDisable, 70, NULL);
	OMX_SendCommand(mContext.pCamera, OMX_CommandPortDisable, 72, NULL);
	OMX_SendCommand(mContext.pCamera, OMX_CommandPortDisable, 73, NULL);

	// Configure OMX_IndexParamCameraDeviceNumber callback enable to ensure whether camera is initialized properly.
	print_log("Configure DeviceNumber callback enable.");
	OMX_CONFIG_REQUESTCALLBACKTYPE configCameraCallback;
	OMX_INIT_STRUCTURE(configCameraCallback);
	configCameraCallback.nPortIndex	= OMX_ALL;	// Must Be OMX_ALL
	configCameraCallback.nIndex 	= OMX_IndexParamCameraDeviceNumber;
	configCameraCallback.bEnable 	= OMX_TRUE;
	if((err = OMX_SetConfig(mContext.pCamera, OMX_IndexConfigRequestCallback, &configCameraCallback)) != OMX_ErrorNone) {
		print_omx_error(err, "FAIL");
		terminate();
		exit(-1);
	}

	// OMX CameraDeviceNumber set -> will trigger Camera Ready callback
	print_log("Set CameraDeviceNumber parameter.");
	OMX_PARAM_U32TYPE deviceNumber;
	OMX_INIT_STRUCTURE(deviceNumber);
	deviceNumber.nPortIndex = OMX_ALL;
	deviceNumber.nU32 = 0;	// Mostly zero
	if((err = OMX_SetParameter(mContext.pCamera, OMX_IndexParamCameraDeviceNumber, &deviceNumber)) != OMX_ErrorNone) {
		print_omx_error(err, "FAIL");
		terminate();
		exit(-1);
	}

	// Set video format of #71 port.
	print_log("Set video format of the camera : Using #71.");
	OMX_INIT_STRUCTURE(portDef);
	portDef.nPortIndex = 71;

	print_log("Get non-initialized definition of #71.");
	OMX_GetParameter(mContext.pCamera, OMX_IndexParamPortDefinition, &portDef);

	print_log("Set up parameters of video format of #71.");
	formatVideo = &portDef.format.video;
	formatVideo->eColorFormat 	= OMX_COLOR_FormatYUV420PackedPlanar;
	formatVideo->nFrameWidth	= mContext.nWidth;
	formatVideo->nFrameHeight	= mContext.nHeight;
	formatVideo->xFramerate		= mContext.nFramerate << 16;	// Fixed point. 1
	formatVideo->nStride		= formatVideo->nFrameWidth;		// Stride 0 -> Raise segment fault.
	if((err = OMX_SetParameter(mContext.pCamera, OMX_IndexParamPortDefinition, &portDef)) != OMX_ErrorNone) {
		print_omx_error(err, "FAIL");
		terminate();
		exit(-1);
	}
	OMX_GetParameter(mContext.pCamera, OMX_IndexParamPortDefinition, &portDef);
	formatVideo = &portDef.format.video;
	mContext.nSizeY = formatVideo->nFrameWidth * formatVideo->nSliceHeight;
	mContext.nSizeU	= mContext.nSizeY / 4;
	mContext.nSizeV	= mContext.nSizeY / 4;
	print_log("%d %d %d", mContext.nSizeY, mContext.nSizeU, mContext.nSizeV);

	// Set video format of #90 port.
	print_log("Set video format of the render : Using #90.");
	OMX_INIT_STRUCTURE(portDef);
	portDef.nPortIndex = 90;

	print_log("Get default definition of #90.");
	OMX_GetParameter(mContext.pRender, OMX_IndexParamPortDefinition, &portDef);

	print_log("Set up parameters of video format of #90.");
	formatVideo = &portDef.format.video;
	formatVideo->eColorFormat 		= OMX_COLOR_FormatYUV420PackedPlanar;
	formatVideo->eCompressionFormat	= OMX_VIDEO_CodingUnused;
	formatVideo->nFrameWidth		= mContext.nWidth;
	formatVideo->nFrameHeight		= mContext.nHeight;
	formatVideo->nStride			= mContext.nWidth;
	formatVideo->nSliceHeight		= mContext.nHeight;
	formatVideo->xFramerate			= mContext.nFramerate << 16;
	if((err = OMX_SetParameter(mContext.pRender, OMX_IndexParamPortDefinition, &portDef)) != OMX_ErrorNone) {
		print_omx_error(err, "FAIL");
		terminate();
		exit(-1);
	}

	// Configure rendering region
	OMX_CONFIG_DISPLAYREGIONTYPE displayRegion;
	OMX_INIT_STRUCTURE(displayRegion);
	displayRegion.nPortIndex = 90;
	displayRegion.dest_rect.width 	= mContext.nWidth;
	displayRegion.dest_rect.height 	= mContext.nHeight;
	displayRegion.set = OMX_DISPLAY_SET_NUM | OMX_DISPLAY_SET_FULLSCREEN | OMX_DISPLAY_SET_MODE | OMX_DISPLAY_SET_DEST_RECT;
	displayRegion.mode = OMX_DISPLAY_MODE_FILL;
	displayRegion.fullscreen = OMX_FALSE;
	displayRegion.num = 0;
	if((err = OMX_SetConfig(mContext.pRender, OMX_IndexConfigDisplayRegion, &displayRegion)) != OMX_ErrorNone) {
		print_omx_error(err, "FAIL");
		terminate();
		exit(-1);
	}

	// Wait up for camera being ready.
	while(!mContext.isCameraReady) {
		print_log("Waiting until camera device is ready.");
		usleep(100 * 1000);
	}
	print_log("Camera is ready.");
}

void componentPrepare() {
	OMX_ERRORTYPE err;
	OMX_PARAM_PORTDEFINITIONTYPE portDef;

	// Request state of components to be IDLE.
	// The command will turn the component into waiting mode.
	// After allocating buffer to all enabled ports than the component will be IDLE.
	print_log("STATE : CAMERA - IDLE request");
	if((err = OMX_SendCommand(mContext.pCamera, OMX_CommandStateSet, OMX_StateIdle, NULL)) != OMX_ErrorNone) {
		print_omx_error(err, "FAIL");
		terminate();
		exit(-1);
	}

	print_log("STATE : RENDER - IDLE request");
	if((err = OMX_SendCommand(mContext.pRender, OMX_CommandStateSet, OMX_StateIdle, NULL)) != OMX_ErrorNone) {
		print_omx_error(err, "FAIL");
		terminate();
		exit(-1);
	}

	// Allocate buffers to render
	print_log("Allocate buffer to renderer #90 for input.");
	OMX_INIT_STRUCTURE(portDef);
	portDef.nPortIndex = 90;
	OMX_GetParameter(mContext.pRender, OMX_IndexParamPortDefinition, &portDef);
	print_log("Size of predefined buffer : %d * %d", portDef.nBufferSize, portDef.nBufferCountActual);

	mContext.nBufferPoolSize 	= portDef.nBufferCountActual;
	mContext.nBufferPoolIndex 	= 0;
	mContext.pBufferPool		= malloc(sizeof(OMX_BUFFERHEADERTYPE*) * mContext.nBufferPoolSize);
	for(int i = 0; i < mContext.nBufferPoolSize; i++) {
		mContext.pBufferPool[i] = 0x00;
		if((err = OMX_AllocateBuffer(mContext.pRender, &(mContext.pBufferPool[i]), 90, &mContext, portDef.nBufferSize)) != OMX_ErrorNone) {
			print_omx_error(err, "FAIL");
			terminate();
			exit(-1);
		}
	}

	// Allocate buffer to camera
	OMX_INIT_STRUCTURE(portDef);
	portDef.nPortIndex = 71;
	OMX_GetParameter(mContext.pCamera, OMX_IndexParamPortDefinition, &portDef);
	print_log("Size of predefined buffer : %d * %d", portDef.nBufferSize, portDef.nBufferCountActual);
	if((err = OMX_AllocateBuffer(mContext.pCamera, &mContext.pBufferCameraOut, 71, &mContext, portDef.nBufferSize)) != OMX_ErrorNone) {
		print_omx_error(err, "FAIL");
		terminate();
		exit(-1);
	}
	mContext.pSrcY 	= mContext.pBufferCameraOut->pBuffer;
	mContext.pSrcU	= mContext.pSrcY + mContext.nSizeY;
	mContext.pSrcV	= mContext.pSrcU + mContext.nSizeU;
	print_log("0x%08x 0x%08x 0x%08x", mContext.pSrcY, mContext.pSrcU, mContext.pSrcV);

	// Wait up for component being idle.
	if(!wait_for_state_change(OMX_StateIdle, mContext.pRender, mContext.pCamera, NULL)) {
		print_log("FAIL");
		terminate();
		exit(-1);
	}
	print_log("STATE : IDLE OK!");
}

int main(void) {
	/* Temporary variables */
	OMX_ERRORTYPE	err;
	OMX_PARAM_PORTDEFINITIONTYPE	portDef;

	/* Initialize application variables */
	memset(&mContext, 0, (size_t)sizeof(mContext));
	mContext.nWidth 	= 640;
	mContext.nHeight 	= 480;
	mContext.nFramerate	= 25;

	// RPI initialize.
	bcm_host_init();

	// OMX initialize.
	print_log("Initialize OMX");
	if((err = OMX_Init()) != OMX_ErrorNone) {
		print_omx_error(err, "FAIL");
		OMX_Deinit();
		exit(-1);
	}

	// For loading component, Callback shall provide.
	OMX_CALLBACKTYPE callbackOMX;
	callbackOMX.EventHandler	= onOMXevent;
	callbackOMX.EmptyBufferDone	= onEmptyRenderIn;
	callbackOMX.FillBufferDone	= onFillCameraOut;

	componentLoad(&callbackOMX);
	componentConfigure();
	componentPrepare();

	// Request state of component to be EXECUTE.
	print_log("STATE : CAMERA - EXECUTING request");
	if((err = OMX_SendCommand(mContext.pCamera, OMX_CommandStateSet, OMX_StateExecuting, NULL)) != OMX_ErrorNone) {
		print_omx_error(err, "FAIL");
		terminate();
		exit(-1);
	}

	print_log("STATE : RENDER - EXECUTING request");
	if((err = OMX_SendCommand(mContext.pRender, OMX_CommandStateSet, OMX_StateExecuting, NULL)) != OMX_ErrorNone) {
		print_omx_error(err, "FAIL");
		terminate();
		exit(-1);
	}

	if(!wait_for_state_change(OMX_StateExecuting, mContext.pCamera, mContext.pRender, NULL)) {
		print_log("FAIL");
		terminate();
		exit(-1);
	}
	print_log("STATE : EXECUTING OK!");

	// Since #71 is capturing port, needs capture signal like other handy capture devices
	print_log("Capture start.");
	OMX_CONFIG_PORTBOOLEANTYPE	portCapturing;
	OMX_INIT_STRUCTURE(portCapturing);
	portCapturing.nPortIndex = 71;
	portCapturing.bEnabled = OMX_TRUE;
	OMX_SetConfig(mContext.pCamera, OMX_IndexConfigPortCapturing, &portCapturing);


	OMX_U8*			pY = NULL;
	OMX_U8*			pU = NULL;
	OMX_U8*			pV = NULL;
	unsigned int	nOffsetU 	= mContext.nWidth * mContext.nHeight;
	unsigned int 	nOffsetV 	= nOffsetU * 5 / 4;
	unsigned int 	nFrameMax	= mContext.nFramerate * 5;
	unsigned int	nFrames		= 0;

	print_log("Capture for %d frames.", nFrameMax);
	OMX_FillThisBuffer(mContext.pCamera, mContext.pBufferCameraOut);
	while(nFrames < nFrameMax) {
		if(mContext.isFilled) {
			OMX_BUFFERHEADERTYPE* pBuffer = mContext.pBufferPool[mContext.nBufferPoolIndex];
			if(pBuffer->nFilledLen == 0) {
				pY = pBuffer->pBuffer;
				pU = pY + nOffsetU;
				pV = pY + nOffsetV;
			}

			memcpy(pY, mContext.pSrcY, mContext.nSizeY);	pY += mContext.nSizeY;
			memcpy(pU, mContext.pSrcU, mContext.nSizeU);	pU += mContext.nSizeU;
			memcpy(pV, mContext.pSrcV, mContext.nSizeV);	pV += mContext.nSizeV;
			pBuffer->nFilledLen += mContext.pBufferCameraOut->nFilledLen;

			if(mContext.pBufferCameraOut->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) {
				print_log("BUFFER 0x%08x filled", pBuffer);
				OMX_EmptyThisBuffer(mContext.pRender, pBuffer);
				mContext.nBufferPoolIndex++;
				if(mContext.nBufferPoolIndex == mContext.nBufferPoolSize) mContext.nBufferPoolIndex = 0;
				nFrames++;
			}
			mContext.isFilled = OMX_FALSE;
			OMX_FillThisBuffer(mContext.pCamera, mContext.pBufferCameraOut);
		}

		usleep(1);
	}

	portCapturing.bEnabled = OMX_FALSE;
	OMX_SetConfig(mContext.pCamera, OMX_IndexConfigPortCapturing, &portCapturing);
	print_log("Capture stop.");

	terminate();
}
