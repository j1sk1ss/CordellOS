; from https://github.com/szhou42/osdev/blob/master

global i386_switch2user
i386_switch2user:
	
	cli
	mov	ebp, esp
	push	ebp
	
	mov	eax, [esp + 16]	; old_esp save
	;; mov	ecx, [esp]	; current ret eip

	
	mov	ecx, [esp + 12]	; new esp
	mov	edx, [esp + 8]	; new eip
	
	mov	eax, 0x20	; 0x20 -> user ds
	or	eax, 0x3	; 0x3 for privilege level 3 (usermode)
	mov	ds, eax
	mov	es, eax
	mov	fs, eax
	mov	gs, eax

	push	eax		; (5)
	push	ecx		; (4)
	pushf			; (3)

	pop	eax		
	or	eax, 0x200	; hack to re-enable interupt
	push	eax
	
	mov	eax, 0x18	; 0x18 -> user cs
	or	eax, 0x3	; priv lvl 3

	push eax		; (2)
	push edx		; (1)
	
	iret