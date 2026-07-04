#include "ml_infer.h"
#include "ml_weights.h"
#include "ml_math.h"

float ml_infer(float *input) {
    float layer1[L1_OUT];  /* hidden layer 1 output — 8 neurons */
    float layer2[L2_OUT];  /* hidden layer 2 output — 4 neurons */
    float output[L3_OUT];  /* final output — 1 neuron */

    /* layer 1: input(4) -> hidden(8) */
    ml_multiply((float*)weights_l1, input, layer1, L1_OUT, L1_IN);
    ml_add_bias(layer1, bias_l1, L1_OUT);
    ml_relu(layer1, L1_OUT);
     /* debug layer1 output */
    extern void put_char(char c, char color);
    char buf[16];
    ml_float_to_str(layer1[0], buf, 3);
    extern void vga_print(const char *s, int row, int col, uint8_t color);


    /* layer 2: hidden(8) -> hidden(4) */
    ml_multiply((float*)weights_l2, layer1, layer2, L2_OUT, L2_IN);
    ml_add_bias(layer2, bias_l2, L2_OUT);
    ml_relu(layer2, L2_OUT);

    

    /* output: hidden(4) -> score(1) */
    ml_multiply((float*)weights_l3, layer2, output, L3_OUT, L3_IN);
    ml_add_bias(output, bias_l3, L3_OUT);
    output[0] = ml_sigmoid(output[0]);

    return output[0];
}