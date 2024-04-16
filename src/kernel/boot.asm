; Some code taked from https://forum.osdev.org/viewtopic.php?f=1&t=28429
;					   https://github.com/stevej/osdev/blob/9b745b5de0ba198d3ba245de62453012f346d5af/kernel/start.s

; Declare constants for the multiboot header.
MBALIGN  equ  1 << 0
MEMINFO  equ  1 << 1
VBEMODE  equ  1 << 2
MBFLAGS  equ  MBALIGN | MEMINFO | VBEMODE
MAGIC    equ  0x1BADB002
CHECKSUM equ -(MAGIC + MBFLAGS)
 
; Some constants for loading higher half kernel
VM_BASE     equ 0xC0000000
PDE_INDEX   equ (VM_BASE >> 22)
PSE_BIT     equ 0x00000010
PG_BIT      equ 0x80000000

; ============================
;	MB STRUCT HEADER
; ============================

	section .multiboot
	align 4

	mb_header_start:
		dd MAGIC
		dd MBFLAGS
		dd CHECKSUM

		dd 0x00000000
		dd 0x00000000
		dd 0x00000000
		dd 0x00000000
		dd 0x00000000

		frame_buffer_start:
			dd 0 ; 0 - VBE \ 1 - VGA text
			dd 640, 480, 32
		frame_buffer_end:

	mb_header_end:
 
; ============================
;	MB STRUCT HEADER
; ============================

section .bss
align 16
stack_bottom:
	resb 16384 ; 16 KiB
stack_top:

section .text
global _start:function (_start.end - _start)
_start:

	mov esp, stack_top
	push esp

	push eax
	push ebx

	cli
 
	xor ebp, ebp

	extern kernel_main
	call kernel_main
 
.hang:	hlt
	jmp .hang
.end: