/*
 ============================================================================
 Name        : load_component.c
 Author      : SonienTaegi
 Version     :
 Copyright   : Any free opened source codes are for tutorial purpose. User can use them freely.
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <bcm_host.h>
#include "common.h"

#include <IL/OMX_Core.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Video.h>
#include <IL/OMX_Broadcom.h>

/* Application variant */
typedef struct {
	OMX_HANDLETYPE	pCamera;
	OMX_BOOL		isCameraReady;

	unsigned int	nWidth;
	unsigned int	nHeight;
	unsigned int	nFramerate;

	// Buffer Header pointer. OMX Component will allocate this..
	OMX_BUFFERHEADERTYPE*	pBufferHeader;
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

int main(void) {
	/* Temporary variables */
	OMX_ERRORTYPE	err;
	OMX_PARAM_PORTDEFINITIONTYPE	portDef;

	/* Initialize application variables */
	memset(&mContext, 0, (size_t)sizeof(mContext));
	mContext.nWidth 	= 1280;
	mContext.nHeight 	= 960;
	mContext.nFramerate	= 1;

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
	callbackOMX.EmptyBufferDone	= NULL;
	callbackOMX.FillBufferDone	= NULL;

	// Loading component
	char componentCamera[1024] = "OMX.broadcom.camera";
	print_log("Load %s", componentCamera);
	if((err = OMX_GetHandle(&mContext.pCamera, componentCamera, &mContext, &callbackOMX)) != OMX_ErrorNone ) {
		print_omx_error(err, "FAIL");
		OMX_Deinit();
		exit(-1);
	}
	print_log("Handler address : 0x%08x", mContext.pCamera);

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
		OMX_FreeHandle(mContext.pCamera);
		OMX_Deinit();
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
		OMX_FreeHandle(mContext.pCamera);
		OMX_Deinit();
		exit(-1);
	}

	// Set video format of #71 port.
	print_log("Set video format of the camera : Using #71.");
	OMX_INIT_STRUCTURE(portDef);
	portDef.nPortIndex = 71;

	print_log("Get non-initialized definition of #71");
	OMX_GetParameter(mContext.pCamera, OMX_IndexParamPortDefinition, &portDef);

	print_log("Set up parameters of video format.");
	OMX_VIDEO_PORTDEFINITIONTYPE* formatVideo = &portDef.format.video;
	formatVideo->eColorFormat 	= OMX_COLOR_FormatYUV420PackedPlanar;
	formatVideo->nFrameWidth	= mContext.nWidth;
	formatVideo->nFrameHeight	= mContext.nHeight;
	formatVideo->xFramerate		= mContext.nFramerate << 16;	//
	formatVideo->nStride		= formatVideo->nFrameWidth;		// Stride 0 -> Raise segment fault.
	if((err = OMX_SetParameter(mContext.pCamera, OMX_IndexParamPortDefinition, &portDef)) != OMX_ErrorNone) {
		print_omx_error(err, "FAIL");
		OMX_FreeHandle(mContext.pCamera);
		OMX_Deinit();
		exit(-1);
	}

	// Wait up for camera being ready.
	while(!mContext.isCameraReady) {
		print_log("Waiting until camera device is ready.");
		usleep(200 * 1000);
	}
	print_log("Camera is ready.");

	// Request state of component to be IDLE.
	// The command will turn the component into waiting mode.
	// After allocating buffer to all enabled ports than the component will be IDLE.
	print_log("STATE : IDLE request");
	if((err = OMX_SendCommand(mContext.pCamera, OMX_CommandStateSet, OMX_StateIdle, NULL)) != OMX_ErrorNone) {
		print_omx_error(err, "FAIL");
		OMX_FreeHandle(mContext.pCamera);
		OMX_Deinit();
		exit(-1);
	}

	// Allocate buffer to enabled buffer.
	print_log("Allocate buffer to camera #71 for output.");
	OMX_INIT_STRUCTURE(portDef);
	portDef.nPortIndex = 71;
	OMX_GetParameter(mContext.pCamera, OMX_IndexParamPortDefinition, &portDef);
	if((err = OMX_AllocateBuffer(mContext.pCamera, &mContext.pBufferHeader, 71, &mContext, portDef.nBufferSize)) != OMX_ErrorNone) {
		print_omx_error(err, "FAIL");
		OMX_FreeHandle(mContext.pCamera);
		OMX_Deinit();
		exit(-1);
	}
	print_log("Buffer is allocated : %d / %d bytes @0x%08x",
			mContext.pBufferHeader->nAllocLen,
			portDef.nBufferSize,
			mContext.pBufferHeader);

	// Wait up for component being idle.
	while(1) {
		OMX_STATETYPE cameraState;
		OMX_GetState(mContext.pCamera, &cameraState);
		if(cameraState == OMX_StateIdle) break;

		print_log("Waiting until state of camera is idle.");
		usleep(200 * 1000);
	}
	print_log("STATE : IDLE OK!");

	usleep(1000 * 1000);

	// Request state of component to be LOADED.
	// The command will turn the component into waiting mode.
	// After free all buffer from all enabled ports than the component will be LOADED.
	print_log("STATE : LOADED request");
	OMX_SendCommand(mContext.pCamera, OMX_CommandStateSet, OMX_StateLoaded, NULL);

	// Free buffer from all enabled ports.
	print_log("Free buffer");
	if((err = OMX_FreeBuffer(mContext.pCamera, 71, mContext.pBufferHeader)) != OMX_ErrorNone ) {
		print_omx_error(err, "FAIL");
	}
	mContext.pBufferHeader = NULL;

	// Wait up for component being loaded.
	while(1) {
		OMX_STATETYPE cameraState;
		OMX_GetState(mContext.pCamera, &cameraState);
		if(cameraState == OMX_StateLoaded) break;

		print_log("Waiting until state of camera is loaded.");
		usleep(200 * 1000);
	}
	print_log("STATE : LOADED OK!");

	// Unload camera
	OMX_FreeHandle(mContext.pCamera);

	OMX_Deinit();

	print_log("Press enter to terminate.");
	getchar();
}
