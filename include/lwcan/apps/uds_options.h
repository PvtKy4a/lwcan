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
 *  Default timeout for P2 timer in milliseconds
 */
#if !defined UDS_P2_DEFAULT
#define UDS_P2_DEFAULT              50
#endif

/*
 *  Default timeout for P2* timer in milliseconds
 */
#if !defined UDS_P2_STAR_DEFAULT
#define UDS_P2_STAR_DEFAULT         5000
#endif

/*
 *  Number of attempts to connect the client to the server
 */
#if !defined UDS_CONNECT_RETRY_NUM
#define UDS_CONNECT_RETRY_NUM       20
#endif

/*
 *  Timeout between connection attempts in milliseconds
 */
#if !defined UDS_CONNECT_RETRY_TIMEOUT
#define UDS_CONNECT_RETRY_TIMEOUT   50
#endif

/*
 *  Keep the current active session with the tester present service after the UDS_S3_CLIENT timeout
 *  Uses service 0x31 Tester present
 */
#if !defined UDS_KEEP_SESSION
#define UDS_KEEP_SESSION            1
#endif

/*
 *  Subfunction for tester present to suppress response
 */
#if !defined UDS_KEEP_SESSION_SUPPRESS && UDS_KEEP_SESSION
#define UDS_KEEP_SESSION_SUPPRESS   0x80
#endif

#ifdef __cplusplus
}
#endif

#endif
