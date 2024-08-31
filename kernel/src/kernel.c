#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "limine.h"

#include "serialout.h"
#include "graphics.h"
#include "kterminal.h"

#define KERNEL_STACK_SIZE 0x4000
#define PAGE_SIZE 0x1000

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

    kterm_printf_newline("Framebuffer (virtual) address: 0x%x", framebuffer_request.response->framebuffers[0]->address);
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

    /*
        Create own page tables

        Limine identity maps all memory to ffff800000000000 to give us a starting point.
        Setting up a proper pmm/vmm is going to be very helpful!

        First need to create a bitmap (or bytemap if want flags per page) to mark what memory is available
        Will need iterate through memmap from bootloader and fill out the bitmap accordingly.
        Find the first section of usable memory thats big enough for the memmap and use that to store it.

        Once the bitmap has been created, page tables and essential phys:virt mappings can be made.
        Most important things to add to the new page table include:
        - map kernel to ffffffff800000000000
        - map framebuffer to desired address
        
        Once page tables have been set up and filled with the basics so that the kernel wont immediately triple fault,
        just change CR3 to point to the new PML4 :D

        After that is done, decide a virt addr for kernel heap memory to go.
        Whenever new heap memory is required, map any available physical page(s) on top of the heap mem :DD

        Ill probably write a kbrk function before kmalloc, then have kmalloc utilise kbrk just like malloc and brk :DD
    */

    /*
        Will need a function for mapping virt addr to physical addr, this function should create tables as theyre required.
        This function will need to do lots of interacting with the physical memory manager to get new free pages a bunch.
    */

    // === Step 1: bytemap of physical mem
    // Figure out how many bytes are needed
    kterm_printf_newline("Total pages of memory: %u (0x%x)", totalMemory / PAGE_SIZE, totalMemory / PAGE_SIZE);
    // Find first section in mmap that can fit the bytemap
    uint64_t bytemap_base = UINT64_MAX;
    for(int i=0; i<memmap_request.response->entry_count; i++){
        if(memmap_request.response->entries[i]->type == LIMINE_MEMMAP_USABLE && memmap_request.response->entries[i]->length > totalMemory / PAGE_SIZE){
            bytemap_base = memmap_request.response->entries[i]->base;
            kterm_printf_newline("Found mem section usable for bytemap at base addr: 0x%x with length 0x%x", bytemap_base, memmap_request.response->entries[i]->length);
            break;
        }
    }
    if(bytemap_base == UINT64_MAX){
        kterm_printf_newline("FATAL ERR: no usable memory section found for bytemap");
        debug_serial_printf("FATAL ERR: no usable memory section found for bytemap");
        khalt();
    }
    uint32_t bytemap_size_npages = ((totalMemory / PAGE_SIZE) / PAGE_SIZE) + 1;
    kterm_printf_newline("Number of pages needed for bytemap: %u", bytemap_size_npages);
    /*
        Byte map structure
        Each byte represents a page of memory (0x1000 bytes)
        The byte contains the following information (counting from LSB as 1st bit):
            1st bit : PAGE_FREE 1=FREE 0=USED
            2nd bit -- 8th bit: unused for now
    */
    // Fill the byte map out with PAGE_USED (ensure entire array is 0x0)
    uint64_t bytemap_base_virtual = bytemap_base + 0xffff800000000000;
    for(int i=0; i < bytemap_size_npages * 0x1000; i++) {
        ((char*)bytemap_base_virtual)[i] = 0x0;
        //debug_serial_printf("set 0x%x = 0\n", &((char*)bytemap_base_virtual)[i]);
    }

    // Iterate through memmap and mark usable pages as such
    for(int i=0; i<memmap_request.response->entry_count; i++){
        if(memmap_request.response->entries[i]->type == LIMINE_MEMMAP_USABLE){
            uint64_t usable_section_base = memmap_request.response->entries[i]->base;
            uint64_t usable_section_base_page = usable_section_base / PAGE_SIZE;
            uint64_t usable_section_len_pages = memmap_request.response->entries[i]->length / PAGE_SIZE;
            kterm_printf_newline("Usable section at base 0x%x (page no %u) with len %u pages", usable_section_base, usable_section_base_page, usable_section_len_pages);
            for(int i=usable_section_base_page; i<usable_section_base_page+usable_section_len_pages; i++){
                ((char*)bytemap_base_virtual)[i] = 0b00000001;
                //debug_serial_printf("set 0x%x = 0b00000001\n", &((char*)bytemap_base_virtual)[i]);
            }
        }
    }
    // Now these free pages can be found

    // === Step 2: Create page tables with critically important entries (kernel, framebuffer)
    // Example mapping kernel physical 0x7e3a000 to virt 0xffffffff80000000:
    // 0xffffffff80000000 = 0b1111111111111111111111111111111110000000000000000000000000000000
    // first 16 bits unused:0b----------------111111111111111110000000000000000000000000000000
    // PML4->PDP = 0b----------------<111111111>111111110000000000000000000000000000000 = 512
    // PDP->PD = 0b----------------111111111<111111110>000000000000000000000000000000 = 511
    // PD->PT = 0b----------------111111111111111110<000000000>000000000000000000000 = 0
    // PT->PTE = 0b----------------111111111111111110000000000<000000000>000000000000 = 0
    // PTE + page offset = 0b----------------111111111111111110000000000000000000<000000000000> = 0

    // Find kernel base addr + len
    uint64_t kernel_physical_addr;
    uint64_t kernel_length;
    for(int i=0; i<memmap_request.response->entry_count; i++){
        if(memmap_request.response->entries[i]->type == LIMINE_MEMMAP_KERNEL_AND_MODULES){
            kernel_physical_addr = memmap_request.response->entries[i]->base;
            kernel_length = memmap_request.response->entries[i]->length;
        }
    }

    /*
    Page table stuff just here for testing purposes
    */
    uint64_t kPML4[512] __attribute__((aligned(4096)));
    uint64_t kPDP_1[512] __attribute__((aligned(4096)));
    uint64_t kPD_1[512] __attribute__((aligned(4096)));
    uint64_t kPT_1[512] __attribute__((aligned(4096)));

    // Zero out the test page tables
    for(int i=0; i<512; i++){
        kPML4[i] = 0x0;
        kPDP_1[i] = 0x0;
        kPD_1[i] = 0x0;
        kPT_1[i] = 0x0;
    }

    kPML4[511] = ((uint64_t)kPDP_1 - 0xffff800000000000 ) | 0b1000000000000000000000000000000000000000000000000000000001100011;
    kPDP_1[510] = ((uint64_t)kPD_1 - 0xffff800000000000 ) | 0b1000000000000000000000000000000000000000000000000000000001100011;
    kPD_1[0] = ((uint64_t)kPT_1 - 0xffff800000000000 ) | 0b1000000000000000000000000000000000000000000000000000000001100011;
    for(int i=0; i<kernel_length/0x1000; i++){
        kPT_1[i] = kernel_physical_addr + (0x1000 * i) | 0b1000000000000000000000000000000000000000000000000000000001100011;
    }
    
    uint64_t pml4_physical_addr = ((uint64_t)kPML4) - 0xffff800000000000;
    kterm_printf_newline("new PML4 physical addr: 0x%x", pml4_physical_addr);

    debug_serial_printf("Switching CR3... prepare to crash... ");
    asm volatile("mov %0, %%cr3" :: "r"(pml4_physical_addr));

    debug_serial_printf("Didnt crash :D");

    khalt();
}
