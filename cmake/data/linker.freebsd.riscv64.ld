/*
 * FreeBSD RISC-V 64 linker script — merge .rodata into .text with a non-zero base
 *
 * The RISC-V 64 backend emits .rodata.cst8 constant-pool entries for
 * 64-bit values that cannot be cheaply materialised as immediate
 * instruction sequences.  Merging .rodata into .text keeps the
 * constant-pool data inside the extracted PIC binary.
 *
 * The explicit origin address (0x200000) is required because FreeBSD
 * does not allow mapping at virtual address 0 (security.bsd.map_at_zero
 * defaults to 0).  Without this, the custom linker script overrides
 * LLD's default base address and the binary loads at VA 0, causing a
 * segfault on every FreeBSD RISC-V kernel.
 *
 * .text input sections are listed FIRST so that --symbol-ordering-file
 * can place entry_point at offset 0 of the output section.
 */

SECTIONS
{
    . = 0x200000;
    .text : {
        *(.text .text.*)
        *(.rodata .rodata.*)
    }
}
