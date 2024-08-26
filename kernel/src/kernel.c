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
    kterm_printf_newline("=========================================================================================");
    kterm_printf_newline("|                                                                    @@@@       -@@@:   |");
    kterm_printf_newline("| .@@@  @@@@ @@@@@@@ @@@   @@ @@@@@@@@   @@@    @@  @@@@@@         @@@@@@@@   @@@@@@@@@ |");
    kterm_printf_newline("|  @@@  @@@: #@      -@    @@    @*     @@ @    @  @@    @@      @@@     @@:  @@@       |");
    kterm_printf_newline("|  @+@@ @ @: #@@@@@@ +@@@@@@@    @@     @  @@   @@@@@    @@      @@@     @@-   @@@@@    |");
    kterm_printf_newline("|  @@ @-@ @: #@         #- @@    @@    @@@@@@@  @  @@    @@ @@@@ @@-    @@@       @@@=  |");
    kterm_printf_newline("| .@@ @@  @@ @@@@@@@       @@    @@   @@    @@@ @@  @@@@@@       @@+   @@@   @@    @@@  |");
    kterm_printf_newline("|                                                                 @@@@@@@    @@@@@@@@   |");
    kterm_printf_newline("=========================================================================================");


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
        kterm_printf_newline("Base=0x%x Length=0x%x Type=%s", 
                memmap_request.response->entries[i]->base,
                memmap_request.response->entries[i]->length,
                typestr);
    }
    kterm_printf_newline("Usable mem size: %u bytes across %u sections", usableMemSize, usableSections);
    kterm_printf_newline("Total mem size (not incl MEMMAP_RESERVED): %u bytes", totalMemory);
    /*
        4096 byte page size, use one byte per page to store various flags
        usableMemSize / 4096
    */
    kterm_printf_newline("Number of bytes required for bytemap of total usable memory: %u bytes", usableMemSize / 4096);

    /*
        Usable memory should be mapped to continuous chunk of virtual addresses
        Once this has been done, a bytemap can be assembled tracking which virtual addresses are allocated
        This is just a simple way of doing things for now
    */
    /*
        PML4 - 512 entries pointing to different page dir pointer tables
        PDPT - 512 entries (per PDPT, of which there are 512) pointing to page directory tables
        PDT - 512 entries (per PDT x512) pointing to pages
        page - 4096 entries containing pointers to physical memory addresses of pages (0x1000 sized chunks of memory)
    */

    /*

        find_free_phys_page(){
            // For super super not optimised basic version 1, just iterate physical mem bitmap until free page found
            for(bitmap entries){
                if(entry free){
                    return entry address;
                }
            }
            return null; // no mem left
        }

        map_addr(){

        }

        allocate_new_page(virt_addr){ // Map the given virtual address to any available physical address
            phsy_addr = find_free_phys_page()
            map_addr(phys_addr, virt_addr);
        }


    */


    khalt();
}



/*

                                                                                                                   
                                                                                                                   
                                                                                        @@@@@@@@:       @@@@@@@@   
  @@@@+   @@@@  @@@@@@@@@  @@@     @@ %@@@@@@@@@+    @@@@     @@    @@@@@@@           %@@@@   @@@@    @@@@    @@@  
   @@@@   @@@@  .@         @@-     @@      @        @@ @@     @@   @@     @@         @@@@      @@@    @@@          
   @@ @  @@ @@  .@@@@@@@@  @@@     @@     .@       @@@ -@@    @@@@@@       @@        @@@       @@@-   -@@@@@       
   @@ @@ @  @@  .@@         @@@@@@@@@     .@       @@   @@@   @@  @@       @+ @@@@@ @@@@       @@@       @@@@@@    
  .@@ =@@@  @@  :@@                @@     -@      @@@@@@@@@   @@   @@    -@@        @@@@      @@@@          @@@@   
  @@@  @@@ =@@  @@@@@@@@@          @@     @@*    @@@     @@@  @@    @@@@@@#          @@@     @@@@    @-     :@@@   
                                                                                     @@@@@@@@@@     @@@@@@@@@@@    
                                                                                        @@@@#          @@@@@+      


                                                                                       
                                                                    @@@@       -@@@:   
 .@@@  @@@@ @@@@@@@ @@@   @@ @@@@@@@@   @@@    @@  @@@@@@         @@@@@@@@   @@@@@@@@@ 
  @@@  @@@: #@      %@    @@    @*     @@ @    @  @@    @@      @@@     @@:  @@@       
  @+@@ @ @: #@@@@@@ +@@@@@@@    @@     @  @@   @@@@@    @@      @@@     @@-   @@@@@    
  @@ @%@ @: #@         #% @@    @@    @@@@@@@  @  @@    @@ @@@@ @@%    @@@       @@@=  
 .@@ @@  @@ @@@@@@@       @@    @@   @@    @@@ @@  @@@@@@       @@+   @@@   @@    @@@  
                                                                 @@@@@@@    @@@@@@@@   
                                                                                       
                                                                                       
                                                                                                                     
*/