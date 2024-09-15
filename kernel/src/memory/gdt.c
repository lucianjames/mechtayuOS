#include "gdt.h"

#define GDT_SIZE 5

gdt_entry_t gdt_entries[GDT_SIZE] = {0};

gdt_entry_t gdt_create_entry(uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity){
    gdt_entry_t entry;
    entry.base_low = (base & 0xFFFF);
    entry.base_middle = (base >> 16) & 0xFF;
    entry.base_high = (base >> 24) & 0xFF;
    entry.limit_low = (limit & 0xFFFF);
    entry.granularity = (limit >> 16) & 0x0F;
    entry.granularity |= (granularity & 0xF0);
    entry.access = access;
    return entry;
}

void gdt_setup(){
    asm("cli");
    gdt_entries[0] = gdt_create_entry(0, 0, 0, 0);
    gdt_entries[1] = gdt_create_entry(0, 0xFFFFF, 0x9A, 0xCF);
    gdt_entries[2] = gdt_create_entry(0, 0xFFFFF, 0x92, 0xCF);
    gdt_entries[3] = gdt_create_entry(0, 0xFFFFF, 0xFA, 0xCF);
    gdt_entries[4] = gdt_create_entry(0, 0xFFFFF, 0xF2, 0xCF);
    gdt_set_gdt((sizeof(gdt_entry_t)*GDT_SIZE)-1, (uintptr_t)&gdt_entries);
    //gdt_reload_segments();
}