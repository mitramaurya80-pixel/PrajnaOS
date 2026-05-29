/*
 * ml_math.h — PrajnaOS Level 8 Math Library
 * Neural network math — no stdlib, no math.h
 * Pure C, bare metal kernel safe
 *
 * Author  : Ravi
 * Project : PrajnaOS ML Inference Engine
 * Level   : L8
 */

#ifndef ML_MATH_H
#define ML_MATH_H

#include "types.h"

/* -------------------------------------------------------
 * Scalar operations
 * ------------------------------------------------------- */

/*
 * ml_sigmoid — sigmoid activation without math.h
 * Uses Pade approximant: accurate to ~1% across [-6, 6]
 * For bare metal: avoids exp(), works entirely in float
 *
 *   sigma(x) = 1 / (1 + e^-x)
 *   approx  = 0.5 + x*(0.25 - x*x*0.020833f) for |x| <= 2.4
 *   clamped to [0,1] outside that range
 */
float ml_sigmoid(float x);

/*
 * ml_relu — rectified linear unit (in-place on array)
 * Sets negative elements to 0.
 * x : pointer to float array
 * n : number of elements
 */
void ml_relu(float *x, int n);

/*
 * ml_relu_scalar — single-value ReLU
 */
float ml_relu_scalar(float x);

/*
 * ml_clamp — clamp value between lo and hi
 */
float ml_clamp(float x, float lo, float hi);

/*
 * ml_normalize — scale a value from [min, max] to [0.0, 1.0]
 * Safe: returns 0.5 if max == min (avoids divide-by-zero)
 */
float ml_normalize(float val, float min, float max);

/*
 * ml_abs — absolute value without <math.h>
 */
float ml_abs(float x);

/* -------------------------------------------------------
 * Vector operations
 * ------------------------------------------------------- */

/*
 * ml_dot — dot product of two float vectors
 * Returns sum of a[i] * b[i] for i in [0, n)
 */
float ml_dot(const float *a, const float *b, int n);

/*
 * ml_add_bias — add bias vector to output vector (in-place)
 * x[i] += bias[i] for i in [0, n)
 */
void ml_add_bias(float *x, const float *bias, int n);

/*
 * ml_vec_copy — copy src into dst
 */
void ml_vec_copy(float *dst, const float *src, int n);

/*
 * ml_vec_zero — zero-fill a float array
 */
void ml_vec_zero(float *x, int n);

/* -------------------------------------------------------
 * Matrix operations
 * Row-major storage: A[i][j] = A[i*cols + j]
 * ------------------------------------------------------- */

/*
 * ml_multiply — matrix-vector multiply: C = A * B
 *
 *   A is (out_rows x in_cols)   [weights matrix]
 *   B is (in_cols x 1)          [input vector]
 *   C is (out_rows x 1)         [output vector]
 *
 * This is the core of every layer's forward pass.
 *
 * Example — layer 1: input(4) -> hidden(8)
 *   ml_multiply(weights_l1, input, layer1_out, 8, 4)
 *   A is 8x4 = 32 floats
 *   B is 4 floats
 *   C is 8 floats (result)
 */
void ml_multiply(const float *A, const float *B, float *C, int out_rows, int in_cols);

/* -------------------------------------------------------
 * Debug / diagnostics (safe to call from kernel shell)
 * ------------------------------------------------------- */

/*
 * ml_print_vec — prints a float vector via VGA terminal
 * name : label string (printed before values)
 * x    : float array
 * n    : length
 */
void ml_print_vec(const char *name, const float *x, int n);

/*
 * ml_float_to_str — convert float to string, no sprintf
 * buf must be at least 16 bytes
 * precision : digits after decimal point (max 6)
 */
void ml_float_to_str(float val, char *buf, int precision);

#endif /* ML_MATH_H */