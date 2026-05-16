:/ Flush the TSS segment with the 0x28 command /:
glob function TSS_flush() -> i0 {
    asm(0x28) { "ltr %0" }
}