#include "gdt.h"

uint64_t gdt_descriptors[6] = {0};

/*
    Just a flat memory model.
    Would probably be "fine" to keep using limines GDT but
    having control is better.
*/
void gdt_setup(){

    gdt_descriptors[0] = create_descriptor(0, 0, 0);
    gdt_descriptors[1] = create_descriptor(0, 0x000FFFFF, (GDT_CODE_PL0));
    gdt_descriptors[2] = create_descriptor(0, 0x000FFFFF, (GDT_DATA_PL0));
    gdt_descriptors[3] = create_descriptor(0, 0x000FFFFF, (GDT_CODE_PL3));
    gdt_descriptors[4] = create_descriptor(0, 0x000FFFFF, (GDT_DATA_PL3));
    /*
    Should add TSS......

        0x0028	Task State Segment
            (64-bit System Segment)	Base = &TSS
            Limit = sizeof(TSS)-1
            Access Byte = 0x89
            Flags = 0x0
    */

    asm volatile ("lgdt %0" :: "m"(gdt_descriptors) : "memory");
}

uint64_t create_descriptor(uint32_t base, uint32_t limit, uint16_t flag){
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
