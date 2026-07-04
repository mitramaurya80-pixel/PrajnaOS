#include "include/klib.h"
#include "include/types.h"
#include "include/pit.h"

/* ── uint32 to decimal string ── */
void uint_to_str(uint32_t n, char *buf) {
    int i = 0;
    if (n == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    uint32_t tmp = n;
    while (tmp > 0) { buf[i++] = '0' + (tmp % 10); tmp /= 10; }
    /* reverse digits */
    int start = 0, end = i - 1;
    while (start < end) {
        char sw = buf[start]; buf[start] = buf[end]; buf[end] = sw;
        start++; end--;
    }
    buf[i] = '\0';
}

void wait(uint32_t ticks) {
    uint32_t tick_=ticks*100; //convert to 100Hz
    uint32_t start = pit_get_ticks();
    while ((pit_get_ticks() - start) < tick_) { /* busy wait */ }
}

/* ── signed int32 to decimal string ── */
void int_to_str(int32_t n, char *buf) {
    if (n < 0) { buf[0] = '-'; uint_to_str((uint32_t)(-n), buf + 1); }
    else uint_to_str((uint32_t)n, buf);
}

/* ── compare two strings — returns 0 if equal ── */
int kstrcmp(const char *a, const char *b) {
    while (*b) {
        if (*a != *b) return 1;
        a++; b++;
    }
    return !(*a == ' ' || *a == '\0');
}

/* ── string length ── */
int kstrlen(const char *s) {
    int i = 0;
    while (s[i]) i++;
    return i;
}

/* ── copy string from src to dst ── */
void kstrcpy(char *dst, const char *src) {
    int i = 0;
    while (src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

/* ── append src to end of dst ── */
void kstrcat(char *dst, const char *src) {
    int i = kstrlen(dst);
    int j = 0;
    while (src[j]) { dst[i++] = src[j++]; }
    dst[i] = '\0';
}

/* ── fill n bytes of memory with val ── */
void kmemset(void *dst, uint8_t val, uint32_t n) {
    uint8_t *d = (uint8_t *)dst;
    for (uint32_t i = 0; i < n; i++) d[i] = val;
}

/* ── copy n bytes from src to dst ── */
void kmemcpy(void *dst, const void *src, uint32_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    for (uint32_t i = 0; i < n; i++) d[i] = s[i];
}
