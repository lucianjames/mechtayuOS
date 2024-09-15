#ifndef GDT_H
#define GDT_H

#include <stdint.h>

struct gdt_entry_struct{
    uint16_t limit_low; // lim 0-15
    uint16_t base_low; // base 0-15
    uint8_t  base_middle; // base 16-23
    uint8_t  access; // Access flags
    uint8_t  granularity; // granularity flags + high lim bits 15-19
    uint8_t  base_high; // base 24-31
}__attribute__((packed));
typedef struct gdt_entry_struct gdt_entry_t;

gdt_entry_t gdt_create_entry(uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity);
void gdt_setup();
extern void gdt_set_gdt(uint16_t limit, uintptr_t base);
//extern void gdt_reload_segments();

#endif
