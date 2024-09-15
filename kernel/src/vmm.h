#ifndef VMM_H
#define VMM_H

#include <stddef.h>
#include <stdbool.h>
#include "limine.h"

#include "constants.h"

#include "utility.h"
#include "serialout.h"
#include "pmm.h"

#define VMM_IDENTITY_MAP_OFFSET 0x666000000000

extern bool g_vmm_usingLiminePageTables;

uint64_t translateaddr_idmap_v2p_limine(uint64_t lvaddr);
uint64_t translateaddr_idmap_p2v_limine(uint64_t addr);

uint64_t translateaddr_idmap_v2p(uint64_t vaddr);
uint64_t translateaddr_idmap_p2v(uint64_t addr);

void _vmm_internaL_zeropage(uint64_t* page);

void vmm_setup(const struct limine_memmap_response memmap_req);
uint64_t vmm_iterate_table(uint64_t table_physical_address, uint16_t offset, uint64_t flags);
uint64_t vmm_map_phys2virt(uint64_t phys_addr, uint64_t virt_addr, uint64_t flags);
uint64_t vmm_identity_map_page(uint64_t phys_addr, uint64_t flags);
uint64_t vmm_identity_map_n_pages(uint64_t phys_base_addr, int n_pages, uint64_t flags);
void vmm_switchCR3();

#endif