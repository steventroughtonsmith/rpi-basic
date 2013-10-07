//
//  rpi-basic
//
//  Copyright 2013 Steven Troughton-Smith. All rights reserved.
//

#include <stdarg.h>
#include <string.h>
#include <stdint.h>

#define __COMMON_H_
#define __MATH_H_
#define __MEM_H_

typedef		unsigned long		ulong;
typedef		unsigned int		uint32;
typedef		unsigned short		uint16;
typedef		unsigned char		byte;

#include "raspberrylib.h"
#include "console.h"
#include "uart.h"

using namespace RaspberryLib;

extern Console *shared_console;

extern "C" void printf(char* fmt, ...)
{
	va_list va;
	va_start(va,fmt);
	shared_console->kprintf(fmt,va);
	va_end(va);
}

extern "C" void printfv(char* fmt, ...)
{
	va_list va;
	va_start(va,fmt);
	uart_printf(fmt,va);
	va_end(va);
}

extern "C" void split_putc(char c)
{
	printf("SPLIT:%c", c);
}

#define MBOX_BASE 0x2000b880

#define MBOX_PEEK 0x10
#define MBOX_READ 0x00
#define MBOX_WRITE 0x20
#define MBOX_STATUS 0x18
#define MBOX_SENDER 0x14
#define MBOX_CONFIG 0x1c

#define MBOX_FB		1
#define MBOX_PROP	8

#define MBOX_SUCCESS	0x80000000

#define MBOX_FULL		0x80000000
#define	MBOX_EMPTY		0x40000000

extern "C" void memory_barrier();

inline void mmio_write(uint32_t reg, uint32_t data)
{
	memory_barrier();
	*(volatile uint32_t *)(reg) = data;
	memory_barrier();
}

inline uint32_t mmio_read(uint32_t reg)
{
	memory_barrier();
	return *(volatile uint32_t *)(reg);
	memory_barrier();
}

extern "C" uint32_t mbox_read(uint8_t channel)
{
	while(1)
	{
		while(mmio_read(MBOX_BASE + MBOX_STATUS) & MBOX_EMPTY);
		
		uint32_t data = mmio_read(MBOX_BASE + MBOX_READ);
		uint8_t read_channel = (uint8_t)(data & 0xf);
		if(read_channel == channel)
			return (data & 0xfffffff0);
	}
}

extern "C" void mbox_write(uint8_t channel, uint32_t data)
{
	while(mmio_read(MBOX_BASE + MBOX_STATUS) & MBOX_FULL);
	mmio_write(MBOX_BASE + MBOX_WRITE, (data & 0xfffffff0) | (uint32_t)(channel & 0xf));
}
