#include "lwcan/options.h"

#if LWCAN_RAW /* don't build if not configured for use in lwcan_options.h */

#include "lwcan/raw.h"
#include "lwcan/private/raw_private.h"
#include "lwcan/timeouts.h"
#include "lwcan/debug.h"

#include <string.h>

#define CANRAW_MEM_CHUNK_SIZE sizeof(struct canraw_pcb)

#define CANRAW_MEM_POOL_SIZE (CANRAW_MEM_CHUNK_SIZE * CANRAW_MAX_PCB_NUM)

#define CANRAW_MEM_POOL_BEGIN_ADDR (uint8_t *)(&canraw_mem_pool[0])

#define CANRAW_MEM_POOL_END_ADDR (uint8_t *)(&canraw_mem_pool[CANRAW_MEM_POOL_SIZE - 1])

#define CANRAW_MEM_POOL_SERVICE_BEGIN_IDX CANRAW_MEM_POOL_SIZE

#define CANRAW_MEM_POOL_SERVICE_END_IDX ((CANRAW_MEM_POOL_SIZE + CANRAW_MAX_PCB_NUM) - 1)

static uint8_t canraw_mem_pool[CANRAW_MEM_POOL_SIZE + CANRAW_MAX_PCB_NUM];

static struct canraw_pcb *canraw_pcb_list;

static uint8_t canraw_pcb_num;

static void *canraw_pcb_malloc(void)
{
    for (uint16_t i = CANRAW_MEM_POOL_SERVICE_BEGIN_IDX; i <= CANRAW_MEM_POOL_SERVICE_END_IDX; i++)
    {
        if (canraw_mem_pool[i] == 0)
        {
            canraw_mem_pool[i] = 0xAA;

            return (void *)&canraw_mem_pool[(i - CANRAW_MEM_POOL_SERVICE_BEGIN_IDX)  * CANRAW_MEM_CHUNK_SIZE];
        }
    }

    return NULL;
}

static void canraw_pcb_free(void *mem)
{
    uint16_t idx;

    uint8_t addr;

    /* Checking an address for inclusion in a memory pool */
    if (mem == NULL || (uint8_t *)mem < CANRAW_MEM_POOL_BEGIN_ADDR || (uint8_t *)mem > CANRAW_MEM_POOL_END_ADDR)
    {
        return;
    }

    /* get address within memory pool */
    addr = (uint8_t *)mem - CANRAW_MEM_POOL_BEGIN_ADDR;

    /* check address for multiple of chunk size */
    if ((addr % CANRAW_MEM_CHUNK_SIZE) != 0)
    {
        return;
    }

    /* get the index of the element which means that the chunk has been allocated */
    idx = CANRAW_MEM_POOL_SERVICE_BEGIN_IDX + (addr / CANRAW_MEM_CHUNK_SIZE);

    /* freeing the chunk */
    canraw_mem_pool[idx] = 0;
}

void canraw_init(void)
{
    memset(canraw_mem_pool, 0, sizeof(canraw_mem_pool));

    canraw_pcb_list = NULL;

    canraw_pcb_num = 0;
}

struct canraw_pcb *canraw_new(void)
{
    struct canraw_pcb *pcb;

    if (canraw_pcb_num >= CANRAW_MAX_PCB_NUM)
    {
        LWCAN_ASSERT("canraw_pcb_num < CANRAW_MAX_PCB_NUM", canraw_pcb_num < CANRAW_MAX_PCB_NUM);

        return NULL;
    }

    pcb = (struct canraw_pcb *)canraw_pcb_malloc();

    if (pcb == NULL)
    {
        LWCAN_ASSERT("pcb != NULL", pcb != NULL);

        return NULL;
    }

    memset(pcb, 0, sizeof(struct canraw_pcb));

    pcb->next = canraw_pcb_list;

    canraw_pcb_list = pcb;

    canraw_pcb_num += 1;

    return pcb;

}

lwcanerr_t canraw_bind(struct canraw_pcb *pcb, struct addr_can *addr)
{
    if (pcb == NULL || addr == NULL)
    {
        LWCAN_ASSERT("pcb != NULL", pcb != NULL);
        LWCAN_ASSERT("addr != NULL", addr != NULL);

        return ERROR_ARG;
    }

    if (addr->can_ifindex == 0)
    {
        LWCAN_ASSERT("addr->can_ifindex != 0", addr->can_ifindex != 0);

        return ERROR_CANIF;
    }

    pcb->if_index = addr->can_ifindex;

    return ERROR_OK;
}

lwcanerr_t canraw_remove(struct canraw_pcb *pcb)
{
    struct canraw_pcb *pcb_temp;

    lwcanerr_t ret;

    if (pcb == NULL)
    {
        LWCAN_ASSERT("pcb != NULL", pcb != NULL);

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

canraw_input_state_t canraw_input(struct canif *canif, void *frame)
{
    struct canraw_pcb *pcb;

    struct canraw_pcb *prev;

    uint8_t if_index;

    if_index = canif_get_index(canif);

    pcb = canraw_pcb_list;

    prev = NULL;

    while (pcb != NULL)
    {
        if (pcb->if_index != if_index)
        {
            prev = pcb;

            pcb = pcb->next;
        }
        else
        {
            if (pcb->receive == NULL || pcb->receive(pcb->callback_arg, pcb, frame) == 0)
            {
                break;
            }

            if (prev != NULL)
            {
                /* move the pcb to the front of canraw_pcb_list so that is found faster next time */
                prev->next = pcb->next;

                pcb->next = canraw_pcb_list;

                canraw_pcb_list = pcb;
            }

            return RAW_INPUT_EATEN;
        }
    }

    return RAW_INPUT_NONE;
}

lwcanerr_t canraw_send(struct canraw_pcb *pcb, void *frame, uint8_t frame_size)
{
    struct canif *canif;

    if (pcb == NULL || frame == NULL)
    {
        LWCAN_ASSERT("pcb != NULL", pcb != NULL);
        LWCAN_ASSERT("frame != NULL", frame != NULL);

        return ERROR_ARG;
    }

    if (frame_size != sizeof(struct can_frame) && frame_size != sizeof(struct canfd_frame))
    {
        LWCAN_ASSERT("frame_size == sizeof(struct can_frame)", frame_size == sizeof(struct can_frame));
        LWCAN_ASSERT("frame_size == sizeof(struct canfd_frame)", frame_size == sizeof(struct canfd_frame));

        return ERROR_ARG;
    }

    canif = canif_get_by_index(pcb->if_index);

    if (canif == NULL)
    {
        return ERROR_CANIF;
    }

    return canif->output(canif, frame, frame_size);
}

lwcanerr_t canraw_set_receive_callback(struct canraw_pcb *pcb, canraw_receive_function receive)
{
    if (pcb == NULL)
    {
        LWCAN_ASSERT("pcb != NULL", pcb != NULL);

        return ERROR_ARG;
    }

    pcb->receive = receive;

    return ERROR_OK;
}

lwcanerr_t canraw_set_callback_arg(struct canraw_pcb *pcb, void *arg)
{
    if (pcb == NULL)
    {
        LWCAN_ASSERT("pcb != NULL", pcb != NULL);

        return ERROR_ARG;
    }

    pcb->callback_arg = arg;

    return ERROR_OK;
}

#endif
