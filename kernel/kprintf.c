#include "kprintf.h"
#include "drivers/serial.h"
#include<stdint.h>
#include<stdarg.h>
#include "drivers/vga.h"

// ---- kernel log ring buffer ----
static char     klog_buf[KLOG_SIZE];
static uint64_t klog_total = 0;      // bytes ever logged (index = total % size)

// Screen + serial only. Not recorded.
static void out_char(char c) {
    serial_write_char(c);
    vga_putc(c);
}

// Every kprintf character goes through here: record it, then show it.
static void emit_char(char c) {
    klog_buf[klog_total % KLOG_SIZE] = c;
    klog_total++;
    out_char(c);
}

void console_write(const char* s, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) out_char(s[i]);
}

int klog_wrapped(void) {
    return klog_total > KLOG_SIZE;
}

uint32_t klog_snapshot(char* dst, uint32_t max) {
    // Interrupts off while copying, so an IRQ that prints can't change the
    // ring under us. Save/restore RFLAGS so we don't enable IRQs that were off.
    uint64_t flags;
    __asm__ volatile ("pushfq; pop %0; cli" : "=r"(flags) : : "memory");

    uint64_t total = klog_total;
    uint64_t count = total < KLOG_SIZE ? total : KLOG_SIZE;
    uint64_t start = total - count;              // oldest byte still stored

    // Wrapped: the oldest bytes are the tail of a partly-overwritten line.
    // Skip forward past the first newline so the dump starts on a clean line.
    if (total > KLOG_SIZE) {
        while (count > 0 && klog_buf[start % KLOG_SIZE] != '\n') { start++; count--; }
        if (count > 0) { start++; count--; }
    }
    if (count > max) { start += count - max; count = max; }

    for (uint64_t i = 0; i < count; i++)
        dst[i] = klog_buf[(start + i) % KLOG_SIZE];

    __asm__ volatile ("push %0; popfq" : : "r"(flags) : "memory", "cc");
    return (uint32_t)count;
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
