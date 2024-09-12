#ifndef VMM_H
#define VMM_H

#include <stddef.h>
#include <stdbool.h>
#include "limine.h"

#include "constants.h"

#include "utility.h"
#include "serialout.h"
#include "pmm.h"


extern bool g_vmm_usingLiminePageTables;

uint64_t translateaddr_v2p_limine(uint64_t lvaddr);
uint64_t translateaddr_p2v_limine(uint64_t addr);

uint64_t translateaddr_v2p(uint64_t vaddr);
uint64_t translateaddr_p2v(uint64_t addr);

void _vmm_internaL_zeropage(uint64_t* page);

void vmm_setup();
int vmm_map_phys2virt(uint64_t phys_addr, uint64_t virt_addr, uint64_t flags);
void vmm_switchCR3();

#endif