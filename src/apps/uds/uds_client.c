#include "lwcan/apps/uds_client.h"

#if LWCAN_ISOTP

#include "lwcan/isotp.h"
#include "lwcan/timeouts.h"
#include "lwcan/system.h"

#include <string.h>

#define UDS_SID_OFFSET 0
#define UDS_REJECTED_SID_OFFSET 1
#define UDS_NRC_OFFSET 2

#define UDS_RESPONSE_PENDING_NRC 0x78

#define UDS_TESTER_PRESENT_SID 0x3E
#define UDS_NEGATIVE_RESPONSE_SID 0x7F

typedef enum
{
    UDS_IDLE,

    UDS_CONNECTING,

    UDS_SENDING_REQUEST,

    UDS_WAIT_RESPONSE,

    UDS_WAIT_SEGMENTED_RESPONSE,

    UDS_RESPONSE_PENDING
} uds_state_t;

struct uds_state
{
    uint8_t state;

    uint8_t is_connected;

    uint8_t connect_try_num;

    uint32_t connect_try_time;

    uint8_t with_response;

    uint16_t p2;

    uint16_t p2_star;

    connect_cb_function connect_cb;

    void *connetct_cb_arg;

    const struct uds_context *context;

    void *handle;

    struct isotp_pcb *isotp_pcb;
};

static volatile struct uds_state uds_state;

static void connect_try(void *arg)
{
    uint8_t request[2];

    lwcanerr_t ret;

    (void)arg;

    if (!uds_state.is_connected && uds_state.connect_try_num == 0)
    {
        uds_state.connect_cb(uds_state.connetct_cb_arg, ERROR_ABORTED);

        uds_state.state = UDS_IDLE;

        return;
    }

    uds_state.connect_try_time = system_now();

    uds_state.connect_try_num -= 1;

    request[0] = UDS_TESTER_PRESENT_SID;
    request[1] = 0; // Not suppress positive response

    ret = isotp_send(uds_state.isotp_pcb, request, sizeof(request));

    if (ret != ERROR_OK)
    {
        uds_state.connect_cb(uds_state.connetct_cb_arg, ret);

        uds_state.state = UDS_IDLE;
    }
}

static void s3_timer_handler(void *arg)
{
    (void)arg;

#if UDS_KEEP_SESSION
    uint8_t request[2];

    request[0] = UDS_TESTER_PRESENT_SID;
    request[1] = UDS_KEEP_SESSION_SUPPRESS;

    uds_send_request(request, 2, 0);
#else
    uds_state.is_connected = 0;

    uds_state.p2 = UDS_P2_DEFAULT;

    uds_state.p2_star = UDS_P2_STAR_DEFAULT;
#endif
}

static void p2_timer_handler(void *arg)
{
    uint32_t connect_passed_time, connect_timeout;

    (void)arg;

    if (uds_state.state == UDS_CONNECTING)
    {
        connect_passed_time = system_now() - uds_state.connect_try_time;

        if (connect_passed_time >= UDS_CONNECT_RETRY_TIMEOUT)
        {
            connect_timeout = 0;
        }
        else
        {
            connect_timeout = UDS_CONNECT_RETRY_TIMEOUT - connect_passed_time;
        }

        lwcan_timeout(connect_timeout, connect_try, NULL);

        return;
    }

    if (uds_state.context != NULL && uds_state.context->error != NULL)
    {
        uds_state.context->error(uds_state.handle, ERROR_RECEIVE_TIMEOUT);
    }

    uds_state.state = UDS_IDLE;
}

static void p2_star_timer_handler(void *arg)
{
    (void)arg;

    if (uds_state.context != NULL && uds_state.context->error != NULL)
    {
        uds_state.context->error(uds_state.handle, ERROR_RECEIVE_TIMEOUT);
    }

    uds_state.state = UDS_IDLE;
}

static void handle_negative_response(struct lwcan_buffer *buffer)
{
    if (uds_state.state == UDS_CONNECTING)
    {
        uds_state.connect_cb(uds_state.connetct_cb_arg, ERROR_ABORTED);

        uds_state.state = UDS_IDLE;

        return;
    }

    if (buffer->payload[UDS_NRC_OFFSET] == UDS_RESPONSE_PENDING_NRC)
    {
        if (uds_state.state != UDS_RESPONSE_PENDING)
        {
            uds_state.state = UDS_RESPONSE_PENDING;
        }

        lwcan_timeout(uds_state.p2_star, p2_star_timer_handler, NULL);

        return;
    }

    if (uds_state.context != NULL && uds_state.context->negative_response != NULL)
    {
        uds_state.context->negative_response(uds_state.handle, buffer->payload[UDS_REJECTED_SID_OFFSET], buffer->payload[UDS_NRC_OFFSET]);
    }

    uds_state.state = UDS_IDLE;
}

static void handle_positive_response(struct lwcan_buffer *buffer)
{
    if (uds_state.state == UDS_CONNECTING)
    {
        uds_state.is_connected = 1;

        uds_state.connect_cb(uds_state.connetct_cb_arg, ERROR_OK);

        uds_state.state = UDS_IDLE;

        return;
    }

    if (uds_state.context != NULL && uds_state.context->positive_response != NULL)
    {
        uds_state.context->positive_response(uds_state.handle, buffer->payload, buffer->length);
    }

    uds_state.state = UDS_IDLE;
}

static void uds_receive(void *arg, struct isotp_pcb *isotp_pcb, struct lwcan_buffer *buffer)
{
    (void)arg;
    (void)isotp_pcb;

    if (uds_state.state == UDS_IDLE)
    {
        goto received;
    }

    switch (uds_state.state)
    {
    case UDS_WAIT_RESPONSE:
        lwcan_untimeout(p2_timer_handler, NULL);
        break;
    case UDS_CONNECTING:
        lwcan_untimeout(p2_timer_handler, NULL);
        break;
    case UDS_RESPONSE_PENDING:
        lwcan_untimeout(p2_star_timer_handler, NULL);
        break;
    default:
        break;
    }

    if (buffer->payload[UDS_SID_OFFSET] == UDS_NEGATIVE_RESPONSE_SID)
    {
        handle_negative_response(buffer);
    }
    else
    {
        handle_positive_response(buffer);
    }

received:
    isotp_received(isotp_pcb, buffer);
}

static void uds_receive_ff(void *arg, struct isotp_pcb *isotp_pcb)
{
    (void)arg;
    (void)isotp_pcb;

    if (uds_state.state == UDS_IDLE)
    {
        return;
    }

    switch (uds_state.state)
    {
    case UDS_WAIT_RESPONSE:
        lwcan_untimeout(p2_timer_handler, NULL);
        break;
    case UDS_RESPONSE_PENDING:
        lwcan_untimeout(p2_star_timer_handler, NULL);
        break;
    default:
        break;
    }

    uds_state.state = UDS_WAIT_SEGMENTED_RESPONSE;
}

static void uds_error(void *arg, lwcanerr_t error)
{
    uint32_t connect_passed_time, connect_timeout;

    (void)arg;

    if (uds_state.state == UDS_CONNECTING)
    {
        connect_passed_time = system_now() - uds_state.connect_try_time;

        if (connect_passed_time >= UDS_CONNECT_RETRY_TIMEOUT)
        {
            connect_timeout = 0;
        }
        else
        {
            connect_timeout = UDS_CONNECT_RETRY_TIMEOUT - connect_passed_time;
        }

        lwcan_timeout(connect_timeout, connect_try, NULL);

        return;
    }

    if (uds_state.state == UDS_IDLE)
    {
        return;
    }

    switch (uds_state.state)
    {
    case UDS_WAIT_RESPONSE:
        lwcan_untimeout(p2_timer_handler, NULL);
        break;
    case UDS_RESPONSE_PENDING:
        lwcan_untimeout(p2_star_timer_handler, NULL);
        break;
    default:
        break;
    }

    if (uds_state.context != NULL && uds_state.context->error != NULL)
    {
        uds_state.context->error(uds_state.handle, error);
    }

    uds_state.state = UDS_IDLE;
}

static void uds_sent(void *arg, struct isotp_pcb *pcb, uint32_t length)
{
    (void)arg;
    (void)pcb;
    (void)length;

    if (uds_state.state == UDS_CONNECTING)
    {
        lwcan_timeout(uds_state.p2, p2_timer_handler, NULL);

        return;
    }

    if (uds_state.state != UDS_SENDING_REQUEST)
    {
        return;
    }

    if (uds_state.with_response)
    {
        uds_state.state = UDS_WAIT_RESPONSE;

        lwcan_timeout(uds_state.p2, p2_timer_handler, NULL);
    }
    else
    {
        uds_state.state = UDS_IDLE;
    }

    lwcan_untimeout(s3_timer_handler, NULL);

    lwcan_timeout(UDS_S3_CLIENT, s3_timer_handler, NULL);
}

/**
 * @brief Initialize UDS client
 *
 * @return 0 if success or error code
 */
lwcanerr_t uds_client_init(void)
{
    struct isotp_pcb *isotp_pcb;

    isotp_pcb = isotp_new();

    if (isotp_pcb == NULL)
    {
        return ERROR_MEMORY;
    }

    isotp_set_receive_callback(isotp_pcb, uds_receive);

    isotp_set_receive_ff_callback(isotp_pcb, uds_receive_ff);

    isotp_set_sent_callback(isotp_pcb, uds_sent);

    isotp_set_error_callback(isotp_pcb, uds_error);

    memset((void *)&uds_state, 0, sizeof(struct uds_state));

    uds_state.isotp_pcb = isotp_pcb;

    uds_state.p2 = UDS_P2_DEFAULT;

    uds_state.p2_star = UDS_P2_STAR_DEFAULT;

    return ERROR_OK;
}

/**
 * @brief Deinitialize UDS client
 *
 * @return 0 if success or error code
 */
lwcanerr_t uds_client_deinit(void)
{
    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    isotp_remove(uds_state.isotp_pcb);

    if (uds_state.is_connected)
    {
        lwcan_untimeout(s3_timer_handler, NULL);
    }

    memset((void *)&uds_state, 0, sizeof(uds_state));

    return ERROR_OK;
}

/**
 * @brief Connect to server
 *
 * @param addr server address
 * @param connect_cb callback on connection results (successful or not (with error code))
 * @param arg callback arg
 * @return 0 if success or error code
 */
lwcanerr_t uds_client_connect(const struct addr_can *addr, connect_cb_function cb, void *arg)
{
    lwcanerr_t ret;

    if (cb == NULL)
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

    uds_state.state = UDS_CONNECTING;

    uds_state.connect_try_num = UDS_CONNECT_RETRY_NUM;

    uds_state.connect_cb = cb;

    uds_state.connetct_cb_arg = arg;

    connect_try(NULL);

    return ERROR_OK;
}

/**
 * @brief Disconnect from server
 *
 */
void uds_client_disconnect(void)
{
    if (!uds_state.is_connected)
    {
        return;
    }

    uds_state.state = UDS_IDLE;

    lwcan_untimeout(p2_timer_handler, NULL);

    lwcan_untimeout(p2_star_timer_handler, NULL);

    lwcan_untimeout(s3_timer_handler, NULL);

    uds_state.is_connected = 0;

    uds_state.p2 = UDS_P2_DEFAULT;

    uds_state.p2_star = UDS_P2_STAR_DEFAULT;
}

/**
 * @brief Set UDS session context
 * (can be changed during an active session if there is no communication at the moment)
 *
 * @param context session context
 * @param handle context handle
 * @return 0 if success or error code
 */
lwcanerr_t uds_set_context(const struct uds_context *context, void *handle)
{
    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    uds_state.context = context;

    uds_state.handle = handle;

    return ERROR_OK;
}

/**
 * @brief Set time for P2 timer
 *
 * @param time time in milliseconds
 * @return lwcanerr_t
 */
lwcanerr_t uds_set_p2_timer(uint32_t time)
{
    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    uds_state.p2 = time;

    return ERROR_OK;
}

/**
 * @brief Set time for P2* timer
 *
 * @param time time in milliseconds
 * @return 0 if success or error code
 */
lwcanerr_t uds_set_p2_star_timer(uint32_t time)
{
    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    uds_state.p2_star = time;

    return ERROR_OK;
}

/**
 * @brief Send uds request
 *
 * @param request pre-formed request according to the used UDS implementation (e.g. {0x31, 0x01, 0x00, 0x04, 0x11})
 * @param size request size
 * @param with_response 0 if there is no response to the request, or >0 if there is a response to the request
 * @return 0 if success or error code
 */
lwcanerr_t uds_send_request(const void *request, uint32_t size, uint8_t with_response)
{
    if (request == NULL || size == 0)
    {
        return ERROR_ARG;
    }

    if (!uds_state.is_connected)
    {
        return ERROR_CONNECT;
    }

    if (uds_state.state != UDS_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    uds_state.with_response = with_response;

    uds_state.state = UDS_SENDING_REQUEST;

    return isotp_send(uds_state.isotp_pcb, request, size);
}

#endif
