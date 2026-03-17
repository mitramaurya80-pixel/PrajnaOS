#define VGA_ADDRESS 0xB8000

void kernel_main() {
    char *vga = (char *)VGA_ADDRESS;
    const char *msg = "Hello from my OS!";
    int i = 0;

    while (msg[i]) {
        vga[i * 2]     = msg[i];   // character
        vga[i * 2 + 1] = 0x07;    // white on black
        i++;
    }
}