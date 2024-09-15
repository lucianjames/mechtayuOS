#ifndef KTERMINAL_H
#define KTERMINAL_H

#include <stdarg.h>

#include "third-party/limine.h"
#include "graphics.h"

void kterm_init(struct limine_framebuffer* framebuffer);
int kterm_printuint(int col, uint64_t uint_to_write, int base);
void kterm_printf_newline(const char* fmt, ...);
void kterm_write_newline(const char* str);
void kterm_clear();
void kterm_set_framebuff_addr(uint64_t* framebuffer_addr);

#endif