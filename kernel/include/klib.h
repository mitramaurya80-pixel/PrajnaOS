#ifndef KLIB_H
#define KLIB_H

#include "types.h"

/* ── string utilities ── */
void     uint_to_str(uint32_t n, char *buf);        /* number → string */
void     int_to_str(int32_t n, char *buf);          /* signed number → string */
int      kstrcmp(const char *a, const char *b);     /* compare two strings */
int      kstrlen(const char *s);                    /* string length */
void     kstrcpy(char *dst, const char *src);       /* copy string */
void     kstrcat(char *dst, const char *src);       /* append string */
void     kmemset(void *dst, uint8_t val, uint32_t n); /* fill memory */
void     kmemcpy(void *dst, const void *src, uint32_t n); /* copy memory */
void    wait(uint32_t ticks);                       /* busy wait for ticks */

#endif