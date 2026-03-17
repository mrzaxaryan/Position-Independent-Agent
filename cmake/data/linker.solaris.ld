/*
 * Solaris linker script — explicit PHDRS, no PT_PHDR
 *
 * LLD always emits PT_PHDR for the default layout.  The Solaris/illumos
 * kernel rejects static executables that contain PT_PHDR without PT_INTERP:
 *
 *   if (uphdr != NULL && intphdr == NULL) goto bad;   // ENOEXEC
 *
 * By declaring PHDRS explicitly, LLD creates only the segments listed
 * here.  PT_PHDR and PT_GNU_STACK are never generated.
 */

PHDRS
{
    text PT_LOAD;
}

SECTIONS
{
    . = 0x400000;
    .text : { *(.text .text.*) *(.got.plt) } :text
}
