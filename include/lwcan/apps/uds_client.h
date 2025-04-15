#ifndef UDS_CLIENT_H
#define UDS_CLIENT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lwcan/apps/uds_common.h"
#include "lwcan/apps/uds_options.h"
#include "lwcan/can.h"

#include <stdint.h>

lwcanerr_t uds_client_init(void);

lwcanerr_t uds_client_deinit(void);

lwcanerr_t uds_client_connect(const struct addr_can *addr);

lwcanerr_t uds_client_disconnect(void);

lwcanerr_t uds_set_context(const struct uds_context *context, void *handle);

lwcanerr_t uds_set_p2_timer(uint32_t time);

lwcanerr_t uds_set_p2_star_timer(uint32_t time);

lwcanerr_t uds_send_request(const void *request, uint32_t size, uint8_t with_response);

#ifdef __cplusplus
}
#endif

#endif
