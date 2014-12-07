/*
 ============================================================================
 Name        : load_component.c
 Author      : SonienTaegi ( https://github.com/SonienTaegi/rpi-omx-tutorial )
 Version     :
 Copyright   : GPLv2
 Description : This is basic tutorial of OpenMAX to load component.
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
	return OMX_ErrorNone;
}

int main(void) {
	/* Temporary variables */
	OMX_ERRORTYPE	err;
	OMX_PARAM_PORTDEFINITIONTYPE	portDef;

	/* Initialize application variables */
	memset(&mContext, 0, (size_t)sizeof(mContext));
	mContext.nWidth 	= 640;
	mContext.nHeight 	= 480;
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

	usleep(1000 * 1000);

	// Unload camera
	OMX_FreeHandle(mContext.pCamera);

	OMX_Deinit();

	print_log("Press enter to terminate.");
	getchar();
}
