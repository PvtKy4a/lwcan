#include "lwcan/options.h"

#if LWCAN_RAW /* don't build if not configured for use in lwcan_options.h */

#include "lwcan/raw.h"
#include "lwcan/private/raw_private.h"
#include "lwcan/timeouts.h"

#include <string.h>

#define CANRAW_PCB_MEM_CHUNK_SIZE sizeof(struct canraw_pcb)

#define CANRAW_PCB_MEM_POOL_SIZE ((CANRAW_PCB_MEM_CHUNK_SIZE * CANRAW_MAX_PCB_NUM) + CANRAW_MAX_PCB_NUM)

static uint8_t canraw_pcb_mem_pool[CANRAW_PCB_MEM_POOL_SIZE];

static struct canraw_pcb *canraw_pcb_list;

static uint8_t canraw_pcb_num = 0;

static void *canraw_pcb_malloc(void)
{
    for (uint16_t i = 0; i < CANRAW_PCB_MEM_POOL_SIZE; i += (CANRAW_PCB_MEM_CHUNK_SIZE + 1))
    {
        if (canraw_pcb_mem_pool[i + CANRAW_PCB_MEM_CHUNK_SIZE] == 0)
        {
            canraw_pcb_mem_pool[i + CANRAW_PCB_MEM_CHUNK_SIZE] = 0xAA;

            return &canraw_pcb_mem_pool[i];
        }
    }

    return NULL;
}

static void canraw_pcb_free(void *mem)
{
    if (((uint8_t *)mem > &canraw_pcb_mem_pool[CANRAW_PCB_MEM_POOL_SIZE - CANRAW_PCB_MEM_CHUNK_SIZE - 1]) || ((uint8_t *)mem < &canraw_pcb_mem_pool[0]))
    {
        return;
    }

    ((uint8_t *)mem)[CANRAW_PCB_MEM_CHUNK_SIZE] = 0;
}

void canraw_init(void)
{
    memset(canraw_pcb_mem_pool, 0, sizeof(canraw_pcb_mem_pool));

    canraw_pcb_list = NULL;
}

struct canraw_pcb *canraw_new(uint8_t canif_index)
{
    struct canraw_pcb *pcb;

    if (canif_index == 0 || canraw_pcb_num >= CANRAW_MAX_PCB_NUM)
    {
        return NULL;
    }

    pcb = (struct canraw_pcb *)canraw_pcb_malloc();

    if (pcb == NULL)
    {
        return NULL;
    }

    memset(pcb, 0, sizeof(struct canraw_pcb));

    pcb->canif_index = canif_index;

    pcb->next = canraw_pcb_list;

    canraw_pcb_list = pcb;

    canraw_pcb_num += 1;

    return pcb;

}

lwcanerr_t canraw_bind(struct canraw_pcb *pcb, uint8_t canif_index)
{
    if (pcb == NULL ||canif_index == 0)
    {
        return ERROR_ARG;
    }

    pcb->canif_index = canif_index;

    return ERROR_OK;
}

lwcanerr_t canraw_close(struct canraw_pcb *pcb)
{
    struct canraw_pcb *pcb_temp;

    lwcanerr_t ret;

    if (pcb == NULL)
    {
        ret = ERROR_ARG;

        goto exit;
    }

    if (canraw_pcb_list == pcb)
    {
        canraw_pcb_list = pcb->next;
    }
    else
    {
        for (pcb_temp = canraw_pcb_list; pcb_temp != NULL; pcb_temp = pcb_temp->next)
        {
            if (pcb_temp->next == pcb)
            {
                pcb_temp->next = pcb->next;

                ret = ERROR_OK;

                break;
            }
        }
        if (pcb_temp == NULL)
        {
            ret = ERROR_OK;

            goto exit;
        }
    }

    canraw_pcb_free(pcb);

    canraw_pcb_num -= 1;

exit:
    return ret;
}

canraw_input_state_t canraw_input(struct canif *canif, struct lwcan_frame *frame)
{
    struct canraw_pcb *pcb;

    struct canraw_pcb *prev = NULL;

    uint8_t canif_index;

    if (canif == NULL || frame == NULL)
    {
        goto exit_none;
    }

    canif_index = canif_get_index(canif);

    if (canif_index == 0)
    {
        goto exit_none;
    }

    pcb = canraw_pcb_list;

    while (pcb != NULL)
    {
        if (pcb->canif_index == canif_index && pcb->receive != NULL && pcb->receive(pcb->callback_arg, pcb, frame) != 0)
        {
            if (prev != NULL)
            {
                /* move the pcb to the front of canraw_pcb_list so that is found faster next time */
                prev->next = pcb->next;

                pcb->next = canraw_pcb_list;

                canraw_pcb_list = pcb;
            }

            return RAW_INPUT_EATEN;
        }
        else
        {
            prev = pcb;

            pcb = pcb->next;
        }
    }

exit_none:
    return RAW_INPUT_NONE;
}

lwcanerr_t canraw_send(struct canraw_pcb *pcb, struct lwcan_frame *frame)
{
    struct canif *canif;

    if (pcb == NULL || frame == NULL)
    {
        return ERROR_ARG;
    }

    canif = canif_get_by_index(pcb->canif_index);

    if (canif == NULL)
    {
        return ERROR_CANIF;
    }

    memcpy(&pcb->frame, frame, sizeof(struct lwcan_frame));

    return canif->output(canif, &pcb->frame);
}

lwcanerr_t canraw_set_receive_callback(struct canraw_pcb *pcb, canraw_receive_function receive)
{
    if (pcb == NULL)
    {
        return ERROR_ARG;
    }

    pcb->receive = receive;

    return ERROR_OK;
}

lwcanerr_t canraw_set_callback_arg(struct canraw_pcb *pcb, void *arg)
{
    if (pcb == NULL)
    {
        return ERROR_ARG;
    }

    pcb->callback_arg = arg;

    return ERROR_OK;
}

#endif