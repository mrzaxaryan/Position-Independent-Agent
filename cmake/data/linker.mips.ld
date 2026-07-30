/*
 * MIPS o32 linker script — merge all sections into .text
 *
 * Mirrors linker.mips64.ld. The MIPS backend emits .rodata.cst constant-pool
 * entries and LLD synthesises a small .got (mandatory MIPS GOT header) even
 * with -mno-abicalls -fno-pic. These break position-independence because
 * ExtractBinary only extracts .text. Merging rodata/data/bss/got into .text
 * keeps the binary self-contained and passes the PIC verification check.
 *
 * The PIC loader maps everything RWX and the project forbids mutable globals,
 * so merging .data/.bss into .text is harmless.
 *
 * .text input sections are listed FIRST so --symbol-ordering-file can place
 * entry_point at offset 0 of the output section.
 */

SECTIONS
{
    /* 32-bit load base. Without this the non-PIE executable (required by
       -mno-abicalls) gets VirtAddr 0, which qemu-mipsel cannot map. */
    . = 0x00400000;

    .text : {
        *(.text .text.*)
        *(.rodata .rodata.*)
        *(.data .data.*)
        *(.bss .bss.*)
        *(.got)
    }
}
