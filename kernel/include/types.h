#ifndef TYPES_H
#define TYPES_H

/* unsigned = only positive numbers, no negative */
typedef unsigned char      uint8_t;   /* 1 byte  = 0 to 255          */
typedef unsigned short     uint16_t;  /* 2 bytes = 0 to 65535        */
typedef unsigned int       uint32_t;  /* 4 bytes = 0 to 4,294,967,295*/
typedef unsigned long long uint64_t;  /* 8 bytes = very big number   */

/* signed = can be positive or negative */
typedef signed char        int8_t;    /* 1 byte  = -128 to 127       */
typedef signed short       int16_t;   /* 2 bytes = -32768 to 32767   */
typedef signed int         int32_t;   /* 4 bytes = -2billion to 2billion */
typedef signed long long   int64_t;   /* 8 bytes = very big +/-      */

/* NULL means "pointer to nothing" */
#define NULL ((void*)0)

#endif
