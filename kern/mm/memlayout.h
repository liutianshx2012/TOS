/*************************************************************************
	> File Name: memlayout.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 12时58分47秒
 ************************************************************************/
#ifndef _KERN_MM_MEMLAYOUT_H
#define _KERN_MM_MEMLAYOUT_H

/* This file contains the definitions for memory management in our OS. */

/* global segment number */
#define SEG_K_TEXT    1
#define SEG_K_DATA    2
#define SEG_U_TEXT    3
#define SEG_U_DATA    4
#define SEG_K_TSS     5

/* global descriptor numbers */
#define G_D_K_TEXT    ((SEG_K_TEXT) << 3)        // kernel text   8(0x8)	=> (1000)
#define G_D_K_DATA    ((SEG_K_DATA) << 3)        // kernel data   16(0x10) => (1010)
#define G_D_U_TEXT    ((SEG_U_TEXT) << 3)        // user text     24(0x18) => (0001 1000)
#define G_D_U_DATA    ((SEG_U_DATA) << 3)        // user data  32(0x20) => (0010 0000)
#define G_D_TSS        ((SEG_K_TSS) << 3)// task segment selector 40(0x28) => (0010 1000)

#define DPL_KERNEL  (0)			// 0x0 => (0000)	
#define DPL_USER    (3)			// 0x3 => (0011)
/* segment sector (not segment descriptor)*/
#define KERNEL_CS    ((G_D_K_TEXT) | DPL_KERNEL) // 1000  => 0x8 
#define KERNEL_DS    ((G_D_K_DATA) | DPL_KERNEL) // 1010  => 0x10
#define USER_CS        ((G_D_U_TEXT) | DPL_USER) // 0001 1011 => 0x1b
#define USER_DS        ((G_D_U_DATA) | DPL_USER) // 0010 0011 => 0x23   

#endif
