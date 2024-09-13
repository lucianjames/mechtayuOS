#include "vmm.h"

bool g_vmm_usingLiminePageTables = true;

uint64_t* _vmm_PML4_physAddr = NULL;

/*
    Translate limine identity mapped pages to physical addr.
    DOES NOT WORK FOR HIGHER-HALF KERNEL ADDRS!
    Use limine provided kernel address information to translate
    higher half addrs to physical addrs
*/
uint64_t translateaddr_idmap_v2p_limine(uint64_t lvaddr){
    return lvaddr - 0xffff800000000000;
}

uint64_t translateaddr_idmap_p2v_limine(uint64_t addr){
    return addr + 0xffff800000000000;
}

uint64_t translateaddr_idmap_v2p(uint64_t vaddr){
    if(g_vmm_usingLiminePageTables){
        return translateaddr_idmap_v2p_limine(vaddr);
    } else {
        return vaddr - VMM_IDENTITY_MAP_OFFSET;
    }
}

uint64_t translateaddr_idmap_p2v(uint64_t addr){
    if(g_vmm_usingLiminePageTables){
        return translateaddr_idmap_p2v_limine(addr);
    } else {
        return addr + VMM_IDENTITY_MAP_OFFSET;
    }
}


void vmm_setup(const struct limine_memmap_response memmap_response){
    // Allocate a page for PML4
    _vmm_PML4_physAddr = (uint64_t*)pmm_alloc_pages(1);
    // Identity map PML4
    vmm_map_phys2virt((uint64_t)_vmm_PML4_physAddr, (uint64_t)_vmm_PML4_physAddr + VMM_IDENTITY_MAP_OFFSET, 0x3);

    /*
        ===
        Map kernel
        ===
    */
    // Get kernel base + len
    uint64_t kernel_physical_addr;
    uint64_t kernel_length;
    for(uint64_t i=0; i<memmap_response.entry_count; i++){
        if(memmap_response.entries[i]->type == LIMINE_MEMMAP_KERNEL_AND_MODULES){
            kernel_physical_addr = memmap_response.entries[i]->base;
            kernel_length = memmap_response.entries[i]->length;
        }
    }
    // Map kernel to higher half 0xffffffff80000000
    for(int i=0; i<(kernel_length / PAGE_SIZE)+1; i++){
        vmm_map_phys2virt(kernel_physical_addr + (i*PAGE_SIZE), 0xffffffff80000000 + (i*PAGE_SIZE), 0x03);
    }

    /*
        ===
        Create mappings to stack. Keeps limine offset of 0xffff800000000000
        ===
    */
    // Map stack
    uint64_t rsp_val;
    asm("mov %%rsp, %0" : "=r"(rsp_val));
    //kterm_printf_newline("RSP (virtual): 0x%x", rsp_val);
    uint64_t rsp_val_phys = translateaddr_idmap_v2p(rsp_val);
    //kterm_printf_newline("RSP (translated to physical): 0x%x", rsp_val);
    for(int i=0; i<(KERNEL_STACK_SIZE/PAGE_SIZE)+1; i++){
        vmm_map_phys2virt(rsp_val_phys + (i*PAGE_SIZE), rsp_val_phys + 0xffff800000000000 + (i*PAGE_SIZE), 0x03);
    }

    /*
        ===
        Map PMM bytemap
        ===
    */
    for(int i=0; i<g_kbytemap_info.size_npages; i++){
        vmm_map_phys2virt(g_kbytemap_info.base_phys + (i*PAGE_SIZE), g_kbytemap_info.base_phys + VMM_IDENTITY_MAP_OFFSET + (i*PAGE_SIZE), 0x3);
    }
    
    /*
        Switch to new page table
    */
    debug_serial_printf("Switching CR3... ");
    vmm_switchCR3();
    debug_serial_printf("OK!\n");
}

/*
    Maps a physical address to a virtual address.
    Uses the PML4 at _vmm_PML4_physAddr.
    Creates additional tables as required.
*/
uint64_t vmm_map_phys2virt(uint64_t phys_addr, uint64_t virt_addr, uint64_t flags){
    /*
        Extract page table offsets from virtual address
    */
    // Bits > 47 are unused
    uint16_t va_PML4_offset = (virt_addr >> 39) & 0b111111111;
    uint16_t va_PDP_offset = (virt_addr >> 30) & 0b111111111;
    uint16_t va_PD_offset = (virt_addr >> 21) & 0b111111111;
    uint16_t va_PT_offset = (virt_addr >> 12) & 0b111111111;
    // Bits < 12 are offset, not included in PTE

    /*
        PTE format:
        0-11 : flags
        12 - 48 : PFN (phys addr div by page size)
        49 - 62 : reserved
        63 : executable flag
    */
    /*
        Assemble PTE
    */
    uint64_t flagFilter = 0b1000000000000000000000000000000000000000000000000000111111111111; // These bits are allowed to be set by the flags parameter.
    flags &= flagFilter;

    uint64_t PTE = (flags) 
                    | ((phys_addr / PAGE_SIZE) << PAGE_BITSIZE);

    //debug_serial_printf("New PTE: 0x%x\n", PTE);

    /*
        Walk through page tables to find PT to insert PTE into
    */
    uint64_t* PML4_virtAddr = (uint64_t*)translateaddr_idmap_p2v((uint64_t)_vmm_PML4_physAddr);
    uint64_t PDP_physAddr = PML4_virtAddr[va_PML4_offset] & (~flagFilter);
    if(PDP_physAddr == 0x0){
        PML4_virtAddr[va_PML4_offset] = (uint64_t)pmm_alloc_pages(1);
        // Identity map the new page so that it can be accessed by the kernel later
        vmm_map_phys2virt(PML4_virtAddr[va_PML4_offset], PML4_virtAddr[va_PML4_offset] + VMM_IDENTITY_MAP_OFFSET, 0x3);
        PDP_physAddr = PML4_virtAddr[va_PML4_offset] & (~flagFilter);
        PML4_virtAddr[va_PML4_offset] |= flags;
    }

    uint64_t* PDP_virtAddr = (uint64_t*)translateaddr_idmap_p2v((uint64_t)PDP_physAddr);
    uint64_t PD_physAddr = PDP_virtAddr[va_PDP_offset] & (~flagFilter);
    if(PD_physAddr == 0x0){
        PDP_virtAddr[va_PDP_offset] = (uint64_t)pmm_alloc_pages(1);
        // Identity map the new page so that it can be accessed by the kernel later
        vmm_map_phys2virt(PDP_virtAddr[va_PDP_offset], PDP_virtAddr[va_PDP_offset] + VMM_IDENTITY_MAP_OFFSET, 0x3);
        PD_physAddr = PDP_virtAddr[va_PDP_offset];
        PDP_virtAddr[va_PDP_offset] |= flags;
    }

    uint64_t* PD_virtAddr = (uint64_t*)translateaddr_idmap_p2v((uint64_t)PD_physAddr);
    uint64_t PT_physAddr = PD_virtAddr[va_PD_offset] & (~flagFilter);
    if(PT_physAddr == 0x0){
        PD_virtAddr[va_PD_offset] = (uint64_t)pmm_alloc_pages(1);
        // Identity map the new page so that it can be accessed by the kernel later
        vmm_map_phys2virt(PD_virtAddr[va_PD_offset], PD_virtAddr[va_PD_offset] + VMM_IDENTITY_MAP_OFFSET, 0x3);
        PT_physAddr = PD_virtAddr[va_PD_offset];
        PD_virtAddr[va_PD_offset] |= flags;
    }

    uint64_t* PT_virtAddr = (uint64_t*)translateaddr_idmap_p2v((uint64_t)PT_physAddr);

    // Insert the page table entry
    PT_virtAddr[va_PT_offset] = PTE;

    return virt_addr;
}

uint64_t vmm_identity_map_page(uint64_t phys_addr, uint64_t flags){
    return vmm_map_phys2virt(phys_addr, phys_addr + VMM_IDENTITY_MAP_OFFSET, flags);
}

uint64_t vmm_identity_map_n_pages(uint64_t phys_base_addr, int n_pages, uint64_t flags){
    uint64_t res = 0;
    for(int i=0; i<n_pages; i++){
        uint64_t vaddr = vmm_identity_map_page(phys_base_addr + (i*PAGE_SIZE), flags);
        if(i==0){
            res = vaddr;
        }
    }
    return res;
}

void vmm_switchCR3(){
    asm volatile("mov %0, %%cr3" :: "r"((uint64_t)_vmm_PML4_physAddr));
    g_vmm_usingLiminePageTables = false;
}