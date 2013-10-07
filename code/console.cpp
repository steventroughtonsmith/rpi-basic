
#include "console.h"
#include "uart.h"

void Console::newLine( void ) {
	this->chary++;
	this->charx = BACKGROUND_OFFSET_X;
	
	// Check for overflows.
	if ( this->chary > MAX_LINE ) {
		this->clear();
	}
}

void Console::printChar( char c, uint32 color ) {
		
	// Look at all the important characters.
	switch ( c ) {
		case '\n':
		{
			// Erase the cursor
			this->canvas->ClearCharacter( (this->charx) * TILE_WIDTH + this->padding,
										 this->chary * TILE_HEIGHT + this->padding, BACKGROUND_COLOR );
			this->newLine();
			
			// Cursor
			this->canvas->ClearCharacter( (this->charx) * TILE_WIDTH + this->padding,
										 this->chary * TILE_HEIGHT + this->padding, CURSOR_COLOR );
			// Refresh the screen.
			//this->canvas->Draw();
			return;
		}
		break;
		case '\b': 
			if ( this->charx > BACKGROUND_OFFSET_X ) {
				
				// Erase the cursor.
				this->canvas->ClearCharacter( (this->charx) * TILE_WIDTH + this->padding,
											 this->chary * TILE_HEIGHT + this->padding, BACKGROUND_COLOR );
				
				this->charx--;
				
				// Erase the character.
				this->canvas->ClearCharacter( this->charx * TILE_WIDTH + this->padding,
											  this->chary * TILE_HEIGHT + this->padding, BACKGROUND_COLOR );
				
				// Cursor
				this->canvas->ClearCharacter( (this->charx) * TILE_WIDTH + this->padding,
											 this->chary * TILE_HEIGHT + this->padding, CURSOR_COLOR );

				// Refresh
				//this->canvas->Draw();
			}
			return;
		break;
		case '\t':
			if ( this->charx + 4 < MAX_CHAR_PER_LINE ) {
				this->charx += 4;
			}
			return;
		break;
	}
	
	// Check for overflows.
	if ( this->charx > MAX_CHAR_PER_LINE ) {
		this->newLine();
	}
	
	// Cursor
	this->canvas->ClearCharacter( (this->charx+1) * TILE_WIDTH + this->padding,
								 this->chary * TILE_HEIGHT + this->padding, CURSOR_COLOR );
	
	if ( this->chary > MAX_LINE ) {
		this->clear( );
		// Refresh the screen.
		//this->canvas->Draw();
		return;
	}
	

	// Draw it.
	this->canvas->DrawCharacter((this->charx * TILE_WIDTH + this->padding),
								(this->chary * TILE_HEIGHT + this->padding),
								 c, color );

	// Increment.
	this->charx++;
	
	

	// Refresh the screen.
	//this->canvas->Draw();

	
}

// Standard printf function.
void Console::kprint( const char* string ) {
	this->kprint( (char*) string );
}

// Standard printf function.
void Console::kprintf( const char* fmt, va_list argp) {
	const char *p;
	//va_list argp;
	int i;
	char *s;
//	char fmtbuf[256];
	
	//va_start(argp, fmt);
	
	for(p = fmt; *p != '\0'; p++)
	{
		if(*p != '%')
		{
			this->printChar(*p, 0xFFFFFF);
			continue;
		}
		
		switch(*++p)
		{
			case 'c':
				i = va_arg(argp, int);
				this->printChar(i, 0xFFFFFF);
				break;
				
			case 'd':
				i = va_arg(argp, int);
				this->kbase( (mint)i, 10 );
				break;
				
			case 's':
				s = va_arg(argp, char *);
				this->kprint(s);
				break;
				
			case 'x':
				i = va_arg(argp, int);
				this->kbase( (mint)i, 16 );
				break;
				
			case '%':
				this->printChar(*p, 0xFFFFFF);
				break;
		}
	}
	
	//va_end(argp);
}

// Clearscreen function.
void Console::clear( void ) {
	this->charx = BACKGROUND_OFFSET_X;
	this->chary = BACKGROUND_OFFSET_Y;
	this->canvas->Clear( BORDER_COLOR );
	
	
	//this->canvas->Draw();
}

void Console::kprint( char* string ) {
	// Iterate over the string.
	while ( *string != '\0' ) {
		// Print the character.
		this->printChar( *(string++), 0xFFFFFF );
	}
}

void Console::kbase( long prim, long base, long size ) {

	// Validate the base.
	if ( base < 1 ) {
		this->kprint("error: Unsupported base in mathematical operation.\n");
		return;
	}

	// Create the kfloat
	Math::kfloat digit(prim);

	char symbols[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P' };
	
	
	if ( digit.getIsLarge() ) {		
		this->kbase(digit.getBig1(), base, Math::getDigitCount<mint>( digit.getBig1(), base ) - 1);
		this->kbase(digit.getBig2(), base, Math::getDigitCount<mint>( digit.getBig2(), base ) - 1);
		return;
	} else if ( size == 0 ) {
	
		size = Math::getDigitCount<mint>(prim, base) - 1;
		
	}
	
	while ( size >= 0 ) {
		
		mint denominator = Math::pow( base, size - 1, false );
		
		Math::kfloat top, bottom, val;
		
		top = ( prim <= base ) ? prim * base : prim;
		
		bottom = denominator;
		
		val = top / bottom;
		
		// Print the character.
		// Check if the character overflows.
		if ( val.getMajor() > base ) {
			this->printChar( 'e', 0x00FF00 );
		} else {	
			this->printChar( symbols[(char)val.getMajor()], 0xD52E53 );
		}
				
	    prim = prim - (val.getMajor() * denominator );
	    
	    // Decrement
	    size--;
		
	}	
}

void Console::kbase( long value, long base ) {
	this->kbase( value, base, 0 );
}


void Console::kout( const char* string ) {
	
	const char* prefix = "[DONE]\t";
	while ( *(prefix) != '\0' ) {
		this->printChar( *(prefix++), 0xFF0000 );
	}
	this->kprint( string );
	this->printChar('\n', 0xFF0000 );
	
}


// Constructor.
Console::Console( gpu2dCanvas* surface ) {

	// Setup the variables.
	this->charx = BACKGROUND_OFFSET_X;
	this->chary = BACKGROUND_OFFSET_Y;
	this->padding = 10;
	this->canvas = surface;
	
	// Clear the screen.
	this->clear();
}
