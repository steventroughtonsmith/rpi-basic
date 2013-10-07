//
//  rpi-basic
//
//  Copyright 2013 Steven Troughton-Smith. All rights reserved.
//	Based on code by SharpCoder
//

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "irq.cpp"
#include "raspberrylib.cpp"
#include "gpu2d.cpp"
#include "console.cpp"
#include "sound.cpp"

// Include the meta data generate at compile time
// #include "meta.h"
#include "mem.h"
#include "math.h"
#include "meta.h"

#include "keyboard.h"

#include "../basic/ubasic.h"

#include "uart.cpp"

/* Dir */
struct dirent {
	struct dirent *next;
	char *name;
	uint32 byte_size;
	char is_dir;
	void *opaque;
	struct fs *fs;
};

struct dirent;
struct dir_info {
	struct dirent *first;
	struct dirent *next;
};

#ifdef DIR
#undef DIR
#endif
#define DIR struct dir_info

extern "C" DIR *opendir(const char *name);
extern "C" struct dirent *readdir(DIR *dirp);
extern "C" int closedir(DIR *dirp);


char *ready_prompt = "READY.\n";

using namespace RaspberryLib;

// Define any functions.
void print_header();
void runloop();
void ubasic_loop();
void command_list();
void command_cls();
void command_reset();
void command_help();
void command_print();
void command_catalog();

extern "C" void reset();
extern "C" void enable_irq();

Console *shared_console;

#define PROGRAM_SPACE 128*1024
#define LINE_SIZE 1024

char interpreted_program[PROGRAM_SPACE];
char current_line[LINE_SIZE];

int interpreted_program_length = 0;
int current_line_length = 0;

void reset_program()
{
	memset(interpreted_program, 0, PROGRAM_SPACE*sizeof(char));
	
	interpreted_program_length = 0;
}

void reset_line()
{
	memset(interpreted_program, 0, LINE_SIZE*sizeof(char));
	current_line_length = 0;
}

void enable_uart()
{
	uart_init();
	uart_printf("UART Enabled\n");
}

unsigned time_ticks;

extern "C" void c_irq_handler();

void c_irq_handler()
{
	++time_ticks;
}

extern "C" void list_root()
{
#if 0
	char *path = "emmc0_0/";
	
	DIR *dir;
    struct dirent *dent;
    char buffer[50];
    strcpy(buffer, path);
    dir = opendir(buffer);   //this part
    if(dir!=NULL)
    {
        while((dent=readdir(dir))!=NULL)
            printf(dent->name);
    }
    closedir(dir);
#endif
}

extern "C" void libfs_init();
extern "C" void vfs_list_devices();

#define UNUSED(x) (void)(x)

uint32 _atags;
uint32 _arm_m_type;
static char *atag_cmd_line;


// Define the entry point for our application.
// Note: It must be marked as "extern" in order for the linker
// to see it properly.
extern "C" void kmain( uint32 boot_dev, uint32 arm_m_type, uint32 atags )
{
	atag_cmd_line = (char *)0;
	_atags = atags;
	_arm_m_type = arm_m_type;
	UNUSED(boot_dev);
	
	// Enable UART
	enable_uart();
	
	// Initialize memory management first.
	init_page_table();
	
	// Create a canvas.
	gpu2dCanvas canvas(false);
	
	// Create a console.
	Console console(&canvas);
	shared_console = &console;
	
#if SD_CARD_SUPPORT
	// Register the various file systems
	libfs_init();
	
	// List devices
	printf("Devices: ( ");
	vfs_list_devices();
	printf(" )\n");
#endif
	
	// Wire up the interrupts.
	irq_console = &console;
	use_irq_console = false;
	
	// Draw to the console.
	print_header();
	
	// IRQ
	irq_enable();
	
	reset_program();
	
	// USB
	UsbInitialise();
	
	printf(ready_prompt);
	
	while (1)
	{
		runloop();
		canvas.Draw();
	}
	
	return;
}

static int ended;

void runloop()
{
	KeyboardUpdate();
	
	char c = KeyboardGetChar();
	
	//kprintf("a");
	
	if (c != 0)
	{
		
		printf("%c", c);
		
		
		if (c == '\b')
		{
			current_line_length--;
		}
		else
			current_line[current_line_length++] = c;
		
		//		char printPrefix[6];
		//
		//		if (current_line_length > 5)
		//		{
		//			for (int i = 0; i< 5; i++)
		//			{
		//				printPrefix[i] = current_line[i];
		//			}
		//
		//			printPrefix[5] = '\0';
		//		}
		
		if (c == '\n')
		{
			if (current_line[0] >= '0' && current_line[0] <= '9')
			{
				for (int i = 0; i < current_line_length; i++)
				{
					interpreted_program[interpreted_program_length+i] = current_line[i];
				}
				
				interpreted_program_length+= current_line_length;
			}
			else
			{
				current_line[current_line_length] = '\0';
				
				if (strcmp(current_line, "run\n") == 0)
				{
					interpreted_program[interpreted_program_length] = '\0';
					
					ubasic_init(interpreted_program);
					
					
					ubasic_loop();
					
					
					reset_program();
				}
				else if (strcmp(current_line, "list\n") == 0)
				{
					command_list();
				}
				else if (strcmp(current_line, "cls\n") == 0)
				{
					command_cls();
				}
				else if (strcmp(current_line, "reset\n") == 0)
				{
					command_reset();
				}
				else if (strcmp(current_line, "help\n") == 0)
				{
					command_help();
				}
				else if (strcmp(current_line, "catalog\n") == 0)
				{
					command_catalog();
				}
				//				else if (strcmp(printPrefix, "print") == 0)
				//				{
				//					command_print();
				//				}
				else
				{
					printf("\n?SYNTAX ERROR\nREADY.\n");
				}
			}
			
			//reset_line();
			current_line_length = 0;
			
		}
		
	}
}

void command_catalog()
{
	// TODO
}

void command_print()
{
	char *oneshot = "";
	
	strcat(oneshot, "10 ");
	
	current_line[current_line_length-1] = '\0';
	strcat(oneshot, current_line);
	
	
	ubasic_init(oneshot);
	
	ubasic_loop();
}

void command_help()
{
	printf("\n?RUN LIST RESET CLS HELP PRINT LET IF THEN ELSE FOR TO NEXT\nGOTO GOSUB RETURN CALL END\nREADY.\n");
}

void command_reset()
{
	reset_program();
	reset_line();
	
	command_cls();
	print_header();
	printf(ready_prompt);
	
}

void command_cls()
{
	shared_console->clear();
}

void command_list()
{
	printf("%s\n", interpreted_program);
}

void ubasic_loop()
{
	bool shouldHalt = false;
	
	do {
		KeyboardUpdate();
		
		char c = KeyboardGetChar();
		if (c == 0xff)
		{
			shouldHalt = true;
			ended = 1;
			break;
		}
		ubasic_run();
		
		
	} while(!ubasic_finished());
	
	if (shouldHalt)
	{
		printf("HALTED.\n");
	}
	else
	{
		printf("\n");
	}
}

void print_header()
{
	meta info = getBuildInfo();
	
	printf("                **** RaspberryPi BASIC V1 ****\n");
	printf("               (c) 2013 Steven Troughton-Smith\n");
	printf("\n\n");
}
