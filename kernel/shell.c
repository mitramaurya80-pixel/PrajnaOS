#include "include/types.h"
#include "include/shell.h"
#include "include/pit.h"
/* ── our own strcmp — no stdlib in PrajnaOS ── */
static int kstrcmp(char *a, char *b) {
    while (*b) {        /* while both strings have chars */
        if (*a != *b)         /* if chars don't match */
            return 1;         /* not equal */
        a++; b++;             /* move to next char */
    }
    return !(*a== ' ' || *a == '\0');        /* equal only if both ended */
}
static void print_prompt(char *s, char color) {
    while (*s) // loop until null terminator
        put_char(*s++, color); // print each character with the specified color
    /* no \n here */
}

/* ── print string to screen ── */
extern void put_char(char c ,char color);  /* already in isr.c */

static void print(char *s ,char color) {
    while (*s)                 /* loop until null terminator */
        put_char(*s++, color);        /* print each character */
    put_char('\n', color);            /* newline at end */
}

/* ── command handler ── */
void shell_handle(char *cmd) {

    if (kstrcmp(cmd, "help") == 0) {
        print("Commands: help, clear, about, uptime", 0x0E);

    } else if (kstrcmp(cmd, "about") == 0) {
        print("PrajnaOS - Consciousness. Intelligence. Control.", 0x01);
        print("Built from scratch in C and x86 Assembly.", 0x01);

    } else if (cmd[0] == '\0') {
        /* empty command — just show prompt */

    } else if (kstrcmp(cmd, "clear") == 0) {
        /* clear screen by printing newlines */
        extern void clear_screen();  /* implemented in isr.c */
        clear_screen();
        
    }else if (kstrcmp(cmd, "uptime") == 0) {
    /* print tick count as simple number */
    uint32_t t = pit_get_ticks();
    char buf[32];
    /* convert number to string manually */
    int i = 0;
    if (t == 0) { buf[i++] = '0'; } // handle zero case
    else {
        uint32_t tmp = t;
        int start = i;
        while (tmp > 0) {
            buf[i++] = '0' + (tmp % 10);
            tmp /= 10;
        }
        /* reverse the digits */
        int end = i - 1;
        while (start < end) {
            char swap = buf[start];
            buf[start] = buf[end];
            buf[end] = swap;
            start++; end--;
        }
    buf[i] = '\0';
    print("Ticks: ",0x0E);
    print(buf, 0x0E);
    }
    }else if (kstrcmp(cmd, "echo") == 0)
    {
        if (cmd[4] == ' ') {
            print(cmd + 5, 0x0A); // print text after "echo "
        } else {
            print("Usage: echo <text>", 0x0C);
        }
    }else if (kstrcmp(cmd, "version") == 0) {
        print("PrajnaOS v0.1 - Bare Metal", 0x0E);
    }else if (kstrcmp(cmd, "hello") == 0) {
        print("Hello from PrajnaOS!", 0x0A);
    }else if (kstrcmp(cmd, "beep") == 0) {
        /* make beep sound if speaker available */
        put_char('\x07', 0x07);  /* bell character */
    }else {
        print("Unknown command. Type help." , 0x04);
    }

    /* print prompt after every command */
    print_prompt("PrajnaOS>", 0x03);
    
}