global fmemcpy
fmemcpy:
    ; Save registers
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    ; Load arguments
    mov rbx, [rsp + 8*1 + 8]   ; ebx (used as rbx) = length
    mov rdi, [rsp + 8*2 + 8]   ; edi = destination
    mov rsi, [rsp + 8*3 + 8]   ; esi = source

    ; Check if length is zero
    cmp rbx, 0
    jbe end_mcpy

    ; Align source address to 16 bytes
    mov rdx, rsi
    neg rdx
    and rdx, 15
    cmp rdx, rbx
    jle unaligned_copy
    mov rdx, rbx

    unaligned_copy:
        mov rcx, rdx
        rep movsb
        sub rbx, rdx
        jz end_mcpy
        mov rcx, rbx
        shr rcx, 7
        jz mc_fl

        loop1:
            movaps xmm0, [rsi]
            movaps xmm1, [rsi + 16]
            movaps xmm2, [rsi + 32]
            movaps xmm3, [rsi + 48]
            movaps xmm4, [rsi + 64]
            movaps xmm5, [rsi + 80]
            movaps xmm6, [rsi + 96]
            movaps xmm7, [rsi + 112]
            movups [rdi], xmm0
            movups [rdi + 16], xmm1
            movups [rdi + 32], xmm2
            movups [rdi + 48], xmm3
            movups [rdi + 64], xmm4
            movups [rdi + 80], xmm5
            movups [rdi + 96], xmm6
            movups [rdi + 112], xmm7
            add rsi, 128
            add rdi, 128
            dec rcx

        jnz loop1

    mc_fl:
        and rbx, 0x7F
        jz end_mcpy
        mov rcx, rbx
        rep movsb

    end_mcpy:
    ; Restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret
