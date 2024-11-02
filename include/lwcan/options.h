#ifndef LWCAN_OPTIONS_H
#define LWCAN_OPTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwcan_options.h"

/*
 *  Whether or not an operating system is used
 */
#if !defined LWCAN_WITH_OS
#define LWCAN_WITH_OS               0
#endif

/*
 *  The amount of memory available for sending and receiving
 */
#if !defined LWCAN_MEM_SIZE
#define LWCAN_MEM_SIZE              512
#endif

/*
 *  The Data Length Code representing the number of bytes in the data field
 */
#if !defined LWCAN_DLC
#define LWCAN_DLC                   8
#endif

/**
 * LWCAN_ISOTP == 1: Turn on ISOTP.
 */
#if !defined LWCAN_ISOTP
#define LWCAN_ISOTP                 1
#endif

/*
 *  Represents the byte used for padding messages sent
 */
#if !defined ISOTP_PADDING_BYTE
#define ISOTP_PADDING_BYTE          0xAA
#endif

/*
 *  The single-byte Block Size to include in the flow control message that the layer will send when receiving data. 
 *  Represents the number of consecutive frames that a sender should send before expecting the layer to send a flow control message.
 *  0 means infinitely large block size (implying no flow control message)
 */
#if !defined ISOTP_RECEIVE_BLOCK_SIZE
#define ISOTP_RECEIVE_BLOCK_SIZE    0
#endif

/*
 *  The single-byte Separation Time to include in the flow control message that the layer will send when receiving data.
 *  Refer to ISO-15765-2 for specific values. From 1 to 127, represents milliseconds.
 *  From 0xF1 to 0xF9, represents hundreds of microseconds (100us, 200us, …, 900us).
 *  0 Means no timing requirements
 */
#if !defined ISOTP_SEND_SEPARATION_TIME
#define ISOTP_RECEIVE_SEPARATION_TIME  0
#endif

/*
 *  Time for transmission of the CAN frame on the sender side in milliseconds (see ISO 15765-2)
 */
#if !defined ISOTP_N_AS
#define ISOTP_N_AS                  1000
#endif

/*
 *  Time until reception of the next flow control frame in milliseconds (see ISO 15765-2)
 */
#if !defined ISOTP_N_BS
#define ISOTP_N_BS                  1000
#endif

/*
 *  Time until transmission of the next consecutive frame in milliseconds (see ISO 15765-2)
 */
#if !defined ISOTP_N_CS
#define ISOTP_N_CS                  5
#endif

/*
 *  Time until transmission of the next flow control frame in milliseconds (see ISO 15765-2)
 */
#if !defined ISOTP_N_BR
#define ISOTP_N_BR                  5
#endif

/*
 *  Time until reception of the next consecutive frame in milliseconds (see ISO 15765-2)
 */
#if !defined ISOTP_N_CR
#define ISOTP_N_CR                  1000
#endif

/*
 *  Allows or denies wait state in flow control frame
 */
#if !defined ISOTP_FC_WAIT_ALLOW
#define ISOTP_FC_WAIT_ALLOW         1
#endif

/*
 *  Number of simultaneously active ISOTP connections.
 */
#if !defined ISOTP_MAX_PCB_NUM
#define ISOTP_MAX_PCB_NUM           2
#endif

/**
 * LWCAN_RAW == 1: Turn on RAW.
 */
#if !defined LWCAN_RAW
#define LWCAN_RAW                   0
#endif

/*
 *  Number of simultaneously active RAW connections.
 */
#if !defined CANRAW_MAX_PCB_NUM
#define CANRAW_MAX_PCB_NUM          2
#endif

#ifdef __cplusplus
}
#endif

#endif
