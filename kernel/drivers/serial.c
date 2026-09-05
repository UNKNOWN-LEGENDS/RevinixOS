#include "serial.h"
#include<stdint.h>

#define COM1 0x3F8

// ---- low-level port I/O (x86 in/out instructions) ----
static inline void outb(uint16_t port, uint8_t val) {
	__asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	__asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

void serial_init(void) {
	outb(COM1 + 1, 0x00);		//disable interrupts
	outb(COM1 + 3, 0x80);		//enable DLAB (set baud rate divisor)
	outb(COM1 + 0, 0x03);		//divisor low byte (3 => 38400 baud)
	outb(COM1 + 1, 0x00);		//divisor high byte
	outb(COM1 + 3, 0x03);		//8 bits, no parity, one stop bit
	outb(COM1 + 2, 0xC7);		//enable FIFO, clear, 14-byte threshold
	outb(COM1 + 4, 0x08);		//IRQs enabled, RTS/DSR set
}

//wait until the transmit buffer is empty, then send
static int is_transmit_empty(void) {
	return inb(COM1 + 5) & 0x20;
}

void serial_write_char(char c) {
	if (c == '\n') {
		while (!is_transmit_empty());
		outb(COM1, '\r');
	}
	while (!is_transmit_empty());
	outb(COM1, c);
}

void serial_write_string(const char* s) {
	for (int i=0; s[i] != '\0'; i++) {
		serial_write_char(s[i]);
	}
}

