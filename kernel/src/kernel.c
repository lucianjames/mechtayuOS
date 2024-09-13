#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "limine.h"

#include "serialout.h"
#include "graphics.h"
#include "kterminal.h"
#include "constants.h"
#include "utility.h"
#include "pmm.h"
#include "vmm.h"

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

__attribute__((used, section(".requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};


__attribute__((used, section(".requests_start_marker")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".requests_end_marker")))
static volatile LIMINE_REQUESTS_END_MARKER;















/*
    Kernel entry point
*/
void kmain(void) {
    /*
        Init debug serial comms
    */
    init_debug_serial();
    writestr_debug_serial("Kernel booting...\n");

    /*
        Sanity checks
    */
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
    // Ensure bootloader provided information about kernel location
    if(kernel_address_request.response == NULL){
        writestr_debug_serial("ERR: Bootloader did not provide kernel address info\n");
        khalt();
    }
    // Ensure stack req
    if(kernel_stack_request.response == NULL){
        writestr_debug_serial("ERR: Bootloader did not provide kernel stack size response\n");
        khalt();
    }
    // Ensure memmap exists
    if(memmap_request.response == NULL || memmap_request.response->entry_count < 1){
        writestr_debug_serial("ERR: Bootloader did not provide memmap info\n");
        khalt();
    }

    /*
        Copy some useful bits of limine request/response info to nice neat structs on the stack.
    */
    struct limine_kernel_address_response k_kerneladdr_info = *kernel_address_request.response;
    struct limine_memmap_response k_memmap_info = *memmap_request.response;
    struct limine_framebuffer k_framebuffer = *framebuffer_request.response->framebuffers[0];

    /*
        Set up PMM
    */
    pmm_setup_bytemap(k_memmap_info);

    /*
        Set up VMM - this function will create basic mappings that allow the kernel to continue to execute.
        This provides VERY minimal mapping. ONLY kernel + stack + page table + bytemap will be mapped.
    */
    vmm_setup(k_memmap_info);

    /*
        Set up framebuffer + kterm
    */
    uint64_t framebuffer_physical_address = translateaddr_idmap_v2p_limine((uint64_t)k_framebuffer.address);
    uint64_t framebuffer_length_bytes = (k_framebuffer.height * k_framebuffer.width * k_framebuffer.bpp) / 8;
    int framebuffer_length_pages = (framebuffer_length_bytes / PAGE_SIZE) + 1;
    for(int i=0; i<framebuffer_length_pages; i++){
        vmm_map_phys2virt(framebuffer_physical_address + (i*PAGE_SIZE), framebuffer_physical_address + VMM_IDENTITY_MAP_OFFSET + (i*PAGE_SIZE), 0x3);
    }
    k_framebuffer.address = (void*)(framebuffer_physical_address + VMM_IDENTITY_MAP_OFFSET);
    kterm_init(&k_framebuffer); // Now kterm can be initialised on the framebuffer

    /*
        Print logo
    */
    kterm_printf_newline("=========================================================================================");
    kterm_printf_newline("|                                                                    @@@@       -@@@:   |");
    kterm_printf_newline("| .@@@  @@@@ @@@@@@@ @@@   @@ @@@@@@@@   @@@    @@  @@@@@@         @@@@@@@@   @@@@@@@@@ |");
    kterm_printf_newline("|  @@@  @@@: #@      -@    @@    @*     @@ @    @  @@    @@      @@@     @@:  @@@       |");
    kterm_printf_newline("|  @+@@ @ @: #@@@@@@ +@@@@@@@    @@     @  @@   @@@@@    @@ @@@@ @@@     @@-   @@@@@    |");
    kterm_printf_newline("|  @@ @-@ @: #@         #- @@    @@    @@@@@@@  @  @@    @@      @@-    @@@       @@@=  |");
    kterm_printf_newline("| .@@ @@  @@ @@@@@@@       @@    @@   @@    @@@ @@  @@@@@@       @@+   @@@   @@    @@@  |");
    kterm_printf_newline("|                                                                 @@@@@@@    @@@@@@@@   |");
    kterm_printf_newline("=========================================================================================");



    khalt();



    /*=
==================================== BELOW CODE WONT RUN ===========================================================================================



















    // Set up PMM
    pmm_setup_bytemap(k_memmap_info);

    // Set up VMM - this function will create basic mappings that allow the kernel to continue to execute.
    // This provides VERY minimal mapping. ONLY kernel + stack + page table + bytemap will be mapped.
    vmm_setup(k_memmap_info);

    k_memmap_info.entries = (struct limine_memmap_entry **)translateaddr_idmap_v2p_limine((uint64_t)k_memmap_info.entries);
    vmm_map_phys2virt((uint64_t)k_memmap_info.entries, (uint64_t)k_memmap_info.entries + VMM_IDENTITY_MAP_OFFSET, 0x03);
    k_memmap_info.entries = (struct limine_memmap_entry **)translateaddr_idmap_p2v((uint64_t)k_memmap_info.entries);

    // Set up virtual address for framebuffer
    uint64_t framebuffer_physical_address = translateaddr_idmap_v2p_limine((uint64_t)k_framebuffer.address);
    uint64_t framebuffer_length_bytes = (k_framebuffer.height * k_framebuffer.width * k_framebuffer.bpp) / 8;
    int framebuffer_length_pages = (framebuffer_length_bytes / PAGE_SIZE) + 1;
    
    for(int i=0; i<framebuffer_length_pages; i++){
        vmm_map_phys2virt(framebuffer_physical_address + (i*PAGE_SIZE), framebuffer_physical_address + VMM_IDENTITY_MAP_OFFSET + (i*PAGE_SIZE), 0x3);
    }

    k_framebuffer.address = (void*)(framebuffer_physical_address + VMM_IDENTITY_MAP_OFFSET);

    */
    

    // Now that framebuffer is confirmed to exist, print first message to indicate start of boot! :D
    writestr_debug_serial("======================\n");
    writestr_debug_serial("Hello from the kernel!\n");
    writestr_debug_serial("======================\n");
    serial_dump_psf_info();

    //kterm_init(k_framebuffer);

    kterm_printf_newline("=========================================================================================");
    kterm_printf_newline("|                                                                    @@@@       -@@@:   |");
    kterm_printf_newline("| .@@@  @@@@ @@@@@@@ @@@   @@ @@@@@@@@   @@@    @@  @@@@@@         @@@@@@@@   @@@@@@@@@ |");
    kterm_printf_newline("|  @@@  @@@: #@      -@    @@    @*     @@ @    @  @@    @@      @@@     @@:  @@@       |");
    kterm_printf_newline("|  @+@@ @ @: #@@@@@@ +@@@@@@@    @@     @  @@   @@@@@    @@ @@@@ @@@     @@-   @@@@@    |");
    kterm_printf_newline("|  @@ @-@ @: #@         #- @@    @@    @@@@@@@  @  @@    @@      @@-    @@@       @@@=  |");
    kterm_printf_newline("| .@@ @@  @@ @@@@@@@       @@    @@   @@    @@@ @@  @@@@@@       @@+   @@@   @@    @@@  |");
    kterm_printf_newline("|                                                                 @@@@@@@    @@@@@@@@   |");
    kterm_printf_newline("=========================================================================================");

    //kterm_printf_newline("Framebuffer (virtual) address: 0x%x", framebuffer_request.response->framebuffers[0]->address);
    //kterm_printf_newline("Framebuffer height: %u", framebuffer_request.response->framebuffers[0]->height);
    //kterm_printf_newline("Framebuffer width: %u", framebuffer_request.response->framebuffers[0]->width);
    //kterm_printf_newline("Framebuffer BPP: %u", framebuffer_request.response->framebuffers[0]->bpp);

    kterm_printf_newline("Bytemap base: 0x%x", g_kbytemap_info.base);
    kterm_printf_newline("Bytemap size (bytes): %u (0x%x), (n_pages): %u", g_kbytemap_info.size_npages * PAGE_SIZE, g_kbytemap_info.size_npages * PAGE_SIZE, g_kbytemap_info.size_npages);

    kterm_printf_newline("Kernel physical base addr=0x%x Virtual base addr=0x%x", 
            kernel_address_request.response->physical_base, 
            kernel_address_request.response->virtual_base);
    kterm_printf_newline("Kernel stack size = 0x%x", KERNEL_STACK_SIZE);

    kterm_printf_newline("MMAP:");
    size_t usableMemSize = 0;
    size_t usableSections = 0;
    size_t totalMemory = 0;
    for(uint64_t i=0; i<k_memmap_info.entry_count; i++){
        totalMemory += k_memmap_info.entries[i]->length;
        char* typestr;
        switch(k_memmap_info.entries[i]->type){
            case LIMINE_MEMMAP_USABLE:
                typestr = "MEMMAP_USABLE";
                usableMemSize += k_memmap_info.entries[i]->length;
                usableSections++;
                break;
            case LIMINE_MEMMAP_RESERVED:
                typestr = "MEMMAP_RESERVED";
                totalMemory -= k_memmap_info.entries[i]->length;
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
                k_memmap_info.entries[i]->base,
                k_memmap_info.entries[i]->length,
                typestr);
    }
    kterm_printf_newline("Usable mem size: %u bytes across %u sections", usableMemSize, usableSections);
    kterm_printf_newline("Total mem size (not incl MEMMAP_RESERVED): %u bytes", totalMemory);

    


    khalt();
}
