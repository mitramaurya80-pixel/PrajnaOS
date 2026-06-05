#include "include/types.h"
#include "include/shell.h"
#include "include/pit.h"
extern void put_char(char c, char color);  /* implemented in isr.c */
/* ── our own strcmp — no stdlib in PrajnaOS ── */
static int kstrcmp(char *a, char *b) {
    while (*b) {        /* while both strings have chars */
        if (*a != *b)         /* if chars don't match */
            return 1;         /* not equal */
        a++; b++;             /* move to next char */
    }
    return !(*a== ' ' || *a == '\0');        /* equal only if both ended */
}
/* iris classifier — no stdlib, no malloc
   species: 0=setosa, 1=versicolor, 2=virginica */
int classify_iris(float sl, float sw, float pl, float pw) {
    if (pl <= 2.45f)
        return 0;  /* setosa */
    if (pw <= 1.75f) {
        if (pl <= 4.95f)
            return 1;  /* versicolor */
        return 2;      /* virginica */
    }
    return 2;          /* virginica */
}
static void outb(uint16_t port, uint8_t val) { // Output byte to port
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port)); // Output byte to port
}
static uint8_t inb(uint16_t port) { // read a byte from an I/O port
    uint8_t val; // the "a" constraint means to use the EAX register for output
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port)); // "Nd" means the port can be an immediate value (0-255) or in DX register
    return val; // return the value read from the port
}
// poweroff command fuction
void halt(){
    __asm__ volatile("cli"); /* disable interrupts */
    while (1)
    {
        __asm__ volatile("hlt") ; /* halt CPU until next interrupt */
    }
    
}
void reboot() {
    unsigned char status;
    do {
        status = inb(0x64);
    } while (status & 0x02);

    outb(0x64, 0xFE);
}
static void print_prompt(char *s, char color) {
    while (*s) // loop until null terminator
        put_char(*s++, color); // print each character with the specified color
    /* no \n here */
}

/* ── print string to screen ── */


static void print(char *s ,char color) {
    while (*s)                 /* loop until null terminator */
        put_char(*s++, color);        /* print each character */
    put_char('\n', color);            /* newline at end */
}

/* ── command handler ── */
void shell_handle(char *cmd) {
    /* debug — print first 3 chars of cmd */
    // put_char('\n', 0x0C);
    // put_char('[', 0x0C);
    // put_char(cmd[0], 0x0C);
    // put_char(cmd[1], 0x0C);
    // put_char(cmd[2], 0x0C);
    // put_char(']', 0x0C);
    // put_char('\n', 0x0C);
    

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
    }else if (kstrcmp(cmd, "poweroff") == 0) {
        halt();
    }else if (kstrcmp(cmd, "reboot") == 0) {
        reboot();
    }else if (kstrcmp(cmd, "classify") == 0) {
        /* example command to classify iris flower */
        float sl = 5.1f, sw = 3.5f, pl = 1.4f, pw = 0.2f;
    int species = classify_iris(sl, sw, pl, pw);
    char *species_str = (species == 0) ? "setosa" :
                        (species == 1) ? "versicolor" : "virginica";
    print(species_str, 0x0A);
    }   
    else  {
        
        print("Unknown command. Type help.", 0x04);
        
    }

    /* print prompt after every command */
    print_prompt("PrajnaOS>", 0x03);
    
}