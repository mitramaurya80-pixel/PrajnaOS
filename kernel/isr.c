#include "include/types.h"
#include "include/shell.h"

/* ── Level 3: input buffer ── */
#define BUF_SIZE 256
static char input_buf[BUF_SIZE];   /* stores typed characters */
static int  buf_pos = 0;           /* current position in buffer */


static uint8_t inb(uint16_t port) { // read a byte from an I/O port
    uint8_t val; // the "a" constraint means to use the EAX register for output
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port)); // "Nd" means the port can be an immediate value (0-255) or in DX register
    return val; // return the value read from the port
}

static uint16_t *vga = (uint16_t *)0xB8000; // VGA text mode buffer starts at physical address 0xB8000
static int col = 10; // current column position for text output
static int row = 2; // current row position for text output (start a few lines down for better visibility)

/* tracks whether shift is currently held down */
static int shift_held = 0; // 0 = no shift, 1 = shift held

/* scancodes for shift keys */
#define LSHIFT_PRESS   0x2A // left shift key press scancode
#define RSHIFT_PRESS   0x36 // right shift key press scancode
#define LSHIFT_RELEASE 0xAA  /* 0x2A + 0x80 */ // left shift key release scancode (press scancode + 0x80)
#define RSHIFT_RELEASE 0xB6  /* 0x36 + 0x80 */ // right shift key release scancode (press scancode + 0x80)

/* normal keys — no shift */
static char keys_normal[] = { // scancode to ASCII mapping for normal keys
    0,    0,   '1', '2', '3', '4', '5', '6',
   '7',  '8', '9', '0', '-', '=', '\b', '\t',
   'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',
   'o',  'p', '[', ']', '\n',  0,  'a', 's',
   'd',  'f', 'g', 'h', 'j', 'k', 'l', ';',
  '\'',  '`',  0, '\\', 'z', 'x', 'c', 'v',
   'b',  'n', 'm', ',', '.', '/',  0,   0,
    0,   ' '
};

/* shifted keys — with shift held */
static char keys_shift[] = {
    0,    0,   '!', '@', '#', '$', '%', '^',
   '&',  '*', '(', ')', '_', '+', '\b', '\t',
   'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
   'O',  'P', '{', '}', '\n',  0,  'A', 'S',
   'D',  'F', 'G', 'H', 'J', 'K', 'L', ':',
   '"',  '~',  0,  '|', 'Z', 'X', 'C', 'V',
   'B',  'N', 'M', '<', '>', '?',  0,   0,
    0,   ' '
};

static int cursor_visible =0; // 0=hidden, 1=visible

void blink() {
    if (cursor_visible) {
    vga[row * 80 + col] = (0x02 << 8) | '_';  // show cursor
} else {
    vga[row * 80 + col] = (0x07 << 8) | ' ';  // hide cursor
}
cursor_visible = !cursor_visible;
}

 void put_char(char c ,char color) { // print a single character to the screen at current position  
    if(color == 0) color = 0x07; // default to light gray on black if color is 0

    if (c == '\n') {
        row++;
        col = 0;

        return;
    }
    if (c == '\t') {
        /* tab = jump to next multiple of 4 columns */
        int next = (col + 4) & ~3;  /* round up to next multiple of 4 */
        /* fill spaces up to the tab stop */
        while (col < next && col < 80)
            vga[row * 80 + col++] = (color << 8) | ' ';
        return;
    }
    if (c == '\b') {
        if (col > 0) {
            col--;
            vga[row * 80 + col] = (0x07 << 8) | ' ';
        }
        return;
    }
    vga[row * 80 + col] = (color << 8) | (uint8_t)c;
    col++;
    if (col >= 80) { col = 0; row++; }
    if (row >= 25) { row = 2; }
}

void keyboard_handler() {
    uint8_t scancode = inb(0x60);

    /* check for shift press — do NOT ignore, track it */
    if (scancode == LSHIFT_PRESS || scancode == RSHIFT_PRESS) {
        shift_held = 1;
        return;
    }
    /* check for shift release */
    if (scancode == LSHIFT_RELEASE || scancode == RSHIFT_RELEASE) {
        shift_held = 0;
        return;
    }

    /* ignore all other key release events (bit 7 set) */
    if (scancode & 0x80) return;

    /* pick the right key map based on shift state */
    char *keys = shift_held ? keys_shift : keys_normal;
    int table_size = shift_held ? sizeof(keys_shift) : sizeof(keys_normal);

    
/* ── Level 3: input buffer ── */
     if (scancode < table_size && keys[scancode] != 0) {
        char c = keys[scancode];

        if (c == '\b') {
            /* backspace — remove last char from buffer */
            if (buf_pos > 0) {
                buf_pos--;               /* shrink buffer */
                input_buf[buf_pos] = 0;  /* clear last char */
                put_char('\b', 0x07);          /* erase from screen */
            }

        } else if (c == '\n') {
            /* enter pressed — null terminate and handle command */
            input_buf[buf_pos] = '\0';  /* end the string */
            put_char('\n', 0x07);             /* move to next line */
            shell_handle(input_buf);    /* send to shell — next part */
            buf_pos = 0;               /* reset buffer */

        } else {
            /* normal character — add to buffer and print */
            if (buf_pos < BUF_SIZE - 1) {
                input_buf[buf_pos++] = c;  /* store in buffer */
                put_char(c, 0x07);               /* print on screen */
            }
        }
    }
}
