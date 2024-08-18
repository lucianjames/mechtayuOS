#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "limine.h"

#include "serialout.h"
#include "graphics.h"
#include "kterminal.h"

/*
    Limine bootloader requests, see limine docs/examples for all the info
*/
__attribute__((used, section(".requests")))
static volatile LIMINE_BASE_REVISION(2);

__attribute__((used, section(".requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 2
};

__attribute__((used, section(".requests")))
static volatile struct limine_kernel_address_request kernel_address_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 2
};


__attribute__((used, section(".requests_start_marker")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".requests_end_marker")))
static volatile LIMINE_REQUESTS_END_MARKER;


/*
    Halt execution permanently
*/
static void khalt(void) {
    for (;;) {
        asm ("hlt");
    }
}


/*
    Kernel entry point
*/
void kmain(void) {
    init_debug_serial();
    writestr_debug_serial("Kernel booting...\n");

    // Ensure the bootloader actually understands base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        writestr_debug_serial("ERR: Limine base revision not supported by bootloader\n");
        khalt();
    }

    // Ensure the bootloader has provided a framebuffer
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        writestr_debug_serial("ERR: Bootloader did not provide a framebuffer\n");
        khalt();
    }

    // Now that framebuffer is confirmed to exist, print first message to indicate start of boot! :D
    writestr_debug_serial("======================\n");
    writestr_debug_serial("Hello from the kernel!\n");
    writestr_debug_serial("======================\n");
    serial_dump_psf_info();
    kterm_init(framebuffer_request.response->framebuffers[0]);
    kterm_printf_newline("======================");
    kterm_printf_newline("Hello from the kernel!");
    kterm_printf_newline("======================");

    // Print kernel address
    if(kernel_address_request.response == NULL){
        writestr_debug_serial("ERR: Bootloader did not provide kernel address info\n");
        kterm_printf_newline("ERR: Bootloader did not provide kernel address info");
        khalt();
    }
    kterm_printf_newline("Kernel physical base addr=0x%x Virtual base addr=0x%x", 
            kernel_address_request.response->physical_base, 
            kernel_address_request.response->virtual_base);

    // Print memmap info
    if(memmap_request.response == NULL || memmap_request.response->entry_count < 1){
        writestr_debug_serial("ERR: Bootloader did not provide memmap info\n");
        kterm_printf_newline("ERR: Bootloader did not provide mmap info");
        khalt();
    }
    kterm_printf_newline("MMAP:");
    for(int i=0; i<memmap_request.response->entry_count; i++){
        kterm_printf_newline("Base=0x%x Length=0x%x Type=0x%x", 
                memmap_request.response->entries[i]->base,
                memmap_request.response->entries[i]->length,
                memmap_request.response->entries[i]->type);
    }

    khalt();
}
