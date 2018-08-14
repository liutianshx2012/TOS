/*************************************************************************
	> File Name: vmm.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Fri 08 Jul 2016 02:53:11 PM CST
 ************************************************************************/
#ifndef _KERN_MM_VMM_H
#define _KERN_MM_VMM_H

#include <defs.h>
#include <list.h>
#include <memlayout.h>
#include <sync.h>

//pre define
struct mm_struct;

// the virtual continuous memory area(vma)
struct vma_struct 
{
	struct mm_struct *vm_mm; // the set of vma using the same PDT
	uintptr_t vm_start;		 // start addr of vma
	uintptr_t vm_end;   	 // end addr of vma
	uint32_t vm_flags;  	 // flags of vma   R|W|X
    list_entry_t list_link;  // linear list link which sorted by start addr of vma
};

#define le2vma(le, member)                  \
    to_struct((le), struct vma_struct, member)

#define VM_READ                 0x00000001
#define VM_WRITE                0x00000002
#define VM_EXEC                 0x00000004
#define VM_STACK                0x00000008

// the control struct for a set of vma using the same PDT
// 包含所有 virtual mem addr 共同属性
struct mm_struct 
{
    list_entry_t mmap_list;        // linear list link which sorted by start addr of vma, 链接所有属于 同一 PDT 的 vma 
    struct vma_struct *mmap_cache; // current accessed vma, used for speed purpose
    pde_t *pgdir;                  // the PDT of these vma
    int map_count;                 // the count of these vma in mmap_list,  
    void *sm_priv;                 // the private data for swap manager
	int mm_count;                  // the number of process which shared the mm
    lock_t mm_lock;                // mutex for using dup_mmap fun to duplicat the mm
};

extern volatile unsigned int pgfault_num;

extern struct mm_struct *check_mm_struct;

struct vma_struct *vma_create(uintptr_t vm_start, uintptr_t vm_end, uint32_t vm_flags);

void insert_vma_struct(struct mm_struct *mm, struct vma_struct *vma);

struct vma_struct *find_vma(struct mm_struct *mm, uintptr_t addr);

struct mm_struct *mm_create(void);

void mm_destroy(struct mm_struct *mm);

int mm_map(struct mm_struct *mm, uintptr_t addr, size_t len, uint32_t vm_flags, struct 
							vma_struct **vma_store);
/* unused */
int mm_unmap(struct mm_struct *mm, uintptr_t addr, size_t len);

int dup_mmap(struct mm_struct *to, struct mm_struct *from);

void exit_mmap(struct mm_struct *mm);

uintptr_t get_unmapped_area(struct mm_struct *mm, size_t len);

int mm_brk(struct mm_struct *mm, uintptr_t addr, size_t len);

int do_pgfault(struct mm_struct *mm, uint32_t error_code, uintptr_t addr);

bool user_mem_check(struct mm_struct *mm, uintptr_t start, size_t len, bool write);

bool copy_from_user(struct mm_struct *mm, void *dst, const void *src, size_t len, bool 	
															writable);

bool copy_to_user(struct mm_struct *mm, void *dst, const void *src, size_t len);

void vmm_init(void);

static inline int
mm_count(struct mm_struct *mm) 
{
    return mm->mm_count;
}

static inline void
set_mm_count(struct mm_struct *mm, int val) 
{
    mm->mm_count = val;
}

static inline int
mm_count_inc(struct mm_struct *mm) 
{
    mm->mm_count += 1;
    return mm->mm_count;
}

static inline int
mm_count_dec(struct mm_struct *mm) 
{
    mm->mm_count -= 1;
    return mm->mm_count;
}

static inline void
lock_mm(struct mm_struct *mm) 
{
    if (mm != NULL) {
        lock(&(mm->mm_lock));
    }
}

static inline void
unlock_mm(struct mm_struct *mm) 
{
    if (mm != NULL) {
        unlock(&(mm->mm_lock));
    }
}

#endif
