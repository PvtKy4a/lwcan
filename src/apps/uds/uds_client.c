#include "lwcan/apps/uds_client.h"

#if LWCAN_ISOTP

#include "lwcan/isotp.h"
#include "lwcan/timeouts.h"
#include "lwcan/memory.h"

#include "string.h"

#define UDS_SID_OFFSET 0

#define UDS_REJECTED_SID_OFFSET 1

#define UDS_NRC_OFFSET 2

#define POSITIVE_RESPOSNE_PADDING 0x40

typedef enum
{
    UDS_IDLE,

    UDS_WAIT_RESPONSE,

    UDS_RESPONSE_PENDING,
} uds_state_t;

struct uds_state
{
    uint8_t state;

    uint8_t session;

    uint8_t sa_level;

    uint16_t p2;

    uint16_t p2_star;

    const struct uds_context *context;

    void *handle;

    struct isotp_pcb *isotp_pcb;
};

static struct uds_state uds_state;

static void s3_client_timer_handler(void *arg)
{
    (void)arg;

#if UDS_KEEP_SESSION
    uds_tester_present_service(UDS_KEEP_SESSION_SUB);
#else
    uds_state.session = 0;

    uds_state.sa_level = 0;
#endif
}

static void p2_timer_handler(void *arg)
{
    (void)arg;

    uds_state.state = UDS_IDLE;

    if (uds_state.context->error != NULL)
    {
        uds_state.context->error(uds_state.handle, ERROR_RECEIVE_TIMEOUT);
    }
}

static void p2_star_timer_handler(void *arg)
{
    (void)arg;

    uds_state.state = UDS_IDLE;

    if (uds_state.context->error != NULL)
    {
        uds_state.context->error(uds_state.handle, ERROR_RECEIVE_TIMEOUT);
    }
}

static lwcanerr_t uds_receive(void *arg, struct isotp_pcb *isotp_pcb, struct lwcan_buffer *buffer)
{
    (void)arg;
    (void)isotp_pcb;

    if (uds_state.state == UDS_IDLE)
    {
        goto exit;
    }

    switch (buffer->payload[UDS_SID_OFFSET])
    {
    case UDS_REQUEST_RECEIVED_RESPONSE_PENDING_NRC:
        if (uds_state.state == UDS_WAIT_RESPONSE)
        {
            lwcan_untimeout(p2_timer_handler, NULL);
            lwcan_timeout(uds_state.p2_star, p2_star_timer_handler, NULL);
            uds_state.state = UDS_RESPONSE_PENDING;
        }
        goto exit;

    case UDS_NEGATIVE_RESPONSE_SID:
        lwcan_untimeout(p2_timer_handler, NULL);
        lwcan_untimeout(p2_star_timer_handler, NULL);
        uds_state.state = UDS_IDLE;
        if (uds_state.context->negative_response != NULL)
        {
            uds_state.context->negative_response(uds_state.handle, buffer->payload[UDS_REJECTED_SID_OFFSET], buffer->payload[UDS_NRC_OFFSET]);
        }
        goto exit;

    case (UDS_DIAGNOSTIC_SESSION_CONTROL_SID + POSITIVE_RESPOSNE_PADDING):
        uds_state.session = buffer->payload[UDS_SID_OFFSET + 1];
        uds_state.p2 = buffer->payload[UDS_SID_OFFSET + 2] << 8;
        uds_state.p2 |= buffer->payload[UDS_SID_OFFSET + 3];
        uds_state.p2_star = buffer->payload[UDS_SID_OFFSET + 4] << 8;
        uds_state.p2_star |= buffer->payload[UDS_SID_OFFSET + 5];
        lwcan_timeout(UDS_S3_CLIENT, s3_client_timer_handler, NULL);
        goto positive_response;

    case (UDS_ECU_RESET_SID + POSITIVE_RESPOSNE_PADDING):
    case (UDS_SECURITY_ACCESS_SID + POSITIVE_RESPOSNE_PADDING):
    case (UDS_READ_DATA_BY_LOCAL_IDENTIFIER_SID + POSITIVE_RESPOSNE_PADDING):
    case (UDS_READ_DATA_BY_IDENTIFIER_SID + POSITIVE_RESPOSNE_PADDING):
    case (UDS_WRITE_DATA_BY_IDENTIFIER_SID + POSITIVE_RESPOSNE_PADDING):
    case (UDS_ROUTINE_CONTROL_SID + POSITIVE_RESPOSNE_PADDING):
    case (UDS_CLEAR_DIAGNOSTIC_INFORMATION_SID + POSITIVE_RESPOSNE_PADDING):
    case (UDS_READ_DTC_INFORMATION_SID + POSITIVE_RESPOSNE_PADDING):
    case (UDS_INPUT_OUTPUT_CONTROL_BY_IDENTIFIER_SID + POSITIVE_RESPOSNE_PADDING):
        goto positive_response;

    default:
        goto exit;
    }

positive_response:
    lwcan_untimeout(p2_timer_handler, NULL);

    lwcan_untimeout(p2_star_timer_handler, NULL);

    uds_state.state = UDS_IDLE;

    if (uds_state.context->positive_response != NULL)
    {
        uds_state.context->positive_response(uds_state.handle, (buffer->payload[UDS_SID_OFFSET] - POSITIVE_RESPOSNE_PADDING), (buffer->payload + 1), (buffer->length - 1));
    }

exit:
    isotp_received(isotp_pcb, buffer);

    return ERROR_OK;
}

static void uds_error(void *arg, lwcanerr_t error)
{
    (void)arg;

    if (uds_state.context->error != NULL)
    {
        uds_state.context->error(uds_state.handle, error);
    }
}

lwcanerr_t uds_client_init(const struct uds_context *context, void *handle)
{
    struct isotp_pcb *isotp_pcb;

    if (context == NULL)
    {
        return ERROR_ARG;
    }

    isotp_pcb = isotp_new();

    if (isotp_pcb == NULL)
    {
        return ERROR_MEMORY;
    }

    memset(&uds_state, 0, sizeof(struct uds_state));

    isotp_set_receive_callback(isotp_pcb, uds_receive);

    isotp_set_error_callback(isotp_pcb, uds_error);

    uds_state.isotp_pcb = isotp_pcb;

    uds_state.context = context;

    uds_state.handle = handle;

    uds_state.p2 = UDS_P2_DEFAULT;

    uds_state.p2_star = UDS_P2_STAR_DEFAULT;

    return ERROR_OK;
}

void uds_client_cleanup(void)
{
    isotp_remove(uds_state.isotp_pcb);

    memset(&uds_state, 0, sizeof(uds_state));
}

static lwcanerr_t send_request(uint8_t *data, uint32_t size)
{
    lwcanerr_t ret;

    if (uds_state.session != 0)
    {
        lwcan_untimeout(s3_client_timer_handler, NULL);
    }

    ret = isotp_send(uds_state.isotp_pcb, data, size);

    if (ret != ERROR_OK)
    {
        return ret;
    }

    uds_state.state = UDS_WAIT_RESPONSE;

    lwcan_timeout(uds_state.p2, p2_timer_handler, NULL);

    if (uds_state.session != 0)
    {
        lwcan_timeout(UDS_S3_CLIENT, s3_client_timer_handler, NULL);
    }

    return ERROR_OK;
}

lwcanerr_t uds_start_diagnostic_session(const struct addr_can *addr, uint8_t session_type)
{
    uint8_t data[2];

    lwcanerr_t ret;

    if (addr == NULL)
    {
        return ERROR_ARG;
    }

    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    ret = isotp_bind(uds_state.isotp_pcb, addr);

    if (ret != ERROR_OK)
    {
        return ret;
    }

    data[UDS_SID_OFFSET] = UDS_DIAGNOSTIC_SESSION_CONTROL_SID;

    data[UDS_SID_OFFSET + 1] = session_type;

    return send_request(data, sizeof(data));
}

uint8_t get_active_session(void)
{
    return uds_state.session;
}

lwcanerr_t uds_reset_service(uint8_t sub_function)
{
    uint8_t data[2];

    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    data[UDS_SID_OFFSET] = UDS_ECU_RESET_SID;

    data[UDS_SID_OFFSET + 1] = sub_function;

    return send_request(data, sizeof(data));
}

lwcanerr_t uds_security_access_service(uint8_t sub_function, const uint8_t *parameter, uint8_t size)
{
    uint8_t *data;

    lwcanerr_t ret;

    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    data = (uint8_t *)lwcan_malloc(size + 2);

    if (data == NULL)
    {
        return ERROR_MEMORY;
    }

    data[UDS_SID_OFFSET] = UDS_SECURITY_ACCESS_SID;

    data[UDS_SID_OFFSET + 1] = sub_function;

    memcpy(data + UDS_SID_OFFSET + 2, parameter, size);

    ret = send_request(data, size + 2);

    lwcan_free(data);

    return ret;
}

void uds_set_sa_level(uint8_t level)
{
    uds_state.sa_level = level;
}

uint8_t uds_get_sa_level(void)
{
    return uds_state.sa_level;
}

lwcanerr_t uds_tester_present_service(uint8_t sub_function)
{
    uint8_t data[2];

    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    data[UDS_SID_OFFSET] = UDS_TESTER_PRESENT_SID;

    data[UDS_SID_OFFSET + 1] = sub_function;

    return send_request(data, sizeof(data));
}

lwcanerr_t uds_rdbli_service(uint8_t data_identifier)
{
    uint8_t data[2];

    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    data[UDS_SID_OFFSET] = UDS_READ_DATA_BY_LOCAL_IDENTIFIER_SID;

    data[UDS_SID_OFFSET + 1] = data_identifier;

    return send_request(data, sizeof(data));
}

lwcanerr_t uds_rdbi_service(uint16_t data_identifier)
{
    uint8_t data[3];

    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    data[UDS_SID_OFFSET] = UDS_READ_DATA_BY_IDENTIFIER_SID;

    data[UDS_SID_OFFSET + 1] = (uint8_t)(data_identifier >> 8) & (uint8_t)0xFF;

    data[UDS_SID_OFFSET + 2] = (uint8_t)data_identifier & (uint8_t)0xFF;

    return send_request(data, sizeof(data));
}

lwcanerr_t uds_wdbi_service(uint16_t data_identifier, const uint8_t *data_record, uint8_t size)
{
    uint8_t *data;

    lwcanerr_t ret;

    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    data = (uint8_t *)lwcan_malloc(size + 3);

    if (data == NULL)
    {
        return ERROR_MEMORY;
    }

    data[UDS_SID_OFFSET] = UDS_WRITE_DATA_BY_IDENTIFIER_SID;

    data[UDS_SID_OFFSET + 1] = (uint8_t)(data_identifier >> 8) & (uint8_t)0xFF;

    data[UDS_SID_OFFSET + 2] = (uint8_t)data_identifier & (uint8_t)0xFF;

    memcpy(data + UDS_SID_OFFSET + 3, data_record, size);

    ret = send_request(data, size + 3);

    lwcan_free(data);

    return ret;
}

lwcanerr_t uds_routine_control_service(uint8_t sub_function, uint16_t routine_identifier, const uint8_t *option, uint8_t size)
{
    uint8_t *data;

    lwcanerr_t ret;

    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    data = (uint8_t *)lwcan_malloc(size + 4);

    if (data == NULL)
    {
        return ERROR_MEMORY;
    }

    data[UDS_SID_OFFSET] = UDS_ROUTINE_CONTROL_SID;

    data[UDS_SID_OFFSET + 1] = sub_function;

    data[UDS_SID_OFFSET + 2] = (uint8_t)(routine_identifier >> 8) & (uint8_t)0xFF;

    data[UDS_SID_OFFSET + 3] = (uint8_t)routine_identifier & (uint8_t)0xFF;

    memcpy(data + UDS_SID_OFFSET + 4, option, size);

    ret = send_request(data, size + 4);

    lwcan_free(data);

    return ret;
}

lwcanerr_t uds_clear_dtc_service(const uint8_t *dtc, uint8_t size)
{
    uint8_t *data;

    lwcanerr_t ret;

    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    data = (uint8_t *)lwcan_malloc(size + 1);

    if (data == NULL)
    {
        return ERROR_MEMORY;
    }

    data[UDS_SID_OFFSET] = UDS_CLEAR_DIAGNOSTIC_INFORMATION_SID;

    memcpy(data + UDS_SID_OFFSET + 1, dtc, size);

    ret = send_request(data, size + 1);

    lwcan_free(data);

    return ret;
}

lwcanerr_t uds_read_dtc_service(uint8_t sub_function, uint8_t mask)
{
    uint8_t data[3];

    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    data[UDS_SID_OFFSET] = UDS_READ_DTC_INFORMATION_SID;

    data[UDS_SID_OFFSET + 1] = sub_function;

    data[UDS_SID_OFFSET + 2] = mask;

    return send_request(data, sizeof(data));
}

lwcanerr_t uds_iocbi_service(uint16_t data_identifier, uint8_t parameter, const uint8_t *state, uint8_t size)
{
    uint8_t *data;

    lwcanerr_t ret;

    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    data = (uint8_t *)lwcan_malloc(size + 4);

    if (data == NULL)
    {
        return ERROR_MEMORY;
    }

    data[UDS_SID_OFFSET] = UDS_ROUTINE_CONTROL_SID;

    data[UDS_SID_OFFSET + 1] = (uint8_t)(data_identifier >> 8) & (uint8_t)0xFF;

    data[UDS_SID_OFFSET + 2] = (uint8_t)data_identifier & (uint8_t)0xFF;

    data[UDS_SID_OFFSET + 3] = parameter;

    memcpy(data + UDS_SID_OFFSET + 4, state, size);

    ret = send_request(data, size + 4);

    lwcan_free(data);

    return ret;
}

#endif
