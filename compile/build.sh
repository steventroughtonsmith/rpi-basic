#!/bin/bash

PATH=$PATH:"/opt/local/bin/"

# Options
compile_mode=1
compile_clean=1
include_output=0

# Compiler Toolchain
compiler=arm-none-eabi
ldlags="-nostartfiles -O3"
gppflags="-nostdlib -nostartfiles -ffreestanding -O3"
gccflags="-pedantic -pedantic-errors -nostdlib -nostartfiles -ffreestanding -Wall -std=gnu99 -I../code/filesystem -DBUILDING_RPIBOOT -DENABLE_SD -DENABLE_FAT -DDEBUG2 -DENABLE_MBR -DENABLE_BLOCK_CACHE"
asflags="-mcpu=arm1176jzf-s"
#-O3 -funroll-loops"
#
# Folders
boot=../boot
code=../code
debug=../debug
libs=../lib
basic=../basic
build=../build

# Obligatory header
clear
echo "INIT COMPILER SCRIPT"
echo ""
echo "Time:      $(date +%Y-%m-%d)  $(date +%r)"
echo "Project:   Rasberry Pi C++ Kernel"
echo "Author:    SharpCoder"
echo ""
echo "Compiling the kernel..."

for var in "$@"
do
	case "$var" in
	--debug-only)
		compile_mode=1
		include_output=1
	;;
	--include-output)
		include_output=1
	;;
	--clean)
		compile_clean=1
	;;
	esac
done

if [-d $build ]
then
	echo "Build Directory Exists"
else
	mkdir -p $build
fi

# Assemble the metadata file.

echo "Generating Meta Data..."
if [ ! $compile_mode -eq 1 ]
then
	./make-meta.sh
fi

# Remove the old kernel stuff.
if [ -f "$boot/kernel.img"]
then
	rm -r "$boot/kernel.img"
fi

# Clean
if [ $compile_clean -eq 1 ]
then
	rm -f $build/*.o
fi

ASM_FILES="irq bootstrap keyboard cpp_enables"
CPP_FILES="console_glue kmain"
BASIC_CPP_FILES="ubasic tokenizer"
C_FILES="heap"

# Compile!

for FILE in $ASM_FILES
do
$compiler-as $asflags "$code/$FILE.S" -o $build/$FILE.o
done

for FILE in $CPP_FILES
do
$compiler-g++ $gppflags -c "$code/$FILE.cpp" -o $build/$FILE.o
done

for FILE in $BASIC_CPP_FILES
do
$compiler-g++ $gppflags -c "$basic/$FILE.cpp" -o $build/$FILE.o
done


for FILE in $C_FILES
do
$compiler-gcc $gccflags -c "$code/$FILE.c" -o $build/$FILE.o
done

echo "Linking..."

# Link!
$compiler-g++ $ldlags $build/*.o  -L$libs -lcsud -lfs -o $build/kernel.elf -Wl,-T,"$code/linker.ld" -lgcc

# Generate the IMG file.
$compiler-objcopy $build/kernel.elf -O binary "$boot/kernel.img"

# Clean Up
if [ $include_output -eq 1 ];
then
	echo "Generating debug files..."
	mkdir -p $debug
	$compiler-objdump -D $build/bootstrap.o > "$debug/bootstrap.o.asm"
	$compiler-objcopy $build/bootstrap.o -O srec "$debug/bootstrap.srec"
	hexdump -C $build/kernel.elf > "$debug/kernel.hex"
	$compiler-objdump -D $build/irq.o > "$debug/irq.o.asm"
	$compiler-objdump -D $build/kmain.o > "$debug/kmain.o.asm"
	$compiler-objdump -D $build/kernel.elf > "$debug/kern.elf.asm"
fi

# Footer

if [ -f "$boot/kernel.img" ]
then
	echo "Successful!"
else
	exit -1
fi

