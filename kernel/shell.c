#include "include/types.h"
#include "include/shell.h"
#include "include/pit.h"
#include "include/fat32.h"
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
    else if (kstrcmp(cmd, "ls") == 0) {
        fat32_list_dir();
    }else if (kstrcmp(cmd, "cat") == 0) {
    /* cat filename.ext — print file contents */
    /* cmd format: "cat TEST.TXT" */
    if (cmd[3] != ' ') {
        print("Usage: cat FILENAME.EXT", 0x0C);
    }else {
        /* parse filename and extension from cmd+4 */
        char name[9] = "        ";  /* 8 spaces */
        char ext[4]  = "   ";       /* 3 spaces */
        char *arg = cmd + 4;        /* skip "cat " */
        int j = 0;

        /* copy name — up to 8 chars or dot */
        while (arg[j] && arg[j] != '.' && j < 8) {
            name[j] = arg[j];
            j++;
        }
        name[j] = '\0';

        /* skip dot */
        if (arg[j] == '.') j++;

        /* copy extension — up to 3 chars */
        int e = 0;
        while (arg[j] && e < 3) {
            ext[e++] = arg[j++];
        }
        ext[e] = '\0';

        /* find and read file */
        FAT32_Entry entry;
        uint32_t dir_sector, dir_offset;
        if (fat32_find_file(name, ext, &entry, &dir_sector, &dir_offset) == 0) {
            uint8_t file_buf[512];
            fat32_read_file(&entry, file_buf, 512);
            /* print file contents */
            int k;
            for (k = 0; k < (int)entry.file_size && k < 512; k++)
                put_char(file_buf[k], 0x0F);
            put_char('\n', 0x0F);
        } else {
            print("File not found", 0x0C);
        }
    }
} else if (kstrcmp(cmd, "cd") == 0) {
    if (cmd[2] != ' ') {
        print("Usage: cd FOLDERNAME", 0x0C);
    } else {
        char *arg = cmd + 3;

        /* cd .. — go back */
        if (arg[0] == '.' && arg[1] == '.') {
            current_cluster  = previous_cluster;
            previous_cluster = root_cluster;
            print("OK", 0x0A);
        } else {
            FAT32_Entry entry;
            if (fat32_find_dir(arg, &entry) == 0) {
                previous_cluster = current_cluster;  /* save before changing */
                current_cluster  = ((uint32_t)entry.cluster_high << 16)
                                   | entry.cluster_low;
                if (current_cluster == 0)
                    current_cluster = root_cluster;
                print("OK", 0x0A);
            } else {
                print("Directory not found", 0x0C);
            }
        }
    }
}else if (kstrcmp(cmd, "touch") == 0) {
    if (cmd[5] != ' ') {
        print("Usage: touch FILENAME.EXT", 0x0C);
    } else {
        char name[9] = "        ";
        char ext[4]  = "   ";
        char *arg = cmd + 6;
        int j = 0;

        while (arg[j] && arg[j] != '.' && j < 8) {
            name[j] = arg[j]; j++;
        }
        name[j] = '\0';
        if (arg[j] == '.') j++;
        int e = 0;
        while (arg[j] && e < 3) { ext[e++] = arg[j++]; }
        ext[e] = '\0';

        uint8_t res = fat32_create_file(name, ext);
        if (res == 0)
            print("File created", 0x0A);
        else
            print("Failed to create file", 0x0C);
    }
} else if (kstrcmp(cmd, "write") == 0) {
    /* usage: write FILENAME.EXT text content here */
    if (cmd[5] != ' ') {
        print("Usage: write FILENAME.EXT text", 0x0C);
    } else {
        char name[9] = "        ";
        char ext[4]  = "   ";
        char *arg = cmd + 6;  /* skip "write " */
        int j = 0;

        /* parse filename */
        while (arg[j] && arg[j] != '.' && j < 8) {
            name[j] = arg[j]; j++;
        }
        name[j] = '\0';
        if (arg[j] == '.') j++;

        /* parse extension */
        int e = 0;
        while (arg[j] && arg[j] != ' ' && e < 3) {
            ext[e++] = arg[j++];
        }
        ext[e] = '\0';

        /* skip space after filename */
        if (arg[j] == ' ') j++;

        /* rest is content */
        char *content = arg + j;
        uint32_t content_len = 0;
        while (content[content_len]) content_len++;

        /* find file */
        FAT32_Entry entry;
        uint32_t dir_sector, dir_offset;
        if (fat32_find_file(name, ext, &entry, &dir_sector, &dir_offset) == 0) {
            uint8_t res = fat32_write_file(&entry, dir_sector, dir_offset,
                                           (uint8_t*)content, content_len);
            if (res == 0)
                print("Written", 0x0A);
            else
                print("Write failed", 0x0C);
        } else {
            print("File not found", 0x0C);
        }
    }
}else {
        
        print("Unknown command. Type help.", 0x04);
        
    }

    /* print prompt after every command */
    print_prompt("PrajnaOS>", 0x03);
    
}