#ifndef UDS_CLIENT_H
#define UDS_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwcan/apps/uds_common.h"
#include "lwcan/apps/uds_options.h"
#include "lwcan/can.h"

#include <stdint.h>

lwcanerr_t uds_client_init(void);

void uds_client_cleanup(void);

lwcanerr_t uds_set_context(const struct uds_context *context, void *handle);

lwcanerr_t uds_start_diagnostic_session(const struct addr_can *addr, uint8_t session_type);

void uds_close_diagnostic_session(void);

uint8_t uds_get_active_session(void);

lwcanerr_t uds_send_request(const uint8_t *request, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
