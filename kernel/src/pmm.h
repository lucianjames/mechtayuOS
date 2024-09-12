#ifndef PMM_H
#define PMM_H

#include <stddef.h>
#include "limine.h"

#include "constants.h"

#include "utility.h"
#include "serialout.h"

struct bytemap_info{
    uint64_t base;
    uint32_t size_npages;
};

extern struct bytemap_info g_kbytemap;

void pmm_setup_bytemap(struct limine_memmap_request memmap_request);
char* pmm_alloc_pages(const int n_pages);

#endif

 /*
        Byte map structure
        Each byte represents a page of memory (0x1000 bytes)
        The byte contains the following information (counting from LSB as 1st bit):
            1st bit : PAGE_FREE 1=FREE 0=USED
            2nd bit -- 8th bit: unused for now
    */


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
