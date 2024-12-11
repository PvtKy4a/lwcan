#ifndef LWCAN_LENGTH_H
#define LWCAN_LENGTH_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/* get data length from raw data length code (DLC) */
uint8_t can_fd_dlc2len(uint8_t dlc);

/* map the sanitized data length to an appropriate data length code */
uint8_t can_fd_len2dlc(uint8_t length);

#ifdef __cplusplus
}
#endif

#endif
