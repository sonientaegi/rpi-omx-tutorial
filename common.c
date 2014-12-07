/*
 ============================================================================
 Name        : common.c
 Author      : SonienTaegi ( https://github.com/SonienTaegi/rpi-omx-tutorial )
 Version     :
 Copyright   : GPLv2
 Description : This is common sources for rpi-omx-tutorial.
 ============================================================================
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

void print_log(const char* message, ...) {
	char str[1024] = "";
	va_list	args;
	va_start(args, message);
	vsnprintf(str, sizeof(str), message, args);
	va_end(args);

#ifdef CURSES
#include <curses.h>
#pragma message("Compile CURSES mode\n")
	printw("OMX > %s\n", str);
	refresh();
#else
#pragma message("Compile stdout mode\n")
	printf("OMX > %s\n", str);
#endif
}

void print_omx_error(OMX_ERRORTYPE err, const char* message, ...) {
	va_list args;
    char str[1024] = "";
    char* e;
    va_start(args, message);
    vsnprintf(str, sizeof(str), message, args);
    va_end(args);

    switch(err) {
	case OMX_ErrorNone:
		e = "OK";
		break;
	case OMX_ErrorBadParameter:
		e = "BadParameter";
		break;
	case OMX_ErrorIncorrectStateOperation:
		e = "IncorrectStateOperation";
		break;
	case OMX_ErrorIncorrectStateTransition:
		e = "IncorrectStateTransition";
		break;
	case OMX_ErrorInsufficientResources:
		e = "InsufficientResources";
		break;
	case OMX_ErrorBadPortIndex:
		e = "BadPortIndex";
		break;
	case OMX_ErrorHardware:
		e = "Hardware";
		break;
	default:
		e = "Others";
    }

    print_log("ERROR [0x%08x] : %s (%s)", err, str, e);
}

void print_event(OMX_HANDLETYPE hComponent, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2) {
char *e;
    switch(eEvent) {
	case OMX_EventCmdComplete:
		e = "CmdComplete";
		break;
	case OMX_EventError:
		e = "Error";
		break;
	case OMX_EventMark:
		e = "Mark";
		break;
	case OMX_EventPortSettingsChanged:
		e = "PortSettingsChanged";
		break;
	case OMX_EventBufferFlag:
		e = "BufferFlag";
		break;
	case OMX_EventResourcesAcquired:
		e = "ResourcesAcquired";
		break;
	case OMX_EventComponentResumed:
		e = "ComponentResumed";
		break;
	case OMX_EventDynamicResourcesAvailable:
		e = "DynamicResourcesAvailable";
		break;
	case OMX_EventParamOrConfigChanged:
		e = "ParamOrConfigChanged";
		break;
	default:
		e = "Others";
    }
    print_log("EVENT [0x%08x] @0x%08x : %s (nData1=0x%08x, nData2=0x%08x)", eEvent, hComponent, e, nData1, nData2);
}

OMX_BOOL isState(OMX_HANDLETYPE* hComponent, OMX_STATETYPE state) {
	if(hComponent == NULL)	return OMX_FALSE;

	OMX_STATETYPE currentState;
	OMX_GetState(hComponent, &currentState);
	return currentState == state;
}

/*
 * Wait for state change of handles in array.
 * IMPORTANT : Last element should be NULL.
 */
OMX_BOOL block_until_state_change(OMX_STATETYPE state_tobe, OMX_HANDLETYPE* ppHandler) {
	if(!ppHandler)	return OMX_TRUE;

	OMX_STATETYPE	state_current;
	OMX_BOOL		isValid	= OMX_TRUE;

	OMX_HANDLETYPE pHandler	= NULL;
	while((pHandler = *ppHandler++)) {
		int timeout_counter = 0;
		isValid = OMX_FALSE;

		while(timeout_counter < 2000) {
			OMX_GetState(pHandler, &state_current);
			if((isValid = (state_current == state_tobe)))	break;

			usleep(100);
			timeout_counter++;
		}

		if(!isValid) break;
	}

	return isValid;
}

/*
 * Wait for state change of variable number of handles.
 * IMPORTANT : Last element should be NULL.
 */
OMX_BOOL wait_for_state_change(OMX_STATETYPE state_tobe, ...) {
	va_list ap;
	va_start(ap, state_tobe);

	OMX_STATETYPE	state_current;
	OMX_HANDLETYPE	pHandler = NULL;
	OMX_BOOL		isValid	= OMX_TRUE;
	while((pHandler = va_arg(ap, OMX_HANDLETYPE))) {
		print_log("Waiting for 0x%08x", pHandler);
		int timeout_counter = 0;
		isValid = OMX_FALSE;

		while(timeout_counter < 5000) {
			OMX_GetState(pHandler, &state_current);
			if((isValid = (state_current == state_tobe)))	break;

			usleep(100);
			timeout_counter++;
		}

		if(!isValid) break;
	}
	va_end(ap);

	return isValid;
}
