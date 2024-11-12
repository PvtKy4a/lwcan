#ifndef LWCAN_OPTIONS_H
#define LWCAN_OPTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwcan_options.h"

/*
 *  The amount of memory available for sending and receiving
 */
#if !defined LWCAN_MEM_SIZE
#define LWCAN_MEM_SIZE              512
#endif

/**
 * LWCAN_ISOTP == 1: Turn on ISOTP.
 */
#if !defined LWCAN_ISOTP
#define LWCAN_ISOTP                 1
#endif

/*
 *  Enable CAN FD support for ISOTP
 */
#if !defined ISOTP_CANFD
#define ISOTP_CANFD                 0
#endif

/*
 *  Enable CAN FD bit rate switch (second bitrate for payload data) for ISOTP
 */
#if !defined ISOTP_CANFD_BRS
#define ISOTP_CANFD_BRS             1
#endif

/*
 *  A byte used to pad messages to prevent bit stuffing
 */
#if !defined ISOTP_PADDING_BYTE
#define ISOTP_PADDING_BYTE          0xAA
#endif

/*
 *  The single-byte Block Size to include in the flow control message that the layer will send when receiving data. 
 *  Represents the number of consecutive frames that a sender should send before expecting the layer to send a flow control message.
 *  0 means infinitely large block size (implying no flow control message)
 */
#if !defined ISOTP_RECEIVE_BS
#define ISOTP_RECEIVE_BS            0
#endif

/*
 *  The minimum delay between frames. It is used as a way for the receiver to guide the sender not to overburden it. 
 *  A value between 0 .. 127 (0x7F), represents a minimum delay in milliseconds. 
 *  While values in the range 241 (0xF1) to 249 (0xF9) specify delays increasing from 100 to 900 microseconds.
 */
#if !defined ISOTP_RECEIVE_ST
#define ISOTP_RECEIVE_ST            0
#endif

/*
 *  Maximum limit to the number of FC WAIT a receiver is allowed to sent
 */
#if !defined ISOTP_N_WFT
#define ISOTP_N_WFT                 0
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
