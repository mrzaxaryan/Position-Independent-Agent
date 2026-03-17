/*
 * MIPS64 linker script — merge all sections into .text
 *
 * The MIPS64 backend emits .rodata.cst8 constant-pool entries for
 * 64-bit values that cannot be cheaply materialised as immediate
 * instruction sequences (the cost at -Oz exceeds a PC-relative load
 * from a constant pool).  These entries break position-independence
 * because ExtractBinary only extracts the .text section.
 *
 * LLD also synthesises a small .got (16-byte mandatory MIPS GOT header)
 * even with -mno-abicalls -fno-pic.  No code references it (there is
 * no $gp setup), but its presence in its own section fails the PIC
 * verification check.  Merging it into .text silences the check and
 * keeps the binary self-contained.
 *
 * At -O0 the MIPS backend may emit .data and .bss sections for
 * compiler-generated structures (e.g. function-pointer tables after
 * pic-transform, or zero-initialized common symbols).  These are
 * harmless when merged into .text: the PIC loader maps everything
 * with RWX permissions, and the project forbids mutable globals.
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
    /* Standard MIPS64 n64 ABI base address.  Without this the non-PIE
       executable (required by -mno-abicalls) gets VirtAddr 0, which
       qemu-mips64el-static cannot map. */
    . = 0x120000000;

    .text : {
        *(.text .text.*)
        *(.rodata .rodata.*)
        *(.data .data.*)
        *(.bss .bss.*)
        *(.got)
    }
}
