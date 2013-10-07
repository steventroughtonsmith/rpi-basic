//
//  rpi-basic
//
//  Copyright 2013 Steven Troughton-Smith. All rights reserved.
//

#include <stdarg.h>
#include <string.h>

#include <stdarg.h>

#include "raspberrylib.h"

#define GPFSEL1 0x20200004
#define GPSET0  0x2020001C
#define GPCLR0  0x20200028
#define GPPUD       0x20200094
#define GPPUDCLK0   0x20200098



#define UART0_BASE   0x20201000
#define UART0_DR     (UART0_BASE+0x00)
#define UART0_RSRECR (UART0_BASE+0x04)
#define UART0_FR     (UART0_BASE+0x18)
#define UART0_ILPR   (UART0_BASE+0x20)
#define UART0_IBRD   (UART0_BASE+0x24)
#define UART0_FBRD   (UART0_BASE+0x28)
#define UART0_LCRH   (UART0_BASE+0x2C)
#define UART0_CR     (UART0_BASE+0x30)
#define UART0_IFLS   (UART0_BASE+0x34)
#define UART0_IMSC   (UART0_BASE+0x38)
#define UART0_RIS    (UART0_BASE+0x3C)
#define UART0_MIS    (UART0_BASE+0x40)
#define UART0_ICR    (UART0_BASE+0x44)
#define UART0_DMACR  (UART0_BASE+0x48)
#define UART0_ITCR   (UART0_BASE+0x80)
#define UART0_ITIP   (UART0_BASE+0x84)
#define UART0_ITOP   (UART0_BASE+0x88)
#define UART0_TDR    (UART0_BASE+0x8C)


extern "C" void dummy(unsigned int);

void uart_printf( const char* fmt, ...)
{
	const char *p;
	int i;
	va_list agrp;
	char *s = NULL;
	
	va_start(agrp, fmt);
	
	for(p = fmt; *p != '\0'; p++)
	{
		if(*p != '%')
		{
			uart_putc(*p);
			continue;
		}
		
		switch(*++p)
		{
			case 'c':
				i = va_arg(agrp, int);
				uart_putc(i);
				break;
				
			case 'd':
				i = va_arg(agrp, int);
				//this->kbase( (mint)i, 10 );
				break;
				
			case 's':
				s = va_arg(agrp, char *);
				//this->kprint(s);
				break;
				
			case 'x':
				i = va_arg(agrp, int);
				//this->kbase( (mint)i, 16 );
				break;
				
			case '%':
				uart_putc(*p);
				break;
		}
	}
	
	va_end(agrp);

	
}

void uart_putc ( unsigned int c )
{
	while(1)
	{
		if((RaspberryLib::GET32(UART0_FR)&0x20)==0) break;
	}
	RaspberryLib::PUT32(UART0_DR,c);
	
}
//------------------------------------------------------------------------
unsigned int uart_getc ( void )
{
	while(1)
	{
		if((RaspberryLib::GET32(UART0_FR)&0x10)==0) break;
	}
	return(RaspberryLib::GET32(UART0_DR));
}
//------------------------------------------------------------------------
void uart_init ( void )
{
	unsigned int ra;
	
	RaspberryLib::PUT32(UART0_CR,0);
	
	ra=RaspberryLib::GET32(GPFSEL1);
	ra&=~(7<<12); //gpio14
	ra|=4<<12;    //alt0
	ra&=~(7<<15); //gpio15
	ra|=4<<15;    //alt0
	RaspberryLib::PUT32(GPFSEL1,ra);
	
	RaspberryLib::PUT32(GPPUD,0);
	for(ra=0;ra<150;ra++) dummy(ra);
	RaspberryLib::PUT32(GPPUDCLK0,(1<<14)|(1<<15));
	for(ra=0;ra<150;ra++) dummy(ra);
	RaspberryLib::PUT32(GPPUDCLK0,0);
	
	RaspberryLib::PUT32(UART0_ICR,0x7FF);
	RaspberryLib::PUT32(UART0_IBRD,1);
	RaspberryLib::PUT32(UART0_FBRD,40);
	RaspberryLib::PUT32(UART0_LCRH,0x70);
	RaspberryLib::PUT32(UART0_CR,0x301);
}

