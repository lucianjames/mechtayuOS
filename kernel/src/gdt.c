#include "gdt.h"

/*
    Just a flat memory model.
    Would probably be "fine" to keep using limines GDT but
    having control is better.
*/
void gdt_setup(){

    //asm volatile ("lgdt %0" :: "m"(gdt_descriptor_instance));
}
