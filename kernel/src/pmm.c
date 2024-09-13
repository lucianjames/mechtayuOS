#include "pmm.h"

struct bytemap_info g_kbytemap_info = {0, 0};
uint32_t _pmm_bytemap_free_page_cache = 0;


void pmm_setup_bytemap(struct limine_memmap_response memmap_response){
    // Get size of all memory
    size_t totalMemory = 0;
    for(uint64_t i=0; i<memmap_response.entry_count; i++){
        totalMemory += memmap_response.entries[i]->length;
    }
    
    // Find first section in mmap that can fit the entire bytemap as one continuous chunk
    uint64_t bytemap_base = UINT64_MAX;
    for(uint64_t i=0; i<memmap_response.entry_count; i++){
        if(memmap_response.entries[i]->type == LIMINE_MEMMAP_USABLE && memmap_response.entries[i]->length > totalMemory / PAGE_SIZE){
            bytemap_base = memmap_response.entries[i]->base;
            debug_serial_printf("Found mem section usable for bytemap at base addr: 0x%x with length 0x%x\n", bytemap_base, memmap_response.entries[i]->length);
            break;
        }
    }
    if(bytemap_base == UINT64_MAX){
        debug_serial_printf("FATAL ERR: no usable memory section found for bytemap\n");
        khalt();
    }

    uint32_t bytemap_size_npages = ((totalMemory / PAGE_SIZE) / PAGE_SIZE) + 1;
    uint32_t bytemap_size_bytes = bytemap_size_npages * 0x1000;

    // Fill the bytemap with PAGE_USED
    uint64_t bytemap_base_virtual = translateaddr_idmap_p2v(bytemap_base);
    for(uint32_t i=0; i < bytemap_size_bytes; i++) {
        ((uint8_t*)bytemap_base_virtual)[i] = 0x0;
    }

    // Iter through memmap and mark usabe pages as such in the bytemap
    for(uint64_t i=0; i<memmap_response.entry_count; i++){
        if(memmap_response.entries[i]->type == LIMINE_MEMMAP_USABLE){
            uint64_t usable_section_base = memmap_response.entries[i]->base;
            uint64_t usable_section_base_page = usable_section_base / PAGE_SIZE;
            uint64_t usable_section_len_pages = memmap_response.entries[i]->length / PAGE_SIZE;
            debug_serial_printf("Usable section at base 0x%x (page no %u) with len %u pages\n", usable_section_base, usable_section_base_page, usable_section_len_pages);
            for(uint64_t i=usable_section_base_page; i<usable_section_base_page+usable_section_len_pages; i++){
                ((uint8_t*)bytemap_base_virtual)[i] = 0b00000001;
            }
        }
    }

    // Now mark the pages the bytemap itself is sitting in as used
    uint32_t bytemap_start_page = (bytemap_base/PAGE_SIZE)-1;
    for(uint32_t i=0; i<bytemap_size_npages; i++){
        ((uint8_t*)bytemap_base_virtual)[bytemap_start_page+i] = 0b00000000;
    }

    // Now the bytemap can be used to find free pages of memory
    g_kbytemap_info.base_phys = bytemap_base;
    g_kbytemap_info.size_npages = bytemap_size_npages;
}



uint8_t* pmm_alloc_pages(const int n_pages){
    uint8_t* allocStartAddr = NULL;

    // Find N free pages in the bytemap
    // Writing this the REALLY inefficient way first.
    // Optimisations can come later

    uint8_t* bytemap_vaddr = (uint8_t*)translateaddr_idmap_p2v(g_kbytemap_info.base_phys);

    for(uint32_t i=_pmm_bytemap_free_page_cache; i<g_kbytemap_info.size_npages*0x1000; i++){
        if(bytemap_vaddr[i] & 0b00000001){
            for(int j=0; j<n_pages; j++){
                if(!(bytemap_vaddr[i+j] & 0b00000001)){
                    i+=j;
                    break;
                }
                if(j==n_pages-1){
                    allocStartAddr = (uint8_t*)(i*PAGE_SIZE);
                    // mark from i to i+n_pages as used
                    for(int np = 0; np < n_pages; np++){
                        bytemap_vaddr[i+np] = 0b00000000;
                        _pmm_bytemap_free_page_cache = i+np+1;
                    }
                    return allocStartAddr;
                }
            }
        }

        // If exhausted everything above _pmm_bytemap_free_page_cache, then check everything below it.
        // todo (we wont come close to running out of memory for now.)
    }

    // We could trust the caller to check for null addr from this function,
    // but right now nah
    debug_serial_printf("FATAL ERR: PMM_OOM\n");
    khalt();

    return allocStartAddr;
}



void pmm_free_page(const int pageN){
    uint8_t* bytemap_vaddr = (uint8_t*)translateaddr_idmap_p2v(g_kbytemap_info.base_phys);
    bytemap_vaddr[pageN] |= 0b00000001;
}



void pmm_free_page_physaddr(uint64_t physical_address){
    pmm_free_page(physical_address / PAGE_SIZE);
}