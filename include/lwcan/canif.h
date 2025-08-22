#ifndef LWCAN_CANIF_H
#define LWCAN_CANIF_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lwcan/error.h"
#include "lwcan/can.h"

#include <stdint.h>

struct canif;

typedef void (*canif_sent_function)(void *arg, lwcanerr_t error);

typedef lwcanerr_t (*canif_input_function)(struct canif *canif, void *frame);

typedef lwcanerr_t (*canif_output_function)(struct canif *canif, void *frame, uint8_t frame_size, uint32_t timeout, canif_sent_function sent, void *arg);

typedef lwcanerr_t (*canif_set_bitrate_function)(struct canif *canif, uint32_t bitrate);

typedef lwcanerr_t (*canif_set_filter_function)(struct canif *canif, struct can_filter *filter);

typedef lwcanerr_t (*canif_init_function)(struct canif *canif);

struct canif
{
    struct canif *next;

    uint8_t num;

    void *driver_data;

    char name[4];

    canif_input_function input;

    canif_output_function output;

    canif_set_bitrate_function set_bitrate;

    canif_set_filter_function set_filter;
};

lwcanerr_t canif_add(struct canif *canif, const char *name, canif_init_function init);

lwcanerr_t canif_remove(struct canif *canif);

lwcanerr_t canif_set_bitrate(struct canif *canif, uint32_t bitrate);

lwcanerr_t canif_set_filter(struct canif *canif, struct can_filter *filter);

const char *canif_get_name(struct canif *canif);

struct canif *canif_get_by_name(const char *name);

uint8_t canif_name_to_index(const char *name);

const char *canif_index_to_name(uint8_t index);

uint8_t canif_get_index(struct canif *canif);

struct canif *canif_get_by_index(uint8_t index);

#ifdef __cplusplus
}
#endif

#endif
