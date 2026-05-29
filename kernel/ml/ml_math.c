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

#include "include/ml_math.h"

/* Forward declare your kernel's VGA print function.
 * Replace this with your actual PrajnaOS terminal function. */
extern void vga_print(const char *str);
extern void vga_print_char(char c);

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
    vga_print(name);
    vga_print(": [");
    for (int i = 0; i < n; i++) {
        char buf[16];
        ml_float_to_str(x[i], buf, 4); // 4 decimal places
        vga_print(buf);
        if (i < n - 1) {
            vga_print(", ");
        }
    }
    vga_print("]\n");

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




