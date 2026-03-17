/*
 * i386 linker script — merge .rodata into .text
 *
 * The i386 backend emits .rodata.cst4 constant-pool entries for
 * floating-point values that the LTO code generator materialises
 * at link time (after pic-transform has already run).  These entries
 * break position-independence because ExtractBinary only extracts
 * the .text section.
 *
 * Merging .rodata into .text keeps the constant-pool data inside the
 * extracted PIC binary.  PC-relative loads resolve correctly because
 * the linker applies relocations after layout.
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
