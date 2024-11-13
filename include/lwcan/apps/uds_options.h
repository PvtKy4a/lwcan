#ifndef UDS_OPTIONS_H
#define UDS_OPTIONS_H

#ifdef __cplusplus
extern "C" {
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
 */
#if !defined UDS_KEEP_SESSION
#define UDS_KEEP_SESSION            0
#endif

/*
 *  Sub-function for keep session (to suppress or not the response to a tester present request)
 */
#if UDS_KEEP_SESSION && !defined UDS_KEEP_SESSION_SUB
#define UDS_KEEP_SESSION_SUB        0x80
#endif

#ifdef __cplusplus
}
#endif

#endif
