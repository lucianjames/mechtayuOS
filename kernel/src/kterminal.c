#include "kterminal.h"

int G_KTERM_CROW = 0;
int G_KTERM_MAXROW = 0;
int G_KTERM_MAXCOL = 0;
struct limine_framebuffer* G_KTERM_FRAMEBUFF;

void kterm_init(const struct limine_framebuffer* framebuffer){
    G_KTERM_FRAMEBUFF = framebuffer;

    /*
        Figure out the maximum number of rows we can put on the screen
    */
    psf1_header* psf_info = (psf1_header*)&_binary_zap_vga09_psf_start;
    G_KTERM_MAXROW = G_KTERM_FRAMEBUFF->height / psf_info->charsize + 1;
    G_KTERM_MAXCOL = G_KTERM_FRAMEBUFF->width / 9;
}


void kterm_scroll_check(){
    /*
        Handle when there are no rows left.
        This is just a temporary shitty solution for now
    */
    if(G_KTERM_CROW >= G_KTERM_MAXROW){
        kterm_clear();
        G_KTERM_CROW = 0;
    }
}

void kterm_write_newline(const char* str){
    /*
        TODO handle strings that wont fit onto one line
    */
    draw_psf_str(G_KTERM_FRAMEBUFF, G_KTERM_CROW*(((psf1_header*)&_binary_zap_vga09_psf_start)->charsize+1), 0, str);
    G_KTERM_CROW++;
    kterm_scroll_check();
}


int kterm_printuint(int col, uint64_t uint_to_write, int base){
    if(uint_to_write == 0){
        draw_psf_char(G_KTERM_FRAMEBUFF, G_KTERM_CROW*(((psf1_header*)&_binary_zap_vga09_psf_start)->charsize+1), col*8, '0');
        return col++;
    }

    char str[65];
    int str_idx = 0;
    while(uint_to_write != 0){
        int r = uint_to_write % base;
        str[str_idx++] = (r>9)? (r-10)+'a' : r + '0';
        uint_to_write = uint_to_write / base;
    }

    for(int i=str_idx-1; i>=0; i--){
        draw_psf_char(G_KTERM_FRAMEBUFF, G_KTERM_CROW*(((psf1_header*)&_binary_zap_vga09_psf_start)->charsize+1), col*8, str[i]);
        col++;
    }

    return col-1;
}


/*
    %u = uint32 b10
    %x = uint32 hex
*/
void kterm_printf_newline(const char* fmt, ...){
    int col = 0;
    va_list args;
    va_start(args, fmt);
    for(int i=0; fmt[i]!='\0'; i++){
        if(fmt[i] == '%'){
            i++;
            switch(fmt[i]){
                case 'u':
                {
                    col = kterm_printuint(col, va_arg(args, uint64_t), 10);
                    break;
                }
                case 'x':
                {
                    col = kterm_printuint(col, va_arg(args, uint64_t), 16);
                    break;
                }
                case 's':
                {   
                    int si = 0;
                    const char* sstr = va_arg(args, const char*);
                    while(sstr[si] != 0){
                        draw_psf_char(G_KTERM_FRAMEBUFF, G_KTERM_CROW*(((psf1_header*)&_binary_zap_vga09_psf_start)->charsize+1), col*8, sstr[si]);
                        si++;
                        col++;
                    }
                    col--;
                    break;
                }
            }
        }else{
            /*
                TODO handle strings that wont fit onto one line
            */
            draw_psf_char(G_KTERM_FRAMEBUFF, G_KTERM_CROW*(((psf1_header*)&_binary_zap_vga09_psf_start)->charsize+1), col*8, fmt[i]);
        }
        col++;
    }
    va_end(args);
    G_KTERM_CROW++;
    kterm_scroll_check();
}


void kterm_clear(){
    char* framebuffer = (char*)(unsigned long)G_KTERM_FRAMEBUFF->address;
    size_t fb_size = G_KTERM_FRAMEBUFF->height * G_KTERM_FRAMEBUFF->pitch;
    char* fb_max = framebuffer+fb_size;
    uint32_t colour_black = 0x0;

    /*
        This is terribly optimised and bad
    */
    while(framebuffer<fb_max){
        *(uint32_t*)(framebuffer) = colour_black;
        framebuffer += 4;
    }
}