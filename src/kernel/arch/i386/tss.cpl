:/ Flush the TSS segment with the 0x28 command /:
@[nosection] glob function TSS_flush() -> i0 {
    asm() {
        "mov ax, 0x28",
        "ltr ax"
    }
}
