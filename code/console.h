#ifndef __CONSOLE_H_
#define __CONSOLE_H_

#include "common.h"
#include "math.h"
#include "raspberrylib.h"
#include "gpu2d.h"
#include <stdarg.h>


#define TILE_HEIGHT 16
#define TILE_WIDTH 8
#define TILE_DISPLAY_HEIGHT TILE_HEIGHT

#define BACKGROUND_OFFSET_X 8
#define BACKGROUND_OFFSET_Y 4

#define BORDER_COLOR 0xd52e53
#define BACKGROUND_COLOR 0x650525
#define CURSOR_COLOR BORDER_COLOR

//#define 	MAX_CHAR_PER_LINE		120
//#define		MAX_LINE				45

#define 	MAX_CHAR_PER_LINE		70
#define		MAX_LINE				24

class Console {
	public:

		void kprintf( const char* fmt, va_list agrp);
		
		// Standard printf functions.
		void kprint( char* string );
		void kprint( const char* string );
		void kbase( long value, long base, long size );
		void kbase( long value, long base );
	
		// Really useless functions
		void kout( const char* string );
		
		// Clearscreen function.
		void clear( void );
		
		// Constructor.
		Console( gpu2dCanvas* surface );
		
	private:
		uint32 charx;
		uint32 chary;
		uint32 padding;
		gpu2dCanvas* canvas;
		void printChar( char c, uint32 color );
		void newLine( void );
};

#endif
