#ifndef PMM_H
#define PMM_H

#include <stddef.h>
#include "limine.h"

#include "constants.h"

#include "utility.h"
#include "serialout.h"
#include "vmm.h"

struct bytemap_info{
    uint64_t base_phys;
    uint32_t size_npages;
};

extern struct bytemap_info g_kbytemap_info;

void pmm_setup_bytemap(struct limine_memmap_response memmap_response);
uint8_t* pmm_alloc_pages(const int n_pages);
void pmm_free_page(const int pageN);
void pmm_free_page_physaddr(uint64_t physical_address);

#endif


    /*
    Byte map structure
    Each byte represents a page of memory (0x1000 bytes)
    The byte contains the following information (counting from LSB as 1st bit):
        1st bit : PAGE_FREE : 1=FREE, 0=USED
        2nd bit -- 8th bit  : unused for now
*/