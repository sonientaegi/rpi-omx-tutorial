/*
 ============================================================================
 Name        : common.h
 Author      : SonienTaegi ( https://github.com/SonienTaegi/rpi-omx-tutorial )
 Version     :
 Copyright   : GPLv2
 Description : This is common headers for rpi-omx-tutorial.
 ============================================================================
 */
#ifndef RPI_OMX_TUTORIAL_SRC_COMMON_H_
#define RPI_OMX_TUTORIAL_SRC_COMMON_H_


#include <IL/OMX_Core.h>
#include <IL/OMX_Component.h>

#define OMX_INIT_STRUCTURE(a) \
    memset(&(a), 0, sizeof(a)); \
    (a).nSize = sizeof(a); \
    (a).nVersion.nVersion = OMX_VERSION; \
    (a).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
    (a).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
    (a).nVersion.s.nRevision = OMX_VERSION_REVISION; \
    (a).nVersion.s.nStep = OMX_VERSION_STEP


/*
 * Print log message to console.
 * It works properly whether CURSES mode or not.
 */
void print_log (const char* message, ...);

/*
 * Print log message with error description.
 */
void print_omx_error (OMX_ERRORTYPE err, const char* message, ...);

/*
 * Print event message with description and parameter values.
 */
void print_event (OMX_HANDLETYPE hComponent, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2);

/**
 * Block until state change of components. End of ppHandler must be NULL.
 * Maximum latency of each module is 20ms.
 */
OMX_BOOL block_until_state_change(OMX_STATETYPE state_tobe, OMX_HANDLETYPE* ppHandler);

/*
 * Wait for state change of components. Maximum latency of each module is 20ms.
 */
OMX_BOOL wait_for_state_change (OMX_STATETYPE state_tobe, ...);

/*
 * Check whether state of the component is it or not.
 */
OMX_BOOL isState(OMX_HANDLETYPE* hComponent, OMX_STATETYPE state);

#endif /* RPI_OMX_TUTORIAL_SRC_COMMON_H_ */
