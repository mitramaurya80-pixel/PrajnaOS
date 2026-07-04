#ifndef ML_WEIGHTS_H
#define ML_WEIGHTS_H

#include "types.h"

/* weight file header — matches Python export format */
#define ML_MAGIC 0x504A4E4E

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t n_layers;
} __attribute__((packed)) ML_Header;

typedef struct {
    uint32_t input_size;
    uint32_t output_size;
} __attribute__((packed)) ML_LayerHeader;

/* network dimensions — matches your Python export */
#define L1_IN  4
#define L1_OUT 8
#define L2_IN  8
#define L2_OUT 4
#define L3_IN  4
#define L3_OUT 1

/* global weight arrays — filled by ml_weights_load() */
extern float weights_l1[L1_OUT][L1_IN];
extern float bias_l1[L1_OUT];
extern float weights_l2[L2_OUT][L2_IN];
extern float bias_l2[L2_OUT];
extern float weights_l3[L3_OUT][L3_IN];
extern float bias_l3[L3_OUT];

/* load weights from disk into global arrays */
uint8_t ml_weights_load(void);

#endif