#include "vmm.h"

bool g_vmm_usingLiminePageTables = true;

uint64_t* _vmm_PML4_physAddr = NULL;

/*
    Translate limine identity mapped pages to physical addr.
    DOES NOT WORK FOR HIGHER-HALF KERNEL ADDRS!
    Use limine provided kernel address information to translate
    higher half addrs to physical addrs
*/
uint64_t translateaddr_v2p_limine(uint64_t lvaddr){
    return lvaddr - 0xffff800000000000;
}

uint64_t translateaddr_p2v_limine(uint64_t addr){
    return addr + 0xffff800000000000;
}

uint64_t translateaddr_v2p(uint64_t vaddr){
    if(g_vmm_usingLiminePageTables){
        return translateaddr_v2p_limine(vaddr);
    } else {
        return vaddr; // Temp, actual mapping todo
    }
}

uint64_t translateaddr_p2v(uint64_t addr){
    if(g_vmm_usingLiminePageTables){
        return translateaddr_p2v_limine(addr);
    } else {
        return addr; // Temp, actual mapping todo
    }
}


void vmm_setup(){
    _vmm_PML4_physAddr = (uint64_t*)pmm_alloc_pages(1);
    g_vmm_usingLiminePageTables = true;
}

/*
    Maps a physical address to a virtual address.
    Uses the PML4 at _vmm_PML4_physAddr.
    Creates additional tables as required.
*/
int vmm_map_phys2virt(uint64_t phys_addr, uint64_t virt_addr, uint64_t flags){
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
    uint64_t flagFilter = 0b1000000000000000000000000000000000000000000000000000111111111111;
    flags &= flagFilter;

    uint64_t PTE = (flags) 
                    | ((phys_addr / PAGE_SIZE) << PAGE_BITSIZE);

    //debug_serial_printf("New PTE: 0x%x\n", PTE);

    /*
        Walk through page tables to find PT to insert PTE into
    */
    uint64_t* PML4_virtAddr = (uint64_t*)translateaddr_p2v((uint64_t)_vmm_PML4_physAddr);
    uint64_t PDP_physAddr = PML4_virtAddr[va_PML4_offset] & (~flagFilter);
    if(PDP_physAddr == 0x0){
        PML4_virtAddr[va_PML4_offset] = (uint64_t)pmm_alloc_pages(1);
        PDP_physAddr = PML4_virtAddr[va_PML4_offset] & (~flagFilter);
        PML4_virtAddr[va_PML4_offset] |= flags;
    }

    uint64_t* PDP_virtAddr = (uint64_t*)translateaddr_p2v((uint64_t)PDP_physAddr);
    uint64_t PD_physAddr = PDP_virtAddr[va_PDP_offset] & (~flagFilter);
    if(PD_physAddr == 0x0){
        PDP_virtAddr[va_PDP_offset] = (uint64_t)pmm_alloc_pages(1);
        PD_physAddr = PDP_virtAddr[va_PDP_offset];
        PDP_virtAddr[va_PDP_offset] |= flags;
    }

    uint64_t* PD_virtAddr = (uint64_t*)translateaddr_p2v((uint64_t)PD_physAddr);
    uint64_t PT_physAddr = PD_virtAddr[va_PD_offset] & (~flagFilter);
    if(PT_physAddr == 0x0){
        PD_virtAddr[va_PD_offset] = (uint64_t)pmm_alloc_pages(1);
        PT_physAddr = PD_virtAddr[va_PD_offset];
        PD_virtAddr[va_PD_offset] |= flags;
    }

    uint64_t* PT_virtAddr = (uint64_t*)translateaddr_p2v((uint64_t)PT_physAddr);
    PT_virtAddr[va_PT_offset] = PTE;


    return 0;
}

void vmm_switchCR3(){
    asm volatile("mov %0, %%cr3" :: "r"((uint64_t)_vmm_PML4_physAddr));
    g_vmm_usingLiminePageTables = false;
}