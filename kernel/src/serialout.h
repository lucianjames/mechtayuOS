#ifndef DEBUGSERIAL_H
#define DEBUGSERIAL_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#define SERIAL_DEBUG_COM_PORT   0x3f8 // COM1
#define SERIAL_TEST_BYTE        0x69
#define SERIAL_LOOPBACK_MODE    0x1E
#define SERIAL_NORMAL_MODE      0x0F

uint8_t asm_inline_inb(uint16_t port);
void asm_inline_outb(uint16_t port, uint8_t data);

int init_debug_serial();
int is_debug_serial_received();
char read_debug_serial();
int is_debug_transmit_empty();
void write_debug_serial(char a);
void writestr_debug_serial(const char* str);
void writeuint_debug_serial(uint64_t uint_to_write, int base);

#endif