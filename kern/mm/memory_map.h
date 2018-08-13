/* *
 * physical memory map:                                          Permissions
 *                                                              kernel/user
 *  free start addr 空间内存空间的起始地址
 *     4G ------------------> +---------------------------------+ 0xFFFFffff
 *                            |       32bit设备映射空间           |
 *                            |---------------------------------|
 *                            |                                 |
 *                            |                                 |
 *                            |---------------------------------+ 实际物理内存空间 end addr 
 *                            |       空闲内存空间                |
 *                            |---------------------------------+ free start addr-freemem
 *                            |  n * sizeof(struct Page)大小的   |
 *                            |          空间                    |
 *                            |---------------------------------+ kernel end 0x1189c8
 *                            |      KERNEL  BSS                |
 *                            |---------------------------------+ kernel edata 0x117a38
 *                            |      KERNEL  DATA               |
 *                            |---------------------------------+ kernel etext 0x105c23
 *                            |      KERNEL  TEXT               |
 *                            |---------------------------------+ 0x00100000 (1MB)
 *                            |       BIOS ROM                  |
 *                            |---------------------------------+ 0x000F0000 (960KB)
 *                            |      16bit设备映射空间            |
 *                            |---------------------------------+ 0x000C0000 (768KB)
 *                            |       CGA 显存空间               |
 *                            |---------------------------------+ 0x000B8000 (736KB)
 *                            |        空闲内存空间               |
 *                            |                                 |
 *                            |                                 |  
 *                            |                                 |
 *                            |                                 |
 *                            |---------------------------------+ 0x00011000 (68KB)
 *                            |        kernel ELF header data   |  4KB
 *                            |---------------------------------+ 0x00010000 (64KB)
 *                            |        空闲内存空间               |
 *                            |                                 |
 *                            |---------------------------------+ 0x00008000 e820映射结构
 *                            |                                 |
 *                            |---------------------------------+ 基于 bootloader 大小
 *                            |        bootloader TEXT & DATA   |
 *                      31KB  +---------------------------------+ 0x00007C00 (stack top)
 *                            |   bootloader & kernel 共用的堆栈  |
 *                            |---------------------------------+ 基于堆栈的使用情况
 *                            |    low addr 空闲空间             |
 *                      0KB   +---------------------------------+ 0x0
 * 
 */

/* *
 * Virtual memory map:                                          Permissions
 *                                                              kernel/user
 *
 *     4G ------------------> +---------------------------------+ 0xFFFFffff
 *                            |                                 |
 *                            |         Empty Memory (*)        |
 *                            |                                 |
 *                            +---------------------------------+ 0xFB000000
 *                            |   Cur. Page Table (Kern, RW)    | RW/-- PT_SIZE = 4MB
 *     VPT -----------------> +---------------------------------+ 0xFAC00000 
 *                            |        Invalid Memory (*)       | --/-- size 44MB 
 *     KERN_TOP ------------> +---------------------------------+ 0xF8000000
 *                            |                                 |
 *                            |    Remapped Physical Memory     | RW/-- KMEMSIZE 
 *                            |                                 |   896MB
 *     KERN_BASE -----------> +---------------------------------+ 0xC0000000 (3GB)
 *                            |                                 |
 *                            |                                 |
 *                            |                                 |
 *                            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 
 */ 