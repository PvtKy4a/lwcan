#include "lwcan/apps/uds_client.h"

#if LWCAN_ISOTP

#include "lwcan/isotp.h"
#include "lwcan/timeouts.h"
#include "lwcan/memory.h"

#include "string.h"

#define UDS_SID_OFFSET 0

#define UDS_REJECTED_SID_OFFSET 1

#define UDS_NRC_OFFSET 2

typedef enum
{
    UDS_IDLE,

    UDS_WAIT_RESPONSE,

    UDS_WAIT_SEGMENTED_RESPONSE,

    UDS_RESPONSE_PENDING,
} uds_state_t;

struct uds_state
{
    uint8_t state;

    uint8_t session;

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
    uint8_t request[2];

    request[0] = UDS_TESTER_PRESENT_SID;
    request[1] = UDS_KEEP_SESSION_SUB;

    uds_send_request(request, sizeof(request), UDS_SUPPRESS_KEEP_SESSION_RESPONSE);
#else
    uds_state.session = 0;
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

static void uds_receive(void *arg, struct isotp_pcb *isotp_pcb, struct lwcan_buffer *buffer)
{
    (void)arg;
    (void)isotp_pcb;

    if (uds_state.state == UDS_IDLE)
    {
        goto exit;
    }

    if (uds_state.state == UDS_WAIT_RESPONSE)
    {
        lwcan_untimeout(p2_timer_handler, NULL);
    }

    if (buffer->payload[UDS_SID_OFFSET] == UDS_REQUEST_RECEIVED_RESPONSE_PENDING_NRC)
    {
        if (buffer->payload[UDS_NRC_OFFSET] == UDS_REQUEST_RECEIVED_RESPONSE_PENDING_NRC)
        {
            if (uds_state.state != UDS_RESPONSE_PENDING)
            {
                lwcan_timeout(uds_state.p2_star, p2_star_timer_handler, NULL);

                uds_state.state = UDS_RESPONSE_PENDING;
            }
            else
            {
                lwcan_untimeout(p2_star_timer_handler, NULL);

                lwcan_timeout(uds_state.p2_star, p2_star_timer_handler, NULL);
            }
        }
        else
        {
            if (uds_state.state == UDS_RESPONSE_PENDING)
            {
                lwcan_untimeout(p2_star_timer_handler, NULL);
            }

            uds_state.state = UDS_IDLE;

            if (uds_state.context->negative_response != NULL)
            {
                uds_state.context->negative_response(uds_state.handle, buffer->payload[UDS_REJECTED_SID_OFFSET], buffer->payload[UDS_NRC_OFFSET]);
            }
        }
    }
    else
    {
        if (uds_state.state == UDS_RESPONSE_PENDING)
        {
            lwcan_untimeout(p2_star_timer_handler, NULL);
        }

        if (buffer->payload[UDS_SID_OFFSET] == (UDS_DIAGNOSTIC_SESSION_CONTROL_SID | UDS_POSITIVE_RESPOSNE_PADDING))
        {
            if (uds_state.session == 0)
            {
                lwcan_timeout(UDS_S3_CLIENT, s3_client_timer_handler, NULL);
            }

            uds_state.session = buffer->payload[1];

            uds_state.p2 = buffer->payload[2] << 8;
            uds_state.p2 |= buffer->payload[3];

            uds_state.p2_star = buffer->payload[4] << 8;
            uds_state.p2_star |= buffer->payload[5];
        }

        uds_state.state = UDS_IDLE;

        if (uds_state.context->positive_response != NULL)
        {
            uds_state.context->positive_response(uds_state.handle, buffer->payload, buffer->length);
        }
    }

exit:
    isotp_received(isotp_pcb, buffer);
}

static void uds_receive_ff(void *arg, struct isotp_pcb *isotp_pcb)
{
    (void)arg;
    (void)isotp_pcb;

    lwcan_untimeout(p2_timer_handler, NULL);

    uds_state.state = UDS_WAIT_SEGMENTED_RESPONSE;
}

static void uds_error(void *arg, lwcanerr_t error)
{
    (void)arg;

    uds_state.state = UDS_IDLE;

    lwcan_untimeout(p2_timer_handler, NULL);

    lwcan_untimeout(p2_star_timer_handler, NULL);

    if (uds_state.context->error != NULL)
    {
        uds_state.context->error(uds_state.handle, error);
    }
}

/**
 * @brief initialize UDS client
 * 
 * @return error code
 */
lwcanerr_t uds_client_init(void)
{
    struct isotp_pcb *isotp_pcb;

    isotp_pcb = isotp_new();

    if (isotp_pcb == NULL)
    {
        return ERROR_MEMORY;
    }

    memset(&uds_state, 0, sizeof(struct uds_state));

    isotp_set_receive_callback(isotp_pcb, uds_receive);

    isotp_set_receive_ff_callback(isotp_pcb, uds_receive_ff);

    isotp_set_error_callback(isotp_pcb, uds_error);

    uds_state.isotp_pcb = isotp_pcb;

    uds_state.p2 = UDS_P2_DEFAULT;

    uds_state.p2_star = UDS_P2_STAR_DEFAULT;

    return ERROR_OK;
}

/**
 * @brief returns the UDS client to its original state
 * (after this, the client must be initialized)
 * 
 */
void uds_client_cleanup(void)
{
    isotp_remove(uds_state.isotp_pcb);

    lwcan_untimeout(p2_timer_handler, NULL);

    lwcan_untimeout(p2_star_timer_handler, NULL);

    lwcan_untimeout(s3_client_timer_handler, NULL);

    memset(&uds_state, 0, sizeof(uds_state));
}

/**
 * @brief set UDS session context
 * (can be changed during an active session if there is no communication at the moment)
 *
 * @param context session context
 * @param handle context handle
 * @return error code
 */
lwcanerr_t uds_set_context(const struct uds_context *context, void *handle)
{
    if (context == NULL)
    {
        return ERROR_ARG;
    }

    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    uds_state.context = context;

    uds_state.handle = handle;

    return ERROR_OK;
}

/**
 * @brief start UDS session
 *
 * @param addr server address
 * @param session_type required session
 * @return error code
 */
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

    data[0] = UDS_DIAGNOSTIC_SESSION_CONTROL_SID;
    data[1] = session_type;

    return uds_send_request(data, sizeof(data), 1);
}

/**
 * @brief close active session (immediately)
 *
 */
void uds_close_diagnostic_session(void)
{
    uds_state.state = UDS_IDLE;

    lwcan_untimeout(s3_client_timer_handler, NULL);

    uds_state.session = 0;

    uds_state.p2 = UDS_P2_DEFAULT;

    uds_state.p2_star = UDS_P2_STAR_DEFAULT;
}

/**
 * @brief get active UDS session
 *
 * @return uds session
 */
uint8_t uds_get_active_session(void)
{
    return uds_state.session;
}

/**
 * @brief send uds request
 *
 * @param request pre-formed request according to the used UDS implementation (e.g. {0x31, 0x01, 0x00, 0x04, 0x11})
 * @param size request size
 * @param with_response 0 if there is no response to the request, or >0 if there is a response to the request
 * @return error code
 */
lwcanerr_t uds_send_request(const void *request, uint32_t size, uint8_t with_response)
{
    lwcanerr_t ret;

    if (request == NULL || size == 0)
    {
        return ERROR_ARG;
    }

    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    if (uds_state.session != 0)
    {
        lwcan_untimeout(s3_client_timer_handler, NULL);
    }

    ret = isotp_send(uds_state.isotp_pcb, request, size);

    if (ret != ERROR_OK)
    {
        return ret;
    }

    if (with_response)
    {
        uds_state.state = UDS_WAIT_RESPONSE;

        lwcan_timeout(uds_state.p2, p2_timer_handler, NULL);
    }

    if (uds_state.session != 0)
    {
        lwcan_timeout(UDS_S3_CLIENT, s3_client_timer_handler, NULL);
    }

    return ERROR_OK;
}

#endif
