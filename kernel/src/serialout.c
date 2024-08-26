#include "serialout.h"


uint8_t asm_inline_inb(uint16_t port) {
    uint8_t data;
    asm volatile ("inb %1, %0" 
                    : "=a" (data) 
                    : "Nd" (port)
    );
    return data;
}


void asm_inline_outb(uint16_t port, uint8_t data) {
    asm volatile ("outb %0, %1" 
                    :
                    : "a" (data), "Nd" (port)
    );
}


int init_debug_serial() {
    asm_inline_outb(SERIAL_DEBUG_COM_PORT + 1, 0x00);    // Disable all interrupts
    asm_inline_outb(SERIAL_DEBUG_COM_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    asm_inline_outb(SERIAL_DEBUG_COM_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    asm_inline_outb(SERIAL_DEBUG_COM_PORT + 1, 0x00);    //                  (hi byte)
    asm_inline_outb(SERIAL_DEBUG_COM_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    asm_inline_outb(SERIAL_DEBUG_COM_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    asm_inline_outb(SERIAL_DEBUG_COM_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set

    /*
        Perform serial loopback test
    */
    asm_inline_outb(SERIAL_DEBUG_COM_PORT + 4, 0x1E);      // Set in loopback mode, test the serial chip
    asm_inline_outb(SERIAL_DEBUG_COM_PORT, SERIAL_TEST_BYTE);          // Send test data
    if(asm_inline_inb(SERIAL_DEBUG_COM_PORT) != SERIAL_TEST_BYTE) {
        return 1;
    }
    asm_inline_outb(SERIAL_DEBUG_COM_PORT + 4, SERIAL_NORMAL_MODE);      // Set serial back to normal operation mode
    return 0;
}


int is_debug_serial_received() {
    return asm_inline_inb(SERIAL_DEBUG_COM_PORT + 5) & 0x01;
}


char read_debug_serial() {
    while (is_debug_serial_received() == 0){
        // Wait until there is a byte to receive
    }
    return asm_inline_inb(SERIAL_DEBUG_COM_PORT);
}


int is_debug_transmit_empty() {
   return asm_inline_inb(SERIAL_DEBUG_COM_PORT + 5) & 0x20;
}


void write_debug_serial(char a) {
   while (is_debug_transmit_empty() == 0){
    // Wait until the "transit" of the com port is empty, allowing space for this function to write a byte
   }
 
   asm_inline_outb(SERIAL_DEBUG_COM_PORT,a);
}


void writestr_debug_serial(const char* str){
    for(size_t i=0; str[i] !='\0'; i++){
        write_debug_serial(str[i]);
    }
}


void writeuint_debug_serial(uint64_t uint_to_write, int base){
    if(uint_to_write == 0){
        write_debug_serial('0');
        return;
    }

    char str[65];

    int str_idx = 0;
    while(uint_to_write != 0){
        int r = uint_to_write % base;
        str[str_idx++] = (r>9)? (r-10)+'a' : r + '0';
        uint_to_write = uint_to_write / base;
    }

    for(int i=str_idx-1; i>=0; i--){
        write_debug_serial(str[i]);
    }
}


void debug_serial_printf(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    for(int i=0; fmt[i]!='\0'; i++){
        if(fmt[i] == '%'){
            i++;
            switch(fmt[i]){
                case 'u':
                {
                    writeuint_debug_serial(va_arg(args, uint64_t), 10);
                    break;
                }
                case 'x':
                {
                    writeuint_debug_serial(va_arg(args, uint64_t), 16);
                    break;
                }
                case 's':
                {
                    writestr_debug_serial(va_arg(args, const char*));
                    break;
                }
            }
        }else{
            write_debug_serial(fmt[i]);
        }
    }
    va_end(args);
}