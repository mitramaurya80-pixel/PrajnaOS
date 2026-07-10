#ifndef ISCR_H
#define ISCR_H

void blink();
uint8_t scancode_pop(uint8_t *scancode);
void process_scancode(uint8_t scancode);

#endif
