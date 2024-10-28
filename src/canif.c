#include "lwcan/canif.h"
#include "lwcan/error.h"
#include "lwcan/options.h"
#include "lwcan/private/isotp_private.h"
#include "lwcan/private/raw_private.h"

#include <stdlib.h>
#include <string.h>

#define MAX_CANIF_NUM 254

static struct canif *canif_list = NULL;

static uint8_t canif_num = 0;

lwcanerr_t canif_add(struct canif *canif, uint8_t if_index, canif_init_function init, canif_input_function input, canif_sent_function sent)
{
    struct canif *canif_temp;

    uint8_t num_canifs;

    if (canif == NULL || init == NULL || input == NULL)
    {
        return ERROR_ARG;
    }

    memset(canif, 0, sizeof(struct canif));

    canif->input = input;

    canif->sent = sent;

    canif->num = canif_num;

    canif->if_index = if_index;

    if (init(canif) != ERROR_OK)
    {
        return ERROR_CANIF;
    }

    do
    {
        if (canif->num == MAX_CANIF_NUM)
        {
            canif->num = 0;
        }

        num_canifs = 0;

        for (canif_temp = canif_list; canif_temp != NULL; canif_temp = canif_temp->next)
        {
            if (canif_temp == canif)
            {
                return ERROR_CANIF;
            }

            num_canifs++;

            if (num_canifs >= MAX_CANIF_NUM)
            {
                return ERROR_CANIF_MAX;
            }

            if (canif_temp->num == canif->num)
            {
                canif->num++;

                break;
            }
        }
    } while (canif_temp != NULL);

    if (canif->num == (MAX_CANIF_NUM - 1))
    {
        canif_num = 0;
    }
    else
    {
        canif_num = (uint8_t)(canif->num + 1);
    }

    canif->next = canif_list;

    canif_list = canif;

    return ERROR_OK;
}

lwcanerr_t canif_remove(struct canif *canif)
{
    struct canif *canif_temp;

    lwcanerr_t ret = ERROR_CANIF;

    if (canif == NULL)
    {
        ret = ERROR_ARG;

        goto exit;
    }

    if (canif_list == canif)
    {
        canif_list = canif->next;
    }
    else
    {
        for (canif_temp = canif_list; canif_temp != NULL; canif_temp = canif_temp->next)
        {
            if (canif_temp->next == canif)
            {
                canif_temp->next = canif->next;

                ret = ERROR_OK;

                break;
            }
        }
        if (canif_temp == NULL)
        {
            ret = ERROR_OK;

            goto exit;
        }
    }

exit:
    return ret;
}

lwcanerr_t canif_input(struct canif *canif, struct lwcan_frame *frame)
{
#if LWCAN_RAW
    canraw_io_state_t raw_status;
#endif

    if (canif == NULL || frame == NULL)
    {
        return ERROR_ARG;
    }

#if LWCAN_RAW
    raw_status = canraw_input(canif, frame);

    if (raw_status != RAW_IO_EATEN)
#endif
    {
#if LWCAN_ISOTP
        isotp_input(canif, frame);
#endif
    }

    return ERROR_OK;
}

lwcanerr_t canif_sent(struct canif *canif, struct lwcan_frame *frame)
{
#if LWCAN_RAW
    canraw_io_state_t raw_status;
#endif

    if (canif == NULL || frame == NULL)
    {
        return ERROR_ARG;
    }

#if LWCAN_RAW
    raw_status = canraw_sent(canif, frame);

    if (raw_status != RAW_IO_SENT)
#endif
    {
#if LWCAN_ISOTP
        isotp_sent(canif, frame);
#endif
    }

    return ERROR_OK;
}

uint8_t canif_get_index(struct canif *canif)
{
    uint8_t ret;

    if (canif == NULL)
    {
        ret = 0;

        goto exit;
    }

    ret = canif->num + 1;

exit:
    return ret;
}

struct canif *canif_get_by_index(uint8_t index)
{
    struct canif *canif;

    if (index == 0)
    {
        canif = NULL;

        goto exit;
    }

    for (canif = canif_list; canif != NULL; canif = canif->next)
    {
        if (index == canif_get_index(canif))
        {
            break;
        }
    }

exit:
    return canif;
}
