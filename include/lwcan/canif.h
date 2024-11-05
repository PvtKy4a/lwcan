#ifndef LWCAN_CANIF_H
#define LWCAN_CANIF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwcan/error.h"
#include "lwcan/frame.h"

#include <stdint.h>

struct canif;

typedef lwcanerr_t (*canif_input_function)(struct canif *canif, struct lwcan_frame *frame);

typedef lwcanerr_t (*canif_output_function)(struct canif *canif, struct lwcan_frame *frame);

typedef lwcanerr_t (*canif_init_function)(struct canif *canif);

struct canif
{
    struct canif *next;

    uint8_t num;

    void *driver_data;

    char name[4];

    canif_input_function input;

    canif_output_function output;
};

lwcanerr_t canif_add(struct canif *canif, const char *name, canif_init_function init, canif_input_function input);

lwcanerr_t canif_remove(struct canif *canif);

lwcanerr_t canif_input(struct canif *canif, struct lwcan_frame *frame);

lwcanerr_t canif_get_name(struct canif *canif, char *name);

struct canif *canif_get_by_name(const char *name);

uint8_t canif_name_to_index(const char *name);

lwcanerr_t canif_index_to_name(uint8_t index, char *name);

uint8_t canif_get_index(struct canif *canif);

struct canif *canif_get_by_index(uint8_t index);

#ifdef __cplusplus
}
#endif

#endif
