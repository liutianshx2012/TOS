/* Simple linker script for the kernel.
See the GNU ld 'info' manual ("info ld") to learn the syntax. */

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(kern_init)

SECTIONS {
    /* Load the kernel at this address: "." means the current address */
    . = 0x100000;

    .text : {
        *(.text .stub .text.* .gnu.linkonce.t.*)
}

PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */

.rodata : {
    *(.rodata .rodata.* .gnu.linkonce.r.*)
}

/* Include debugging information in kernel memory */
.stab : {
    PROVIDE(__STAB_BEGIN__ = .);
    *(.stab);
    PROVIDE(__STAB_END__ = .);
    BYTE(0)		/* Force the linker to allocate space
    for this section */
}

.stabstr : {
    PROVIDE(__STABSTR_BEGIN__ = .);
    *(.stabstr);
    PROVIDE(__STABSTR_END__ = .);
    BYTE(0)		/* Force the linker to allocate space
    for this section */
}

/* Adjust the address for the data segment to the next page */
. = ALIGN(0x1000);

/* The data segment */
.data : {
    *(.data)
}

PROVIDE(edata = .);

.bss : {
    *(.bss)
}
/* 在ld script 中定义某个 symbol,可以在 binary 中被访问*/
PROVIDE(end = .);
/* 特殊的段,括号内所有规定的条件段都将被丢弃,不输出文件中*/
/DISCARD/ : {
    *(.eh_frame .note.GNU-stack)
}
}