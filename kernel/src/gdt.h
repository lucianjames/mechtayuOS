#ifndef GDT_H
#define GDT_H

#include <stdint.h>

struct gdt_descriptor{
    uint64_t virt_addr;
    uint16_t size;
}__attribute__((packed));

void gdt_setup();

#endif
