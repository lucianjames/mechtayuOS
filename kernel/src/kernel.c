#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "limine.h"

#include "serialout.h"
#include "graphics.h"

// Set the base revision to 2, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.
__attribute__((used, section(".requests")))
static volatile LIMINE_BASE_REVISION(2);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.
__attribute__((used, section(".requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.
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
    
    writestr_debug_serial("======================\n");
    writestr_debug_serial("Hello from the kernel!\n");
    writestr_debug_serial("======================\n");
    
    serial_dump_psf_info();
    draw_psf_debug_matrix(framebuffer_request.response->framebuffers[0], 128, 128);
    draw_psf_str(framebuffer_request.response->framebuffers[0], 0, 0, "Hello from the kernel!");

    khalt();
}
