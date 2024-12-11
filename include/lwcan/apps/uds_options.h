#ifndef UDS_OPTIONS_H
#define UDS_OPTIONS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lwcan/options.h"

/*
 *  Session timeout for server in milliseconds. After timeout return to default session
 */
#if !defined UDS_S3_SERVER
#define UDS_S3_SERVER               10000
#endif

/*
 *  Session timeout for client in milliseconds. After timeout return to default session
 */
#if !defined UDS_S3_CLIENT
#define UDS_S3_CLIENT               9000
#endif

/*
 *  Default timeout for P2 timer
 */
#if !defined UDS_P2_DEFAULT
#define UDS_P2_DEFAULT              50
#endif

/*
 *  Default timeout for P2* timer
 */
#if !defined UDS_P2_STAR_DEFAULT
#define UDS_P2_STAR_DEFAULT         500
#endif

/*
 *  Keep the current active session with the tester present service after the UDS_S3_CLIENT timeout
 *  Uses service 0x31 Tester present
 */
#if !defined UDS_KEEP_SESSION
#define UDS_KEEP_SESSION            0
#endif

/*
 *  Sub-function for UDS_KEEP_SESSION
 */
#if !defined UDS_KEEP_SESSION_SUB && UDS_KEEP_SESSION
#define UDS_KEEP_SESSION_SUB        0x80
#endif

/*
 *  Specify whether UDS_KEEP_SESSION_SUB suppresses the response to the request (1) or not (0)
 */
#if !defined UDS_SUPPRESS_KEEP_SESSION_RESPONSE && UDS_KEEP_SESSION
#define UDS_SUPPRESS_KEEP_SESSION_RESPONSE 1
#endif

#ifdef __cplusplus
}
#endif

#endif
