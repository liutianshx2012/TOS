/*************************************************************************
	> File Name: mmu.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 12时51分24秒
 ************************************************************************/
#ifndef _KERN_MM_MMU_H
#define _KERN_MM_MMU_H

#include <defs.h>

/* Eflags register 标志寄存器 EFLAGS 中的系统标志 */
#define FL_CF            0x00000001    // Carry Flag 进位标志 0
#define FL_PF            0x00000004    // Parity Flag 恢复标志 2
#define FL_AF            0x00000010    // Auxiliary carry Flag 辅助进位 4
#define FL_ZF            0x00000040    // Zero Flag 零标志 6
#define FL_SF            0x00000080    // Sign Flag 负号标志 7
#define FL_TF            0x00000100    // Trap Flag 跟踪标志 8
#define FL_IF            0x00000200    // Interrupt Flag 中断允许标志 9
#define FL_DF            0x00000400    // Direction Flag 方向标志 10
#define FL_OF            0x00000800    // Overflow Flag  溢出标志 11
#define FL_IOPL_MASK     0x00003000    // I/O Privilege Level bitmask I/O特权级 12-13
#define FL_IOPL_0        0x00000000    // IOPL == 0 
#define FL_IOPL_1        0x00001000    // IOPL == 1
#define FL_IOPL_2        0x00002000    // IOPL == 2
#define FL_IOPL_3        0x00003000    // IOPL == 3
#define FL_NT            0x00004000    // Nested Task 嵌套任务标志 14
#define FL_RF            0x00010000    // Resume Flag 恢复标志 16
#define FL_VM            0x00020000    // Virtual 8086 mode 虚拟 8086 模式 17
#define FL_AC            0x00040000    // Alignment Check 
#define FL_VIF           0x00080000    // Virtual Interrupt Flag
#define FL_VIP           0x00100000    // Virtual Interrupt Pending
#define FL_ID            0x00200000    // ID flag

/* Application segment type bits */
/*
                        E ==> 向下扩展
                        A ==> 可访问
                        W ==> 可写(读)
                        C (Conforming) ==> 一致性段
                        X ==> 可执行
--------------------------------------------------------
    TYPE   |   E    |   W   |   A   |
--------------------------------------------------------
    bit 11 | bit 10 | bit 9 | bit 8  | type | descriptor
--------------------------------------------------------
        0   |   0    |    0  |   0   | data | only R   0
--------------------------------------------------------
        0   |   0    |    0  |   1   | data | only R|A 1
--------------------------------------------------------
        0   |   0    |    1  |   0   | data | R | W    2
--------------------------------------------------------
        0   |   0    |    1  |   1   | data | R|W | A  3 
--------------------------------------------------------
        0   |   1    |    0  |   0   | data | E | R    4  
--------------------------------------------------------
        0   |   1    |    0  |   1   | data | E|R|A    5 
-------------------------------------------------------- 
        0   |   1    |    1  |   0   | data | E|R|W    6
--------------------------------------------------------
        0   |   1    |    1  |   1   | data | E|R|W|A  7
--------------------------------------------------------
            |   C    |    R  |   A   |      | 
--------------------------------------------------------
        1   |   0    |    0  |   0   | code | only X   8
--------------------------------------------------------
        1   |   0    |    0  |   1   | code | X|A      9
--------------------------------------------------------
        1   |   0    |    1  |   0   | code | R|X      a
-------------------------------------------------------- 
        1   |   0    |    1  |   1   | code | R|X|A    b
--------------------------------------------------------
        1   |   1    |    0  |   0   | code | C|only X c
--------------------------------------------------------
        1   |   1    |    0  |   1   | code | C|X|A    d
--------------------------------------------------------
        1   |   1    |    1  |   0   | code | C|X|R    e
--------------------------------------------------------
        1   |   1    |    1  |   1   | code | C|X|R|A  f
--------------------------------------------------------
*/
#define STA_X            0x8            // Executable segment 
#define STA_E            0x4            // Expand down (non-executable segments)
#define STA_C            0x4            // Conforming code segment (executable only)
#define STA_W            0x2            // Writeable (non-executable segments)
#define STA_R            0x2            // Readable (executable segments)
#define STA_A            0x1            // Accessed

/* System segment type bits */
/*
     bit 11  | bit 10 | bit 9 | bit 8| 
     --------------------------------------------------------
        0   |   0    |   0   |   1  |  16-bit TSS(Available)1
     --------------------------------------------------------
        0   |   0    |   1   |   0  |  LDT                  2
     --------------------------------------------------------
        0   |   0    |   1   |   1  |  16-bit TSS(Busy)     3
     --------------------------------------------------------
        0   |   1    |   0   |   0  |  16-bit Call Gate     4
     --------------------------------------------------------
        0   |   1    |   0   |   1  |  Task Gate            5
     --------------------------------------------------------
        0   |   1    |   1   |   0  |  16-bit Interrupt Gate6
     --------------------------------------------------------
        0   |   1    |   1   |   1  |  16-bit Trap Gate     7
     --------------------------------------------------------
        1   |   0    |   0   |   0  |  Reserved             8
     --------------------------------------------------------
        1   |   0    |   0   |   1  |  32-bit TSS(Available)9
     --------------------------------------------------------
        1   |   0    |   1   |   0  |  Reserved             a
     --------------------------------------------------------
        1   |   0    |   1   |   1  |  32-bit TSS(Busy)     b
     --------------------------------------------------------
        1   |   1    |   0   |   0  |  32-bit Call Gate     c
     --------------------------------------------------------
        1   |   1    |   0   |   1  |  Reserved             d
     --------------------------------------------------------
        1   |   1    |   1   |   0  |  32-bit Interrupt Gatee
     --------------------------------------------------------
        1   |   1    |   1   |   1  |  32-bit Trap Gate     f
     --------------------------------------------------------
 */
#define SST_T16A        0x1            // Available 16-bit TSS
#define SST_LDT         0x2            // Local Descriptor Table
#define SST_T16B        0x3            // Busy 16-bit TSS
#define SST_CG16        0x4            // 16-bit Call Gate
#define SST_TG          0x5            // Task Gate / Coum Transmitions
#define SST_IG16        0x6            // 16-bit Interrupt Gate
#define SST_TG16        0x7            // 16-bit Trap Gate
#define SST_T32A        0x9            // Available 32-bit TSS
#define SST_T32B        0xB            // Busy 32-bit TSS
#define SST_CG32        0xC            // 32-bit Call Gate
#define SST_IG32        0xE            // 32-bit Interrupt Gate
#define SST_TG32        0xF            // 32-bit Trap Gate

/* 
 * Gate descriptors for interrupts and traps sizof(struct gatedesc)=64 
 *  
 * */
struct gatedesc {
    unsigned gd_off_15_0 : 16;        // low 16 bits of offset in segment
    unsigned gd_ss : 16;              // segment selector
    unsigned gd_args : 5;             // # args, 0 for interrupt/trap gates
    unsigned gd_rsv1 : 3;             // reserved(should be zero I guess)
    unsigned gd_type : 4;             // type(SST_{TG,IG32,TG32})
    unsigned gd_s : 1;                // must be 0 (system)
    unsigned gd_dpl : 2;              // descriptor(meaning new) privilege level
    unsigned gd_p : 1;                // Present
    unsigned gd_off_31_16 : 16;       // high bits of offset in segment
};

/* *
 * Set up a normal interrupt/trap gate descriptor
 *   - gate:为相应的idt数组内容,处理函数的入口地址
 *   - istrap: 系统段设置为1,中断门设置为0 ; 1 for a trap (= exception) gate,  
 *             0 for an interrupt gate
 *   - sel: 段选择子; Code segment selector for interrupt/trap handler
 *   - off: 为vectors数组内容 ; Offset in code segment for interrupt/trap
 *          handler
 *   - dpl:设置特权级.这里中断都设置为内核级,即第0级 ; Descriptor Privilege Level - 
 *          the privilege level required for software to invoke this
 *          interrupt/trap gate explicitly using an int instruction.
 * */
/* trap gate or interrupt gate def (中断门|陷阱门) 描述符宏定义 */
#define SETGATE(gate, istrap, sel, off, dpl) {            \
    (gate).gd_off_15_0 = (uint32_t)(off) & 0xffff;        \
    (gate).gd_ss = (sel);                                 \
    (gate).gd_args = 0;                                   \
    (gate).gd_rsv1 = 0;                                   \
    (gate).gd_type = (istrap) ? SST_TG32 : SST_IG32;      \
    (gate).gd_s = 0;                                      \
    (gate).gd_dpl = (dpl);                                \
    (gate).gd_p = 1;                                      \
    (gate).gd_off_31_16 = (uint32_t)(off) >> 16;          \
}

/* Set up a call gate descriptor */
#define SETCALLGATE(gate, ss, off, dpl) {                 \
    (gate).gd_off_15_0 = (uint32_t)(off) & 0xffff;        \
    (gate).gd_ss = (ss);                                  \
    (gate).gd_args = 0;                                   \
    (gate).gd_rsv1 = 0;                                   \
    (gate).gd_type = SST_CG32;                            \
    (gate).gd_s = 0;                                      \
    (gate).gd_dpl = (dpl);                                \
    (gate).gd_p = 1;                                      \
    (gate).gd_off_31_16 = (uint32_t)(off) >> 16;          \
}

/* code & data segment descriptors  64 bit  段描述符  */
struct segdesc {
    unsigned sd_lim_15_0 : 16;         // low bits of segment limit
    unsigned sd_base_15_0 : 16;        // low bits of segment base address
    unsigned sd_base_23_16 : 8;        // middle bits of segment base address
    unsigned sd_type : 4;              // segment type (see SST_ constants)
    unsigned sd_s : 1;                 // 0 = system, 1 = application
    unsigned sd_dpl : 2;               // descriptor Privilege Level
    unsigned sd_p : 1;                 // Segment present
    unsigned sd_lim_19_16 : 4;         // high bits of segment limit
    unsigned sd_avl : 1;               // unused (available for software use)
    unsigned sd_rsv1 : 1;              // reserved ( 64 bit code segment (IA-32e mode only))
    unsigned sd_db : 1;                // 0 = 16-bit segment, 1 = 32-bit segment
    unsigned sd_g : 1;                 // granularity: limit scaled by 4K when set
    unsigned sd_base_31_24 : 8;        // high bits of segment base address
};
/* NULL 段声明的宏定义 */
#define SEG_NULL                                            \
    (struct segdesc){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
/* s=1 => 非系统段(Code & Data segment);db=1 =>32 bit 段; g=0 ==>byte*/
#define SEG(type, base, lim, dpl)                        \
    (struct segdesc) {                                   \
        ((lim) >> 12) & 0xffff,                          \
        (base) & 0xffff,                                 \
        ((base) >> 16) & 0xff,                           \
        type,                                            \
        1,                                               \
        dpl,                                             \
        1,                                               \
        (unsigned)(lim) >> 28,                           \
         0,                                              \
         0,                                              \
         1,                                              \
         1,                                              \
        (unsigned) (base) >> 24                          \
    }
/*s=0 => system segment ;db=1 =>32 bit 段; g=1 ==>4k*/
#define SEG_TSS(type, base, lim, dpl)                     \
    (struct segdesc) {                                    \
        (lim) & 0xffff,                                   \
        (base) & 0xffff,                                  \
        ((base) >> 16) & 0xff,                            \
        type,                                             \
        0,                                                \
        dpl,                                              \
        1,                                                \
        (unsigned) (lim) >> 16,                           \
        0,                                                \
        0,                                                \
        1,                                                \
        0,                                                \
        (unsigned) (base) >> 24                           \
    }

/* task state segment format (as described by the Pentium architecture book) */
/* 	任务状态段定义 */
struct taskstate {
    uint32_t ts_link;         	// old ts selector
    uintptr_t ts_esp0;       	// stack pointers and segment selectors
    uint16_t ts_ss0;        	// after an increase in privilege level
    uint16_t ts_padding1;
    uintptr_t ts_esp1;
    uint16_t ts_ss1;
    uint16_t ts_padding2;
    uintptr_t ts_esp2;
    uint16_t ts_ss2;
    uint16_t ts_padding3;
    uintptr_t ts_cr3;        	// page directory base
    uintptr_t ts_eip;        	// saved state from last task switch
    uint32_t ts_eflags;
    uint32_t ts_eax;        	// more saved state (registers)
    uint32_t ts_ecx;
    uint32_t ts_edx;
    uint32_t ts_ebx;
    uintptr_t ts_esp;
    uintptr_t ts_ebp;
    uint32_t ts_esi;
    uint32_t ts_edi;
    uint16_t ts_es;           	 // even more saved state (segment selectors)
    uint16_t ts_padding4;
    uint16_t ts_cs;
    uint16_t ts_padding5;
    uint16_t ts_ss;
    uint16_t ts_padding6;
    uint16_t ts_ds;
    uint16_t ts_padding7;
    uint16_t ts_fs;
    uint16_t ts_padding8;
    uint16_t ts_gs;
    uint16_t ts_padding9;
    uint16_t ts_ldt;
    uint16_t ts_padding10;
    uint16_t ts_t;           	 // trap on task switch
    uint16_t ts_iomb;        	 // i/o map base address
};

#endif