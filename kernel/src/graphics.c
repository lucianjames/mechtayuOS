#include "graphics.h"

void serial_dump_psf_info(){
    writestr_debug_serial("INFO: Dumping PSF information...\n");
    writestr_debug_serial(" _binary_zap_vga09_psf_start: 0x");
    writeuint_debug_serial((uint64_t)&_binary_zap_vga09_psf_start, 16);
    writestr_debug_serial("\n _binary_zap_vga09_psf_end: 0x");
    writeuint_debug_serial((uint64_t)&_binary_zap_vga09_psf_end, 16);
    writestr_debug_serial("\n");
    psf1_header* psf = (psf1_header*)&_binary_zap_vga09_psf_start;
    if(psf->magic[0] == PSF1_MAGIC0 && psf->magic[1] == PSF1_MAGIC1){
        writestr_debug_serial(" PSF1 Magic: Valid (0x36, 0x04)\n");
    }else{
        writestr_debug_serial("ERROR: PSF1 magic NOT valid\n");
    }
    writestr_debug_serial(" PSF mode: 0x");
    writeuint_debug_serial(psf->mode, 16);
    writestr_debug_serial("\n PSF charsize: ");
    writeuint_debug_serial(psf->charsize, 10);
    writestr_debug_serial("\n");
}

/*
    Render a character to the screen at the given offset.
    zap-vga09.pdf shows the table of characters available, and their offsets (for setting char_idx)
*/
void draw_psf_char(const struct limine_framebuffer* framebuffer, int row_offset, int col_offset, int char_idx){
    /*
        Set up the pointers required to draw a character
    */
    psf1_header* header = (psf1_header*)&_binary_zap_vga09_psf_start;
    uint8_t* char_start = (uint8_t*)&_binary_zap_vga09_psf_start + 4 + (char_idx*9);

    /*
        Column offset changes with different BPP values
    */
    int col_pixel_increment = 0;
    switch(framebuffer->bpp){
        case(32):
            col_pixel_increment = 4;
            break;
        case(24):
            col_pixel_increment = 3;
            break;
    }
    
    /*
        Create some very pretty colours
    */
    uint32_t colour_white = 0xFFFFFFFF >> (32 - framebuffer->bpp);
    uint32_t colour_black = 0x0;

    /*
        Draw the character!
        This isnt optimised very well.
    */
    for(int row=0; row<header->charsize; row++){
        for(int col=0; col<8; col++){
            uint32_t* pixel = (uint32_t*)(framebuffer->address + ((row+row_offset)*framebuffer->pitch) + (col+col_offset)*col_pixel_increment);
            *pixel = (*(char_start+row) << col & 0x80)? colour_white : colour_black; // 0x80 == 0b10000000
        }
    }
}

void draw_psf_str(const struct limine_framebuffer* framebuffer, int row_offset, int col_offset, const char* str){
    for(int i=0; str[i]!=0x00; i++){
        draw_psf_char(framebuffer, row_offset, col_offset+(i*9), str[i]);
    }
}

void draw_psf_debug_matrix(const struct limine_framebuffer* framebuffer, int row_offset, int col_offset){
    unsigned char current_char = 0x00;
    for(int i=0; i<16; i++){
        for(int j=0; j<16; j++){
            draw_psf_char(framebuffer, row_offset+(i*10), col_offset+(j*10), current_char);
            current_char++;
        }
    }
}

