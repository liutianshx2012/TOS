/*************************************************************************
	> File Name: bootmain.c
	> Author:TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 10时47分56秒
 ************************************************************************/
#include <defs.h>
#include <x86.h>
#include <elf.h>

/* *********************************************************************
 * This a dirt simple boot loader, whose sole job is to boot
 * an ELF kernel image from the first IDE hard disk.
 *
 * DISK LAYOUT
 *  * This program(bootasm.S and bootmain.c) is the bootloader.
 *    It should be stored in the first sector of the disk.
 *
 *  * The 2nd sector onward holds the kernel image.
 *
 *  * The kernel image must be in ELF format.
 *
 * BOOT UP STEPS
 *  * when the CPU boots it loads the BIOS into memory and executes it
 *
 *  * the BIOS intializes devices, sets of the interrupt routines, and
 *    reads the first sector of the boot device(e.g., hard-drive)
 *    into memory and jumps to it.
 *
 *  * Assuming this boot loader is stored in the first sector of the
 *    hard-drive, this code takes over...
 *
 *  * control starts in bootasm.S -- which sets up protected mode,
 *    and a stack so C code then run, then calls bootmain()
 *
 *  * bootmain() in this file takes over, reads in the kernel and jumps to it.
 * */
unsigned int    SECTSIZE  =  512 ; // 2^10 / 2 = 1024/2 = 512
struct elfhdr * ELFHDR    = ((struct elfhdr *)0x10000) ;     // scratch space

/* waitdisk - wait for disk ready */
static void
waitdisk(void)
{
    while ((inb(0x1F7) & 0xC0) != 0x40) 
        ;
    /*检查硬盘是否就绪(检查0x1F7的最高两位,如果是01,则跳出循环;否则等待)*/
}

/* readsect - read a single sector at @secno into @dst */
static void
readsect(void *dst, uint32_t secno)
{
    // wait for disk to be ready 等待硬盘就绪
    waitdisk(); 
    // 写地址0x1f2~0x1f5,0x1f7,发出读取磁盘的命令
    outb(0x1F2, 1); // count = 1 要读取的扇区数目,此处我们就读取一个
    outb(0x1F3, secno & 0xFF); //要读取的扇区编号,由上面可知从1开始,0存放的Bootloader
    outb(0x1F4, (secno >> 8) & 0xFF);//用来存放读写柱面的低 8位字节
    outb(0x1F5, (secno >> 16) & 0xFF); //用来存放读写柱面的高 2位字节(其高6位恒为0)
    outb(0x1F6, ((secno >> 24) & 0xF) | 0xE0);//用来存放要读/写的磁盘号及磁头号
    outb(0x1F7, 0x20);      // cmd 0x20 - read sectors 读取扇区命令
    // 上面四条指令联合制定了扇区号
    // 在这4个字节线联合构成的32位参数中
    //   29-31位强制设为1
    //   28位(=0)表示访问"Disk 0"
    //   0-27位是28位的偏移量
    // wait for disk to be ready
    waitdisk();//读取一个扇区

    // read a sector
    insl(0x1F0, dst, SECTSIZE / 4); //读取到dst位置,幻数4因为这里以DW为单位
}

/* *
 * readseg - read @count bytes at @offset from kernel into virtual address @va,
 * might copy more than asked.
 * */
static void
readseg(uintptr_t va, uint32_t count, uint32_t offset)
{
    uintptr_t end_va = va + count;

    // round down to sector boundary
    va -= offset % SECTSIZE;

    // translate from bytes to sectors; kernel starts at sector 1
    // 加1因为0扇区被引导占用,ELF 文件从 1 扇区开始
    uint32_t secno = (offset / SECTSIZE) + 1;

    // If this is too slow, we could read lots of sectors at a time.
    // We'd write more to memory than asked, but it doesn't matter --
    // we load in increasing order.
    for (; va < end_va; va += SECTSIZE, secno ++) {
        readsect((void *)va, secno);
    }
}

/* bootmain - the entry of bootloader */
void
bootmain(void)
{
    /* read the 1st page off disk 首先读取ELF的头部(2^(9+3))= 4kb,加载到 va=0x10000 处 */
    readseg((uintptr_t)ELFHDR, SECTSIZE * 8, 0);

    /* is this a valid ELF? 首先判断是不是ELF*/
    if (ELFHDR->e_magic != ELF_MAGIC) {
        goto bad;
    }

    struct proghdr *ph, *eph;

    /* load each program segment (ignores ph flags)
     * ELF头部有描述ELF文件应加载到内存什么位置的描述表,这里读取出来将之存入ph
     * p /x ph->p_va = 0x100000
     * p /x ph->p_memsz =  0xd939 (55609)
     * p /x ph->p_offset = 0x1000 (2^12 = 4096)
     **/
    ph = (struct proghdr *)((uintptr_t)ELFHDR + ELFHDR->e_phoff);
    /* ph =>0x10034  (0x34 = 52) 从ELF 头处 offset => 0x34 处加载
     * segment => 2 指针偏移2, 两个 struct proghdr 内存占位 sizeof(struct proghdr) = 32
     * eph => 0x10034 + 2(32) = 0x10034 + 0x20 *2 = 0x10074
     */
    eph = ph + ELFHDR->e_phnum; 
    /* os kernel 会编译成 ELF 文件, 其中 SEGMENT 描述了 怎么在虚拟内存中加载（layout）
     * 分别加载到 virtual address
     * 按照程序头表的描述,将ELF文件中的数据载入内存
     */
    for (; ph < eph; ph ++) {
        readseg(ph->p_va & 0xFFFFFF, ph->p_memsz, ph->p_offset);
    }
    /* 第二个段 ph->p_va = > 0x10e000    ph->p_offset => 0xf000
     * ELF文件 0x1000 位置后面的 0xd1ec 比特被载入内存 0x00100000
     * ELF文件 0xf000 位置后面的 0x1d20 比特被载入内存 0x0010e000
     * call the entry point from the ELF header
     * note: does not return
     * 根据ELF头表中的入口信息,找到内核的入口并开始运行 
     * ph => 0x10074  ELFHDR = > 0x10000    0x74 = 116 (52 + 32 + 32)
     **/
    ((void (*)(void))(ELFHDR->e_entry & 0xFFFFFF))();

bad:
    outw(0x8A00, 0x8A00);
    outw(0x8A00, 0x8E00);

    while (1);
}

