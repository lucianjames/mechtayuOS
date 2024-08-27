#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "limine.h"

#include "serialout.h"
#include "graphics.h"
#include "kterminal.h"

#define KERNEL_STACK_SIZE 0x4000

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

__attribute__((used, section(".requests")))
static volatile struct limine_stack_size_request kernel_stack_request = {
    .id = LIMINE_STACK_SIZE_REQUEST,
    .revision = 2,
    .stack_size = KERNEL_STACK_SIZE
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
    kterm_printf_newline("=========================================================================================");
    kterm_printf_newline("|                                                                    @@@@       -@@@:   |");
    kterm_printf_newline("| .@@@  @@@@ @@@@@@@ @@@   @@ @@@@@@@@   @@@    @@  @@@@@@         @@@@@@@@   @@@@@@@@@ |");
    kterm_printf_newline("|  @@@  @@@: #@      -@    @@    @*     @@ @    @  @@    @@      @@@     @@:  @@@       |");
    kterm_printf_newline("|  @+@@ @ @: #@@@@@@ +@@@@@@@    @@     @  @@   @@@@@    @@ @@@@ @@@     @@-   @@@@@    |");
    kterm_printf_newline("|  @@ @-@ @: #@         #- @@    @@    @@@@@@@  @  @@    @@      @@-    @@@       @@@=  |");
    kterm_printf_newline("| .@@ @@  @@ @@@@@@@       @@    @@   @@    @@@ @@  @@@@@@       @@+   @@@   @@    @@@  |");
    kterm_printf_newline("|                                                                 @@@@@@@    @@@@@@@@   |");
    kterm_printf_newline("=========================================================================================");

    kterm_printf_newline("Framebuffer address: 0x%x", framebuffer_request.response->framebuffers[0]->address);
    kterm_printf_newline("Framebuffer height: %u", framebuffer_request.response->framebuffers[0]->height);
    kterm_printf_newline("Framebuffer width: %u", framebuffer_request.response->framebuffers[0]->width);
    kterm_printf_newline("Framebuffer BPP: %u", framebuffer_request.response->framebuffers[0]->bpp);

    // Print kernel address
    if(kernel_address_request.response == NULL){
        writestr_debug_serial("ERR: Bootloader did not provide kernel address info\n");
        kterm_printf_newline("ERR: Bootloader did not provide kernel address info");
        khalt();
    }
    kterm_printf_newline("Kernel physical base addr=0x%x Virtual base addr=0x%x", 
            kernel_address_request.response->physical_base, 
            kernel_address_request.response->virtual_base);
    if(kernel_stack_request.response == NULL){
        writestr_debug_serial("ERR: Bootloader did not provide kernel stack size response\n");
        kterm_printf_newline("ERR: Bootloader did not provide kernel stack size response");
        khalt();
    }
    kterm_printf_newline("Kernel stack size = 0x%x", KERNEL_STACK_SIZE);

    // Print memmap info
    if(memmap_request.response == NULL || memmap_request.response->entry_count < 1){
        writestr_debug_serial("ERR: Bootloader did not provide memmap info\n");
        kterm_printf_newline("ERR: Bootloader did not provide mmap info");
        khalt();
    }
    kterm_printf_newline("MMAP:");
    size_t usableMemSize = 0;
    size_t usableSections = 0;
    size_t totalMemory = 0;
    for(int i=0; i<memmap_request.response->entry_count; i++){
        totalMemory += memmap_request.response->entries[i]->length;
        char* typestr;
        switch(memmap_request.response->entries[i]->type){
            case LIMINE_MEMMAP_USABLE:
                typestr = "MEMMAP_USABLE";
                usableMemSize += memmap_request.response->entries[i]->length;
                usableSections++;
                break;
            case LIMINE_MEMMAP_RESERVED:
                typestr = "MEMMAP_RESERVED";
                totalMemory -= memmap_request.response->entries[i]->length;
                break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
                typestr = "MEMMAP_ACPI_RECLAIMABLE";
                break;
            case LIMINE_MEMMAP_ACPI_NVS:
                typestr = "MEMMAP_ACPI_NVS";
                break;
            case LIMINE_MEMMAP_BAD_MEMORY:
                typestr = "MEMMAP_BAD_MEMORY";
                break;
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
                typestr = "MEMMAP_BOOTLOADER_RECLAIMABLE";
                break;
            case LIMINE_MEMMAP_KERNEL_AND_MODULES:
                typestr = "MEMMAP_KERNEL_AND_MODULES";
                break;
            case LIMINE_MEMMAP_FRAMEBUFFER:
                typestr = "MEMMAP_FRAMEBUFFER";
                break;
            default:
                typestr = "UNKNOWN";
                break;
        }
        kterm_printf_newline("  Base=0x%x Length=0x%x Type=%s", 
                memmap_request.response->entries[i]->base,
                memmap_request.response->entries[i]->length,
                typestr);
    }
    kterm_printf_newline("Usable mem size: %u bytes across %u sections", usableMemSize, usableSections);
    kterm_printf_newline("Total mem size (not incl MEMMAP_RESERVED): %u bytes", totalMemory);

    uint64_t cr3_val;
    asm("mov %%cr3, %0" : "=r"(cr3_val));
    kterm_printf_newline("CR3: 0x%x", cr3_val);



    khalt();
}
