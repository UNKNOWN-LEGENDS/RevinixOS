#include "kprintf.h"
#include "drivers/serial.h"
#include<stdint.h>
#include<stdarg.h>
#include "drivers/vga.h"

static void emit_char(char c) {
    serial_write_char(c);
    vga_putc(c);
}
static void emit_str(const char* s) {
    for (int i = 0; s[i]; i++) emit_char(s[i]);
}

//print an unsigned number in the given base (10 or 16)
static void print_uint(uint64_t value, int base) {
	char buf[32];
	const char* digits = "0123456789abcdef";
	int i=0;

	if (value == 0) {
		emit_char('0');
		return;
	}
	while (value > 0) {
		buf[i++] = digits[value % base];
		value/=base;
	}
	while (i > 0) {
		emit_char(buf[--i]); //digits were generated in reverse
	}
}

static void print_int(int64_t value) {
	if (value < 0) {
		emit_char('-');
		print_uint((uint64_t)(-value), 10);
	} else {
		print_uint((uint64_t)value, 10);
	}
}

void kprintf(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);

	for (int i=0; fmt[i] != '\0'; i++) {
		if (fmt[i] != '%') {
			emit_char(fmt[i]);
			continue;
		}
		i++; //skip '%'
		switch (fmt[i]) {
			case 's': emit_str(va_arg(args, const char*)); break;
			case 'd': print_int(va_arg(args, int)); break;
			case 'x': print_uint(va_arg(args, unsigned int), 16); break;
			case 'c': emit_char((char)va_arg(args, int)); break;
			case 'p': {
				emit_str("0x");
				print_uint((uint64_t)va_arg(args, void*), 16);
				break;
			}
			case '%': emit_char('%'); break;
			default: emit_char('%'); emit_char(fmt[i]); break;
		}
	}
	va_end(args);
}
