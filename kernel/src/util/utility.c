#include "utility.h"

/*
    Halt execution permanently
*/
void khalt(void) {
    for (;;) {
        asm ("hlt");
    }
}