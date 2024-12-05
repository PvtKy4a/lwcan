#ifndef UDS_COMMON_H
#define UDS_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwcan/error.h"

#include <stdint.h>

#define UDS_POSITIVE_RESPOSNE_PADDING 0x40

/* UDS Services */
#define UDS_DIAGNOSTIC_SESSION_CONTROL_SID           0x10   
#define UDS_ECU_RESET_SID                            0x11
#define UDS_SECURITY_ACCESS_SID                      0x27
#define UDS_TESTER_PRESENT_SID                       0x3E
#define UDS_READ_DATA_BY_LOCAL_IDENTIFIER_SID        0x21
#define UDS_READ_DATA_BY_IDENTIFIER_SID              0x22
#define UDS_WRITE_DATA_BY_IDENTIFIER_SID             0x2E
#define UDS_ROUTINE_CONTROL_SID                      0x31
#define UDS_CLEAR_DIAGNOSTIC_INFORMATION_SID         0x14
#define UDS_READ_DTC_INFORMATION_SID                 0x19
#define UDS_INPUT_OUTPUT_CONTROL_BY_IDENTIFIER_SID   0x2F
#define UDS_NEGATIVE_RESPONSE_SID                    0x7F

/* UDS negative response codes */
#define UDS_SERVICE_NOT_SUPPORTED_NRC                0x11
#define UDS_SUB_FUNCTION_NOT_SUPPORTED_NRC           0x12
#define UDS_INVALID_MESSAGE_LENGTH_OR_FORMAT_NRC     0x13
#define UDS_BUSY_REPEAT_REQUEST_NRC                  0x21
#define UDS_CONDITIONS_NOT_CORRECT_NRC               0x22
#define UDS_REQUEST_SEQUENCE_ERROR_NRC               0x24
#define UDS_REQUEST_OUT_OF_RANGE_NRC                 0x31
#define UDS_SECURITY_ACCESS_DENIED_NRC               0x33
#define UDS_INVALID_KEY_NRC                          0x35
#define UDS_EXCEEDED_NUMBER_OF_ATTEMPTS_NRC          0x36
#define UDS_REQUIRED_TIME_DELAY_NOT_EXPIRED_NRC      0x37
#define UDS_PROGRAMMING_FAILURE_NRC                  0x72
#define UDS_REQUEST_RECEIVED_RESPONSE_PENDING_NRC    0x78

struct uds_context
{
    void (*request)(void *handle, uint8_t sid, uint8_t *data, uint32_t size); /* not implemented */

    void (*positive_response)(void *handle, uint8_t *data, uint32_t size);

    void (*negative_response)(void *handle, uint8_t rejected_sid, uint8_t nrc);

    void (*error)(void *handle, lwcanerr_t error);
};

#ifdef __cplusplus
}
#endif

#endif
