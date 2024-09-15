
;In Long Mode, the length of the Base field is 8 bytes, rather than 4. 
;As well, the System V ABI passes the first two arguments via the RDI and RSI registers. 
;Thus, this example code can be called as setGdt(limit, base). 
;As well, only a flat model is possible in long mode, so no considerations have to be made otherwise.

global gdt_set_gdt

gdtr DW 0 ; For limit storage
     DQ 0 ; For base storage

gdt_set_gdt: ; setGdt(limit, base)
   mov [gdtr], DI
   mov [gdtr+2], RSI
   lgdt [gdtr]
   ret
