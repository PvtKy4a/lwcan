#ifndef UDS_CLIENT_H
#define UDS_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwcan/apps/uds_common.h"
#include "lwcan/apps/uds_options.h"
#include "lwcan/can.h"

#include <stdint.h>

lwcanerr_t uds_client_init(const struct uds_context *context, void *handle);

lwcanerr_t uds_start_diagnostic_session(const struct addr_can *addr, uint8_t session_type);

uint8_t get_active_session(void);

lwcanerr_t uds_reset_service(uint8_t sub_function);

lwcanerr_t uds_security_access_service(uint8_t sub_function, const uint8_t *parameter, uint8_t size);

void uds_set_sa_level(uint8_t level);

uint8_t uds_get_sa_level(void);

lwcanerr_t uds_tester_present_service(uint8_t sub_function);

lwcanerr_t uds_rdbli_service(uint8_t data_identifier);

lwcanerr_t uds_rdbi_service(uint16_t data_identifier);

lwcanerr_t uds_wdbi_service(uint16_t data_identifier, const uint8_t *data_record, uint8_t size);

lwcanerr_t uds_routine_control_service(uint8_t sub_function, uint16_t routine_identifier, const uint8_t *option, uint8_t size);

lwcanerr_t uds_clear_dtc_service(const uint8_t *dtc, uint8_t size);

lwcanerr_t uds_read_dtc_service(uint8_t sub_function, uint8_t mask);

lwcanerr_t uds_iocbi_service(uint16_t data_identifier, uint8_t parameter, const uint8_t *state, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif
