:/ Flush the TSS segment with the 0x28 command /:
@[nosection] glob function TSS_flush() -> i0 {
    asm(0x28) { "ltr %0" }
}