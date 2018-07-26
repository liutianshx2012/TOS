#include <mmu.h>
#include <memlayout.h>

#define REALLOC(x) (x - KERNBASE)

.text
.globl kern_entry
kern_entry:
    # reload temperate gdt (second time) to remap all physical memory
    # virtual_addr 0~4G = linear_addr & physical_addr - KERNBASE ~ 4G - KERNBASE
    lgdt REALLOC(__GDT_DESC)
    movl $KERNEL_DS, %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss

    ljmp $KERNEL_CS, $relocated

relocated:

    # set ebp, esp
    movl $0x0, %ebp
    # the kernel stack region is from bootstack -- bootstacktop,
    # the kernel stack size is KSTACKSIZE (8KB)defined in memlayout.h
    movl $bootstacktop, %esp
    # now kernel stack is ready , call the first C function
    call kern_init

# should never get here
spin:
    jmp spin

.data
.align PGSIZE
    .globl bootstack
bootstack:
    .space KSTACKSIZE
    .globl bootstacktop
bootstacktop:

.align 4
__GDT:
    SEG_NULLASM
    SEG_ASM(STA_X | STA_R, - KERNBASE, 0xffffFFFF)      # code segment
    SEG_ASM(STA_W, - KERNBASE, 0xFFFFffff)              # data segment
__GDT_DESC:
    .word 0x17                                          # sizeof(__GDT) - 1
    .long REALLOC(__GDT)

