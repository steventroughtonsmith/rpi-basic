// *******************************
// FILE: 		gpu2d.cpp
// AUTHOR:		SharpCoder
// DATE:		2012-03-28
// ABOUT:		This is the 2D graphics engine (re written) for my
//				raspberry pi kernel. I'm trying to implement some
//				nicer functions and a backbuffer to make everything
//				smoother.
//
// LICENSE:		Provided "AS IS". USE AT YOUR OWN RISK.
// *******************************
#include "gpu2d.h"
#include "vga.h"
#include "console.h"
#include "uart.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480


static uint32 phys_w, phys_h, virt_w, virt_h, pitch;
static uint32 fb_addr, fb_size;


gpu2dCanvas::gpu2dCanvas( ) {
	gpu2dCanvas( true );
}


#define FB_FAIL_GET_RESOLUTION		-1
#define FB_FAIL_INVALID_RESOLUTION	-2
#define FB_FAIL_SETUP_FB		-3
#define FB_FAIL_INVALID_TAGS		-4
#define FB_FAIL_INVALID_TAG_RESPONSE	-5
#define FB_FAIL_INVALID_TAG_DATA	-6
#define FB_FAIL_INVALID_PITCH_RESPONSE	-7
#define FB_FAIL_INVALID_PITCH_DATA	-8


#define WIDTH		640
#define HEIGHT		480

#define BYTES_PER_PIXEL	3

#define BPP		(BYTES_PER_PIXEL << 3)
#define S_PITCH		(WIDTH * BYTES_PER_PIXEL)	// The pitch of the backbuffer

#define TAG_ALLOCATE_BUFFER		0x40001
#define TAG_RELEASE_BUFFER		0x48001
#define TAG_BLANK_SCREEN		0x40002
#define TAG_GET_PHYS_WH			0x40003
#define TAG_TEST_PHYS_WH		0x44003
#define TAG_SET_PHYS_WH			0x48003
#define TAG_GET_VIRT_WH			0x40004
#define TAG_TEST_VIRT_WH		0x44004
#define TAG_SET_VIRT_WH			0x48004
#define TAG_GET_DEPTH			0x40005
#define TAG_TEST_DEPTH			0x44005
#define TAG_SET_DEPTH			0x48005
#define TAG_GET_PIXEL_ORDER		0x40006
#define TAG_TEST_PIXEL_ORDER		0x44006
#define TAG_SET_PIXEL_ORDER		0x48006
#define TAG_GET_ALPHA_MODE		0x40007
#define TAG_TEST_ALPHA_MODE		0x44007
#define TAG_SET_ALPHA_MODE		0x48007
#define TAG_GET_PITCH			0x40008
#define TAG_GET_VIRT_OFFSET		0x40009
#define TAG_TEST_VIRT_OFFSET		0x44009
#define TAG_SET_VIRT_OFFSET		0x48009
#define TAG_GET_OVERSCAN		0x4000a
#define TAG_TEST_OVERSCAN		0x4400a
#define TAG_SET_OVERSCAN		0x4800a
#define TAG_GET_PALETTE			0x4000b
#define TAG_TEST_PALETTE		0x4400b
#define TAG_SET_PALETTE			0x4800b

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

extern "C" uint32 mbox_read(char channel);
extern "C" void mbox_write(char channel, uint32 data);

int fb_init()
{
	// define a mailbox buffer
	uint32 mb_addr = 0x40007000;		// 0x7000 in L2 cache coherent mode
	volatile uint32 *mailbuffer = (uint32 *)mb_addr;
	
	/* Get the display size */
	// set up the buffer
	mailbuffer[0] = 8 * 4;			// size of this message
	mailbuffer[1] = 0;			// this is a request
	
	// next comes the first tag
	mailbuffer[2] = TAG_GET_PHYS_WH;	// get physical width/height tag
	mailbuffer[3] = 0x8;			// value buffer size
	mailbuffer[4] = 0;			// request/response
	mailbuffer[5] = 0;			// space to return width
	mailbuffer[6] = 0;			// space to return height
	
	// closing tag
	mailbuffer[7] = 0;
	
	// send the message
	mbox_write(MBOX_PROP, mb_addr);
	
	// read the response
	mbox_read(MBOX_PROP);
	
	/* Check for a valid response */
	if(mailbuffer[1] != MBOX_SUCCESS)
		return FB_FAIL_GET_RESOLUTION;
	phys_w = mailbuffer[5];
	phys_h = mailbuffer[6];
	
	/* Request 640x480 if not otherwise specified */
	if((phys_w == 0) && (phys_h == 0))
	{
		phys_w = WIDTH;
		phys_h = HEIGHT;
	}
	
	if((phys_w == 0) || (phys_h == 0))
		return FB_FAIL_INVALID_RESOLUTION;
	
	/* For now set the physical and virtual sizes to be the same */
	virt_w = phys_w;
	virt_h = phys_h;
	
	/* Now set the physical and virtual sizes and bit depth and allocate the framebuffer */
	mailbuffer[0] = 22 * 4;		// size of buffer
	mailbuffer[1] = 0;		// request
	
	mailbuffer[2] = TAG_SET_PHYS_WH;
	mailbuffer[3] = 8;
	mailbuffer[4] = 8;
	mailbuffer[5] = phys_w;
	mailbuffer[6] = phys_h;
	
	mailbuffer[7] = TAG_SET_VIRT_WH;
	mailbuffer[8] = 8;
	mailbuffer[9] = 8;
	mailbuffer[10] = virt_w;
	mailbuffer[11] = virt_h;
	
	mailbuffer[12] = TAG_SET_DEPTH;
	mailbuffer[13] = 4;
	mailbuffer[14] = 4;
	mailbuffer[15] = BPP;
	
	mailbuffer[16] = TAG_ALLOCATE_BUFFER;
	mailbuffer[17] = 8;
	mailbuffer[18] = 4;	// request size = 4, response size = 8
	mailbuffer[19] = 16;	// requested alignment of buffer, space for returned address
	mailbuffer[20] = 0;	// space for returned size
	
	mailbuffer[21] = 0;	// terminating tag
	
	mbox_write(MBOX_PROP, mb_addr);
	mbox_read(MBOX_PROP);
	
	/* Validate the response */
	if(mailbuffer[1] != MBOX_SUCCESS)
		return FB_FAIL_SETUP_FB;
	
	/* Check the allocate_buffer response */
	if(mailbuffer[18] != (MBOX_SUCCESS | 8))
		return FB_FAIL_INVALID_TAG_RESPONSE;
	
	fb_addr = mailbuffer[19];
	fb_size = mailbuffer[20];
	
	if((fb_addr == 0) || (fb_size == 0))
		return FB_FAIL_INVALID_TAG_DATA;
	
	/* Get the pitch of the display */
	mailbuffer[0] = 7 * 4;
	mailbuffer[1] = 0;
	
	mailbuffer[2] = TAG_GET_PITCH;
	mailbuffer[3] = 4;
	mailbuffer[4] = 0;
	mailbuffer[5] = 0;
	
	mailbuffer[6] = 0;
	
	mbox_write(MBOX_PROP, mb_addr);
	mbox_read(MBOX_PROP);
	
	/* Validate the response */
	if(mailbuffer[1] != MBOX_SUCCESS)
		return FB_FAIL_INVALID_PITCH_RESPONSE;
	if(mailbuffer[4] != (MBOX_SUCCESS | 4))
		return FB_FAIL_INVALID_PITCH_RESPONSE;
	
	pitch = mailbuffer[5];
	if(pitch == 0)
		return FB_FAIL_INVALID_PITCH_DATA;
	
	return 0;
}


// BASIC CONSTRUCTOR
gpu2dCanvas::gpu2dCanvas( bool useDoubleBuffer ) {
	
	// Determine whether to use the double buffer system.
	this->enableDoubleBuffer = useDoubleBuffer;
	
	// Setup invalid state first.
	this->valid = false;
	
	// Create a framebuffer request object.
	this->fbInfo = (FB_Info*)KERNEL_FB_LOC;
	
	// Setup some information about the canvas.
	this->fbInfo->screen_width = SCREEN_WIDTH;
	this->fbInfo->screen_height = SCREEN_HEIGHT;
	this->fbInfo->virtual_width = this->fbInfo->screen_width;
	
	// If we're using double buffer...
	if ( this->enableDoubleBuffer )
		this->fbInfo->virtual_height = this->fbInfo->screen_height * 2;
	else
		this->fbInfo->virtual_height = this->fbInfo->screen_height;
	
	this->fbInfo->depth = 24;
	this->fbInfo->xoffset = 0;
	this->fbInfo->yoffset = 0;
	this->fbInfo->pitch = 0;
	this->fbInfo->framePtr = 0;
	this->fbInfo->bufferSize = 0;
	
	// Initialize the framebuffer stuff.
	if ( !initFrameBuffer() ) {
		return;
	}
	
	// set the "switched" to false.
	// This determines which part of the virtual buffer to read/write to.
	this->switched = false;
	
}

bool gpu2dCanvas::initFrameBuffer( void ) {
#if 1
	// Crete the request data.
	uint32 requestData = RaspberryLib::Memory::PHYSICAL_TO_BUS( (uint32)fbInfo );
	// Send the request.
	RaspberryLib::MailboxWrite( 1, requestData );
	
	// Hold until we get a result.
	uint32 result = 0xFF;
	do {
		result = RaspberryLib::MailboxCheck( 1 );
	} while ( result != 0 );
	
	// If we've got a result, validate the data.
	if ( this->fbInfo->framePtr == 0 ) return false;
	if ( this->fbInfo->pitch == 0 ) return false;
	
	// Fix the pointer data.
	this->fbInfo->framePtr = RaspberryLib::Memory::BUS_TO_PHYSICAL( this->fbInfo->framePtr );
	
	// Set our valid flag and return
	this->valid = true;
#else
	
	fb_init();
	
	this->fbInfo->framePtr = fb_addr;
	this->fbInfo->pitch = pitch;
	this->fbInfo->screen_width = virt_w;
	this->fbInfo->screen_height= virt_h;
	this->valid = true;
#endif
	return true;
}


void gpu2dCanvas::Draw( void ) {
	


	if ( !this->valid ) return;
	if (  !this->enableDoubleBuffer ) return;
	
	
	
	uint32 y = 0;
	if ( !this->switched )
		y = this->fbInfo->screen_height;
	
	this->fbInfo->yoffset = y;
	initFrameBuffer();
	
	// Toggle switched.
	this->switched = !this->switched;
	
}

void gpu2dCanvas::Clear( uint32 color ) {

	if ( !this->valid ) return;
	
	// Setup the variables.
	uint32 x = 0, y = 0, y_offset = 0, bufferOffset;
	if ( !this->switched && this->enableDoubleBuffer )
		y_offset = this->fbInfo->screen_height;
	
	// Calculate the colors.
	char r = ( color & 0xFF0000 ) >> 16;
	char g = ( color & 0x00FF00 ) >> 8;
	char b = ( color & 0x0000FF );
	

	// Iterate
	for ( y = 0; y < this->fbInfo->screen_height; y++ ) {

		for ( x = 0; x < this->fbInfo->screen_width; x++ ) {
			
			// Calculate the location in memory.
			bufferOffset = ( ( y + y_offset ) * this->fbInfo->pitch ) + ( x * 3 );
			
			// Update the data.
			*( (char*)( this->fbInfo->framePtr + bufferOffset ) ) = r;
			*( (char*)( this->fbInfo->framePtr + bufferOffset + 1 ) ) = g;
			*( (char*)( this->fbInfo->framePtr + bufferOffset + 2 ) ) = b;
			
		}
	}

	
	this->DrawBackground(BACKGROUND_COLOR);
}

void gpu2dCanvas::DrawBackground( uint32 color )
{
	
	if ( !this->valid ) return;
	// Setup the variables.
	uint32 x = 0, y = 0, y_offset = 0, bufferOffset;
	if ( !this->switched && this->enableDoubleBuffer )
		y_offset = this->fbInfo->screen_height;
	
	// Calculate the colors.
	char r = ( color & 0xFF0000 ) >> 16;
	char g = ( color & 0x00FF00 ) >> 8;
	char b = ( color & 0x0000FF );
	
	// Iterate
	for ( y = (TILE_HEIGHT*BACKGROUND_OFFSET_Y); y < (this->fbInfo->screen_height - (TILE_HEIGHT*BACKGROUND_OFFSET_Y)); y++ ) {
		for ( x = (TILE_WIDTH*BACKGROUND_OFFSET_X); x < (this->fbInfo->screen_width- (TILE_WIDTH*BACKGROUND_OFFSET_X)); x++ ) {
			
			// Calculate the location in memory.
			bufferOffset = ( ( y + y_offset ) * this->fbInfo->pitch ) + ( x * 3 );
			
			// Update the data.
			*( (char*)( this->fbInfo->framePtr + bufferOffset ) ) = r;
			*( (char*)( this->fbInfo->framePtr + bufferOffset + 1 ) ) = g;
			*( (char*)( this->fbInfo->framePtr + bufferOffset + 2 ) ) = b;
			
		}
	}
}

void gpu2dCanvas::sync( void ) {
	// If we're not in double buffer mode, don't even bother.
	if ( !this->enableDoubleBuffer ) return;
	
	// Copy all the things from one buffer to the other.
	uint32 x = 0, y = 0, y_offset = 0, bufferOffset;
	bool isSwitched = (this->switched);
	
	char* a;
	char* b;
	
	// Iterate
	for ( y = 0; y < this->fbInfo->screen_height; y++ ) {
		for ( x = 0; x < this->fbInfo->screen_width; x++ ) {
			
			// Do a double array copy.
			a = (char*)( this->fbInfo->framePtr + ( y * this->fbInfo->pitch ) + (x * 3) );
			b = (char*)( this->fbInfo->framePtr + ( ( y + this->fbInfo->screen_height ) * this->fbInfo->pitch ) + (x * 3) );
			
			// Figure out which one to copy from based on the buffer thing.
			if ( isSwitched ) {
				*b = *a;
			} else {
				*a = *b;
			}
		}
	}
}

void gpu2dCanvas::setPixel( uint32 x, uint32 y, uint32 color ) {
	// Calculate which buffer to use.
	uint32 y_offset = 0, bufferOffset = 0;
	if ( !this->switched  && this->enableDoubleBuffer ) {
		y_offset = this->fbInfo->screen_height;
	}
	// Calculate the buffer offset.
	bufferOffset = ( ( y + y_offset ) * this->fbInfo->pitch ) + ( x * 3 );
	
	// Calculate the colors.
	char r = ( color & 0xFF0000 ) >> 16;
	char g = ( color & 0x00FF00 ) >> 8;
	char b = ( color & 0x0000FF );
	
	// Set the pixel.
	*(char*)( this->fbInfo->framePtr + bufferOffset + 0 ) = r;
	*(char*)( this->fbInfo->framePtr + bufferOffset + 1 ) = g;
	*(char*)( this->fbInfo->framePtr + bufferOffset + 2 ) = b;
}

void gpu2dCanvas::DrawLine( int x1, int y1, int x2, int y2, uint32 color ) {
	
	int dx = x2 - x1;
	int dy = y2 - y1;
	int delta = (2 * dy) - dx;
	int y = y1, x = 0;
	
	// Iterate
	for ( x = x1; x < x2; x++ ) {
		if ( delta > 0 ) {
			y = y + 1;
			setPixel( x, y, color );
			delta = delta + ( 2 * dy - 2 * dx );
		} else {
			setPixel( x, y, color );
			delta = delta + ( 2 * dy );
		}
	}
}

static unsigned char getIndex(unsigned char c)
{
	if (c < ' ')
		return 0;
	
	unsigned char idx = 0;
	
	while(__font_index__[idx] != c && idx < (sizeof(__font_index__) / sizeof(__font_index__[0])))
		idx++;
	
	if(__font_index__[idx] != c)
	{
		idx = 0;
	}

	return idx;
}

#define TILE_HEIGHT 16
#define TILE_WIDTH 8
#define TILE_DISPLAY_HEIGHT TILE_HEIGHT

void gpu2dCanvas::DrawCharacter( int x, int y, char c, uint32 color )
{
	ClearCharacter(x,y, BACKGROUND_COLOR);
	
	unsigned char idx = getIndex(c);

	if (idx != 0)
	{
		for(unsigned char i = 0; i < TILE_HEIGHT; ++i)
		{
			unsigned char fontLine;
			fontLine = __font_bitmap__[(TILE_HEIGHT * idx + i)];
			for(unsigned char j = 0; j < 8; ++j)
			{
				setPixel ((x + j), (y + i), BACKGROUND_COLOR );
				if(fontLine & (1 << (7 - j)))
				{
					setPixel ((x + j), (y + i), color );
				}
			}
		}
	}
	else
	{
		//ClearCharacter(x,y, 0x000000);

	}
}

void gpu2dCanvas::ClearCharacter( int x, int y, uint32 color ) {
	for(unsigned char num1 = 0; num1 < TILE_HEIGHT; num1++ ) {
		for(unsigned char num2 = 0; num2 < TILE_WIDTH; num2++ ) {
			setPixel( x + num2, y + num1, color );
		}
	}
}
