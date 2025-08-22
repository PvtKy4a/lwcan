#include "lwcan/canif.h"
#include "lwcan/error.h"
#include "lwcan/options.h"
#include "lwcan/private/isotp_private.h"
#include "lwcan/private/raw_private.h"
#include "lwcan/debug.h"

#include <string.h>

#define MAX_CANIF_NUM 254

static struct canif *canif_list = NULL;

static uint8_t canif_num = 0;

static lwcanerr_t canif_input(struct canif *canif, void *frame)
{
#if LWCAN_RAW
    canraw_input_state_t raw_status;
#endif

    if (canif == NULL || frame == NULL)
    {
        LWCAN_ASSERT("canif != NULL", canif != NULL);
        LWCAN_ASSERT("frame != NULL", frame != NULL);

        return ERROR_ARG;
    }

#if LWCAN_RAW
    raw_status = canraw_input(canif, frame);

    if (raw_status != RAW_INPUT_EATEN)
#endif
    {
#if LWCAN_ISOTP
        isotp_input(canif, frame);
#endif
    }

    return ERROR_OK;
}

lwcanerr_t canif_add(struct canif *canif, const char *name, canif_init_function init)
{
    struct canif *canif_temp;

    uint8_t num_canifs;

    if (canif == NULL || name == NULL || strlen(name) == 0 || strlen(name) > sizeof(canif->name) || init == NULL)
    {
        LWCAN_ASSERT("canif != NULL", canif != NULL);
        LWCAN_ASSERT("name != NULL", name != NULL);
        LWCAN_ASSERT("strlen(name) != 0", strlen(name) != 0);
        LWCAN_ASSERT("strlen(name) <= sizeof(canif->name)", strlen(name) <= sizeof(canif->name));
        LWCAN_ASSERT("init != NULL", init != NULL);

        return ERROR_ARG;
    }

    memset(canif, 0, sizeof(struct canif));

    memcpy(canif->name, name, strlen(name));

    canif->input = canif_input;

    canif->num = canif_num;

    if (init(canif) != ERROR_OK)
    {
        return ERROR_IF;
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

    if (canif == NULL)
    {
        LWCAN_ASSERT("canif != NULL", canif != NULL);

        return ERROR_ARG;
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

                break;
            }
        }
    }

    return ERROR_OK;
}

lwcanerr_t canif_set_bitrate(struct canif *canif, uint32_t bitrate)
{
    if (canif == NULL || bitrate == 0)
    {
        LWCAN_ASSERT("canif != NULL", canif != NULL);
        LWCAN_ASSERT("bitrate != 0", bitrate != 0);

        return ERROR_ARG;
    }

    if (canif->set_bitrate == NULL)
    {
        return ERROR_CANIF;
    }

    return canif->set_bitrate(canif, bitrate);
}

lwcanerr_t canif_set_filter(struct canif *canif, struct can_filter *filter)
{
    if (canif == NULL || filter == NULL)
    {
        LWCAN_ASSERT("canif != NULL", canif != NULL);
        LWCAN_ASSERT("filter != NULL", filter != NULL);

        return ERROR_ARG;
    }

    if (canif->set_filter == NULL)
    {
        return ERROR_CANIF;
    }

    return canif->set_filter(canif, filter);
}

const char *canif_get_name(struct canif *canif)
{
    if (canif == NULL)
    {
        LWCAN_ASSERT("canif != NULL", canif != NULL);

        return NULL;
    }

    return canif->name;
}

struct canif *canif_get_by_name(const char *name)
{
    struct canif *canif;

    if (name == NULL)
    {
        LWCAN_ASSERT("name != NULL", name != NULL);

        return NULL;
    }

    for (canif = canif_list; canif != NULL; canif = canif->next)
    {
        if (memcmp(canif->name, name, sizeof(canif->name)) == 0)
        {
            break;
        }
    }

    return canif;
}

uint8_t canif_name_to_index(const char *name)
{
    struct canif *canif;

    canif = canif_get_by_name(name);

    return canif_get_index(canif);
}

const char *canif_index_to_name(uint8_t index)
{
    struct canif *canif = canif_get_by_index(index);

    return canif_get_name(canif);
}

uint8_t canif_get_index(struct canif *canif)
{
    if (canif == NULL)
    {
        LWCAN_ASSERT("canif != NULL", canif != NULL);

        return 0;
    }

    return (canif->num + 1);
}

struct canif *canif_get_by_index(uint8_t index)
{
    struct canif *canif;

    if (index == 0)
    {
        LWCAN_ASSERT("index != 0", index != 0);

        return NULL;
    }

    for (canif = canif_list; canif != NULL; canif = canif->next)
    {
        if (index == canif_get_index(canif))
        {
            break;
        }
    }

    return canif;
}
