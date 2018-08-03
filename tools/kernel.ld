/* Simple linker script for the kernel.
See the GNU ld 'info' manual ("info ld") to learn the syntax. */

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386) /*CPU type*/
ENTRY(kern_entry) /*入口代码地址*/

SECTIONS {
    /* Load the kernel at this address: "." means the current address */
    /*. = 0x100000;*/
    . = 0xC0100000; /* 定义了kernel 加载地址 , 这是一个虚地址,可以任务 link addr = 
                    virutal addr, kernel 建立内核页表时,设定了 physical addr 和 virtual addr 的映射关系 phy addr + 0xC000 0000 = virtual add.
                    即 virutal add 与 physical 之间有一个偏移, bootloader 把 kernel 
                    elf load 到内存时,采用的是 load addr, 这是由于 kernel 还没有运行,
                    即还没有启动页表映射, 这时采用的寻找方式是--->段寻址方式,用的是 
                    bootloader 在初始化阶段设置的段映射关系,其映射关系(bootasm.S 段描述符表的内容)是: linear addr = physical addr = virtual addr.

                    在 bootmain.c 中 
                    readseg(ph->p_va & 0xFFffff, ph->p_memsz, ph->p_offset);
                    这里的 ph->p_va = 0xC0xxXXXX ,就是 ld 根据 kernel.ld 设置的链接地址,
                    并且link addr = virtual addr. 
                    考虑到 ph->p_va & 0xFFffff === 0x0xxXXXX, 所以 bootloader 加载 
                    kernel elf 的 load addr == 0x0xxXXXX ,这实际上是 kernel 所在的 
                    physical addr. 
                    
                    简而言之: kernel 的 link addr 在 kernel.ld 中设置好了, 是一个 
                    vitural addr； kernel 的 load addr 在 bootloader 中的 bootmain 函
                    数中指定,是一个 physical addr.

                    总结: kernel link addr == kernel virtual addr
                  bootloader load kernel  link addr === kernel physical addr

                    */

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

    /* The data segment 存储程序执行代码的一块内存区域. 大小在程序运行前,编译期已经确定,在
                         内存中只读,也可能包含一些只读的常数常量,eg: string 常量.
       .data 是一个地址, data segment 的 start addr.  
    */
    .data : {
        *(.data)
    }

    PROVIDE(edata = .);
    /*
      The bss segment 存储程序中未初始化的全局变量的区域.  静态内存分配
    */
    .bss : {
        *(.bss)
    }
    /* 在ld script 中定义某个 symbol,可以在 binary 中被访问
       bootloader 加载kernel 的结束地址(end 相当于全局指针变量)
    */
    PROVIDE(end = .);
    /* 特殊的段,括号内所有规定的条件段都将被丢弃,不输出文件中*/
    /DISCARD/ : {
        *(.eh_frame .note.GNU-stack)
    }
}
