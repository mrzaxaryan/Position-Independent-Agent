/*
 * RISC-V 64 linker script — merge .rodata into .text
 *
 * The RISC-V 64 backend emits .rodata.cst8 constant-pool entries for
 * 64-bit values that cannot be cheaply materialised as immediate
 * instruction sequences (the cost at -Oz exceeds a PC-relative load
 * from a constant pool).  These entries break position-independence
 * because ExtractBinary only extracts the .text section.
 *
 * Merging .rodata into .text keeps the constant-pool data inside the
 * extracted PIC binary.  PC-relative loads (auipc+ld) resolve correctly
 * because the linker applies relocations after layout.
 *
 * .text input sections are listed FIRST so that --symbol-ordering-file
 * can place entry_point at offset 0 of the output section.
 */

SECTIONS
{
    .text : {
        *(.text .text.*)
        *(.rodata .rodata.*)
    }
}
