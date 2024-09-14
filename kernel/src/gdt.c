#include "gdt.h"

#define GDT_N_ENTRIES 3

struct gdt_descriptor gdt_descriptor_instance = {0};
uint64_t gdt_entries[GDT_N_ENTRIES] = {0};

uint64_t gdt_create_descriptor(uint32_t base, uint32_t limit, uint16_t flag){
    uint64_t descriptor;
 
    // Create the high 32 bit segment
    descriptor  =  limit       & 0x000F0000;         // set limit bits 19:16
    descriptor |= (flag <<  8) & 0x00F0FF00;         // set type, p, dpl, s, g, d/b, l and avl fields
    descriptor |= (base >> 16) & 0x000000FF;         // set base bits 23:16
    descriptor |=  base        & 0xFF000000;         // set base bits 31:24
 
    // Shift by 32 to allow for low part of segment
    descriptor <<= 32;
 
    // Create the low 32 bit segment
    descriptor |= base  << 16;                       // set base bits 15:0
    descriptor |= limit  & 0x0000FFFF;               // set limit bits 15:0
 
    return descriptor;
}

/*
    Just a flat memory model.
    Would probably be "fine" to keep using limines GDT but
    having control is better.
*/
void gdt_setup(){
    /*
        Null descriptor
    */
    gdt_entries[0] = gdt_create_descriptor(0, 0, 0);

    /*
        Kernel code segment
    */
    gdt_entries[1] = gdt_create_descriptor(0, 0x000FFFFF, (GDT_CODE_PL0));
    gdt_entries[2] = gdt_create_descriptor(0, 0x000FFFFF, (GDT_DATA_PL0));

    gdt_descriptor_instance.virt_addr = (uint64_t)&gdt_entries;
    gdt_descriptor_instance.size = GDT_N_ENTRIES * sizeof(uint64_t);

    asm volatile ("lgdt %0" :: "m"(gdt_descriptor_instance));
}
