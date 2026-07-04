#ifndef ML_INFER_H
#define ML_INFER_H

#include "types.h"

/*
 * ml_infer — run forward pass through neural network
 *
 * input[4] = { cpu_usage, mem_usage, wait_time, priority }
 *             all normalized to 0.0 - 1.0
 *
 * returns score 0.0 - 1.0
 * high score = high priority = run this process next
 */
float ml_infer(float *input);

#endif