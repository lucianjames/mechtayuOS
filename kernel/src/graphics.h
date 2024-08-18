#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "limine.h" // For limine framebuffer info struct
#include "serialout.h"


#define PSF1_MAGIC0     0x36
#define PSF1_MAGIC1     0x04

#define PSF1_MODE512    0x01
#define PSF1_MODEHASTAB 0x02
#define PSF1_MODEHASSEQ 0x04
#define PSF1_MAXMODE    0x05

#define PSF1_SEPARATOR  0xFFFF
#define PSF1_STARTSEQ   0xFFFE

typedef struct {
        unsigned char magic[2];     /* Magic number */
        unsigned char mode;         /* PSF font mode */
        unsigned char charsize;     /* Character size */
} psf1_header;


extern uint64_t _binary_zap_vga09_psf_start;
extern uint64_t _binary_zap_vga09_psf_end;

void serial_dump_psf_info();
void draw_psf_char(const struct limine_framebuffer* framebuffer, int row_offset, int col_offset, int char_idx);
void draw_psf_str(const struct limine_framebuffer* framebuffer, int row_offset, int col_offset, const char* str);
void draw_psf_debug_matrix(const struct limine_framebuffer* framebuffer, int row_offset, int col_offset);

#endif