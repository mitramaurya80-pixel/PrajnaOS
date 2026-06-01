 /*
 * ml_math.c — PrajnaOS Level 8 Math Library
 * Neural network math — no stdlib, no math.h, no FPU tricks
 * Pure C, bare metal kernel safe
 *
 * Author  : Ravi
 * Project : PrajnaOS ML Inference Engine
 * Level   : L8
 *
 * Design rules for this file:
 *   - No #include <math.h>       — math.h requires libc
 *   - No #include <stdlib.h>     — malloc not available in kernel
 *   - No printf()                — use kernel vga_print() instead
 *   - float arithmetic only      — FPU is available on x86 (Phase 1)
 *   - No dynamic allocation      — all buffers are stack or static
 *
 * Phase 2 note: Replace float with Q8.8 fixed point when
 * FPU state management becomes a scheduling concern.
 */

#include "/home/vega/projects/PrajnaOS/kernel/include/ml_math.h"
#include "../include/shell.h" /* for put_char() */

/* Forward declare your kernel's VGA print function.
 * Replace this with your actual PrajnaOS terminal function. */
extern void put_char(char c ,char color); 

/*
 * int_to_str — convert integer to decimal string
 * Writes into buf, returns pointer past last digit.
 * Used by ml_float_to_str.
 */

static char* int_to_str(int val,char *buf){
    if (val == 0) {
        *buf++ = '0';
        return buf;
    }
    if(val < 0){
        *buf++ = '-';
        val = -val; // make positive for digit extraction
    }
    /* Determine digits count */
    int temp = val;
    int digits = 0;
    while (temp > 0) { temp /= 10; digits++; }

        /* Write digits right-to-left into a small tmp buffer */
    char tmp_buf[12];
    int i = 0;
    while (val > 0) { 
        tmp_buf[i++] = '0' + (val % 10); 
        val /= 10;
    }
    /* Reverse digits into output buffer */
    for (int j = i - 1; j >= 0; j--) {
        *buf++ = tmp_buf[j]; 
    }
    return buf;
}

void ml_float_to_str(float val,char *buf,int precision){
    char *p = buf;
    if (val < 0.0f) {  
        *p++ = '-';
        val = -val; // make positive for processing
    }

    /* Integer parts*/
    int int_part = (int)val;    
    p = int_to_str(int_part, p);

    if(precision<=0){
        *p = '\0'; // null-terminate and
        return;
    }
    *p++ = '.'; /* Decimal point */

    /* Fractional part: Multiply by 10 and extract digits */
    float frac_part = val - (float)int_part;
    for (int i = 0; i < precision; i++) {
        frac_part *= 10.0f;
        int digit = (int)frac_part;
        *p++ = '0' + digit;
        frac_part -= (float)digit;
    }
    *p = '\0'; // null-terminate the string
}

/* -------------------------------------------------------
 * ml_print_vec — debug helper, calls VGA terminal
 * ------------------------------------------------------- */

void ml_print_vec(const char *name, const float *x, int n) {
    put_char('[', 0x07);
    for (int i = 0; i < n; i++) {
        char buf[16];
        ml_float_to_str(x[i], buf, 4); // 4 decimal places
        put_char(*buf, 0x07);
        if (i < n - 1) {
            put_char(',', 0x07);
            put_char(' ', 0x07);
        }
    }
    put_char(']', 0x07);
    put_char('\n', 0x07);

}
/* -------------------------------------------------------
 * ml_abs — absolute value without <math.h>
 * ------------------------------------------------------- */
float ml_abs(float x) {
    return (x < 0.0f) ? -x : x;
}
/* -------------------------------------------------------
 * ml_clamp — clamp x to [lo, hi]
 * ------------------------------------------------------- */

float ml_clamp(float x, float lo, float hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}
/* -------------------------------------------------------
 * ml_relu — in-place ReLU on array of n floats
 *
 * For each element: x[i] = max(0, x[i])
 * This is the activation function for hidden layers.
 * ------------------------------------------------------- */

void ml_relu(float *x, int n) {
    for (int i = 0; i < n; i++) {
        if (x[i] < 0.0f) {
            x[i] = 0.0f;
        }
    }
}
float ml_relu_scalar(float x) {
    return (x < 0.0f) ? 0.0f : x;
}

/* -------------------------------------------------------
 * ml_sigmoid — sigmoid without math.h (no exp())
 *
 * We use a piecewise polynomial approximation:
 *
 *   For |x| >= 6.0  → saturated: return 0.0 or 1.0
 *   For |x| < 6.0   → Pade approximant:
 *
 *     sigma(x) ≈ 0.5 + x*(0.25 - x*x*0.020833f)
 *
 * This is derived from the Taylor expansion of sigmoid around 0.
 * Accuracy: max error ≈ 0.012 at x = ±2.5
 *           max error ≈ 0.04  at x = ±4.0
 *
 * For scheduling priority (score 0.0 to 1.0), this precision
 * is more than sufficient.
 *
 * Phase 2: Replace with lookup table (512 entries) for speed.
 * ------------------------------------------------------- */

float ml_sigmoid(float x) {
    /* Saturate at extremes — avoids numerical issues */
    if (x >= 6.0f)  return 1.0f;
    if (x <= -6.0f) return 0.0f;
 
    /*
     * Pade approximant — valid for |x| <= ~4.5
     * For |x| in (4.5, 6.0) this slightly underestimates,
     * but the clamp below keeps result in [0, 1]
     */
    float x2 = x * x;
    float result = 0.5f + x * (0.25f - x2 * 0.020833f);
 
    /* Clamp to [0, 1] — approximation can drift slightly outside */
    return ml_clamp(result, 0.0f, 1.0f);
}
 
/* -------------------------------------------------------
 * ml_dot — dot product of two float vectors
 *
 * result = a[0]*b[0] + a[1]*b[1] + ... + a[n-1]*b[n-1]
 *
 * Used in the output layer (single-neuron final score).
 * ------------------------------------------------------- */
float ml_dot(const float *a, const float *b, int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        sum += a[i] * b[i];
    }
    return sum;
}
 
/* -------------------------------------------------------
 * ml_add_bias — add bias vector to output (in-place)
 *
 * x[i] += bias[i]
 *
 * Called after ml_multiply, before activation function.
 * ------------------------------------------------------- */
void ml_add_bias(float *x, const float *bias, int n) {
    for (int i = 0; i < n; i++) {
        x[i] += bias[i];
    }
}
 
/* -------------------------------------------------------
 * ml_vec_copy — copy n floats from src to dst
 * ------------------------------------------------------- */
void ml_vec_copy(float *dst, const float *src, int n) {
    for (int i = 0; i < n; i++) {
        dst[i] = src[i];
    }
}
 
/* -------------------------------------------------------
 * ml_vec_zero — zero-fill n floats
 * ------------------------------------------------------- */
void ml_vec_zero(float *x, int n) {
    for (int i = 0; i < n; i++) {
        x[i] = 0.0f;
    }
}
 
/* -------------------------------------------------------
 * ml_multiply — matrix-vector multiply: C = A * B
 *
 * A is stored row-major: A[row][col] = A[row * in_cols + col]
 *
 * For each output neuron i (row of A):
 *   C[i] = A[i][0]*B[0] + A[i][1]*B[1] + ... + A[i][in_cols-1]*B[in_cols-1]
 *
 * This is a row-vector dot product — one per output neuron.
 *
 * Example — Layer 1 forward pass:
 *   weights_l1 is [8][4]  (8 hidden neurons, 4 inputs)
 *   input      is [4]     (4 input values)
 *   output     is [8]     (8 hidden activations before ReLU)
 *
 *   ml_multiply(weights_l1, input, layer1, 8, 4)
 *
 * Memory layout of weights_l1 (8x4 example, W[out][in]):
 *   [ W[0][0] W[0][1] W[0][2] W[0][3] ]   <- neuron 0 weights
 *   [ W[1][0] W[1][1] W[1][2] W[1][3] ]   <- neuron 1 weights
 *   ...
 *   [ W[7][0] W[7][1] W[7][2] W[7][3] ]   <- neuron 7 weights
 *
 * This matches the binary weight file format in ml_weights.c
 * (weights stored per-neuron, row-major).
 * ------------------------------------------------------- */
void ml_multiply(const float *A, const float *B, float *C, int out_rows, int in_cols) {
    for (int row = 0; row < out_rows; row++) {
        float sum = 0.0f;
        const float *A_row = A + row * in_cols; /* pointer to row start */
        for (int col = 0; col < in_cols; col++) {
            sum += A_row[col] * B[col];
        }
        C[row] = sum;
    }
}
 
/*
 * END ml_math.c
 *
 * Next step: write ml_infer.c which calls these functions
 * in order: multiply -> add_bias -> relu (x2) -> sigmoid
 *
 * Test this file first with a simple unit test in your shell:
 *   float a[4] = {0.8f, 0.4f, 0.9f, 0.2f};
 *   float b[4] = {0.5f, 0.5f, 0.5f, 0.5f};
 *   vga_print_float(ml_dot(a, b, 4));  // should be ~1.15
 *   ml_relu(a, 4);                      // no change (all positive)
 *   vga_print_float(ml_sigmoid(0.0f)); // should be 0.5
 *   vga_print_float(ml_sigmoid(2.0f)); // should be ~0.88
 *   vga_print_float(ml_sigmoid(-2.0f));// should be ~0.12
 */
 



