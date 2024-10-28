#ifndef UDS_OPTIONS_H
#define UDS_OPTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwcan/options.h"

/*
 *  Server minimum response time for a request in milliseconds
 */
#if !defined UDS_P2_SERVER
#define UDS_P2_SERVER               50
#endif

/*
 *  Maximum response time for the server in milliseconds, after which it must give a positive or negative response
 */
#if !defined UDS_P2_STAR_SERVER    
#define UDS_P2_STAR_SERVER          500
#endif

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
 *  This option allows the client to hold the current diagnostic session until the S3 client timeout expires.
 */
#if !defined UDS_CLIENT_KEEP_SESSION    
#define UDS_CLIENT_KEEP_SESSION     1
#endif

#ifdef __cplusplus
}
#endif

#endif
