.section ".text.boot"

.global init
.extern kmain

init:
;@ Move the stack pointer.
	mov sp, #0x8000

/* Clear out bss */
	ldr r4, =_bss_start
	ldr r9, =_bss_end
	mov r5, #0
	mov r6, #0
	mov r7, #0
	mov r8, #0

	b .test

.loop:
	/* this does 4x4 = 16 byte stores at once */
	stmia r4!, {r5-r8}	/* the '!' increments r4 but only after ('ia') the store */
.test:
	cmp r4, r9
	blo .loop

.main:
	/* branch and link to kernel_main */
	ldr r3, =kmain
	blx r3		/* blx may switch to Thumb mode, depending on the target address */

halt:
	wfe		/* equivalent of x86 HLT instruction */
	b halt

.globl dummy
dummy:
    bx lr

.globl memory_barrier
memory_barrier:
	mov	r0, #0
	mcr	p15, #0, r0, c7, c10, #5
	mov	pc, lr

.globl read_sctlr
read_sctlr:
	mrc	p15, #0, r0, c1, c0, #0
	mov	pc, lr

.globl quick_memcpy
quick_memcpy:
	push 	{r4-r9}
	mov	r4, r0
	mov	r5, r1

.loopb:
	ldmia	r5!, {r6-r9}
	stmia	r4!, {r6-r9}
	subs	r2, #16
	bhi	.loopb

	pop	{r4-r9}
	mov	pc, lr

.globl end
	end:
	b halt