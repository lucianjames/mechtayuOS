#ifndef KTERMINAL_H
#define KTERMINAL_H

#include <stdarg.h>

#include "limine.h"
#include "graphics.h"

void kterm_init(const struct limine_framebuffer* framebuffer);
int kterm_printuint(int col, uint32_t uint_to_write, int base);
void kterm_printf_newline(const char* fmt, ...);
void kterm_write_newline(const char* str);
void kterm_clear();

#endif