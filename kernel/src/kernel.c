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


__attribute__((used, section(".requests_start_marker")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".requests_end_marker")))
static volatile LIMINE_REQUESTS_END_MARKER;
















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
    for(uint64_t i=0; i<memmap_request.response->entry_count; i++){
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

    // Set up PMM
    pmm_setup_bytemap(memmap_request);
    kterm_printf_newline("Bytemap base: 0x%x", g_kbytemap.base);
    kterm_printf_newline("Bytemap size (bytes): %u (0x%x), (n_pages): %u", g_kbytemap.size_npages * PAGE_SIZE, g_kbytemap.size_npages * PAGE_SIZE, g_kbytemap.size_npages);

    // Set up VMM
    vmm_setup(); // Allocs page for pml4, sets g_vmm_usingLiminePageTables = true

    uint64_t kernel_physical_addr;
    uint64_t kernel_length;
    for(uint64_t i=0; i<memmap_request.response->entry_count; i++){
        if(memmap_request.response->entries[i]->type == LIMINE_MEMMAP_KERNEL_AND_MODULES){
            kernel_physical_addr = memmap_request.response->entries[i]->base;
            kernel_length = memmap_request.response->entries[i]->length;
        }
    }

    // Map kernel 
    for(int i=0; i<(kernel_length / PAGE_SIZE)+1; i++){
        vmm_map_phys2virt(kernel_physical_addr + (i*PAGE_SIZE), 0xffffffff80000000 + (i*PAGE_SIZE), 0b1000000000000000000000000000000000000000000000000000000000100011);
    }

    // Map stack
    uint64_t rsp_val;
    asm("mov %%rsp, %0" : "=r"(rsp_val));
    rsp_val = translateaddr_v2p(rsp_val);
    kterm_printf_newline("RSP: 0x%x", rsp_val);
    for(int i=0; i<(KERNEL_STACK_SIZE/PAGE_SIZE)+1; i++){
        vmm_map_phys2virt(rsp_val + (i*PAGE_SIZE), rsp_val + 0xffff800000000000 + (i*PAGE_SIZE), 0b1000000000000000000000000000000000000000000000000000000000100011);
    }

    debug_serial_printf("Switching CR3... prepare to crash... ");
    vmm_switchCR3();
    debug_serial_printf("Didnt crash :D");


    // Test PMM
    /*
    char* allocatedPage = pmm_alloc_pages(1);
    kterm_printf_newline("pmm_alloc_pages(1) result: 0x%x", allocatedPage);
    kterm_printf_newline("Writing %u bytes to allocated page", PAGE_SIZE);
    for(int i=0; i<PAGE_SIZE; i++){
        allocatedPage[i] = 0x69;
    }
    */

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

    /*
    Page table stuff just here for testing purposes
    uint64_t* kPML4 = (uint64_t*)pmm_alloc_pages(1);
    uint64_t* kPDP_1 = (uint64_t*)pmm_alloc_pages(1);
    uint64_t* kPD_1 = (uint64_t*)pmm_alloc_pages(1);
    uint64_t* kPT_1 = (uint64_t*)pmm_alloc_pages(1);
    // Zero out the test page tables
    for(int i=0; i<512; i++){
        kPML4[i] = 0x0;
        kPDP_1[i] = 0x0;
        kPD_1[i] = 0x0;
        kPT_1[i] = 0x0;
    }

    kPML4[511] = ((uint64_t)kPDP_1 - 0xffff800000000000 ) | 0b1000000000000000000000000000000000000000000000000000000000100011;
    kPDP_1[510] = (((uint64_t)kPD_1) - 0xffff800000000000 ) | 0b1000000000000000000000000000000000000000000000000000000000100011;
    kPD_1[0] = (((uint64_t)kPT_1) - 0xffff800000000000 ) | 0b1000000000000000000000000000000000000000000000000000000000100011;
    for(uint64_t i=0; i<kernel_length/0x1000; i++){
        kPT_1[i] = (kernel_physical_addr + (0x1000 * i)) | 0b1000000000000000000000000000000000000000000000000000000000000011;
    }
    
    uint64_t pml4_physical_addr = ((uint64_t)kPML4) - 0xffff800000000000;
    kterm_printf_newline("new PML4 physical addr: 0x%x", pml4_physical_addr);
    */

    //CR2=ffffffff80001090
    //RSP=ffff8000bfe93000

    //CR2=ffffffff800010a7
    //RSP=ffff8000bfe92000


    //debug_serial_printf("Switching CR3... prepare to crash... ");
    //asm volatile("mov %0, %%cr3" :: "r"(pml4_physical_addr));
    //debug_serial_printf("Didnt crash :D");

    khalt();
}
