/*
    Student Name: Ziteng Yang
    StudentID: 517021910683
*/

/*
    Note: This system call can 
*/

#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/sched.h>
#include<linux/unistd.h>
//#include <stdlib.h>

#include <asm/pgtable.h>    //where we can find PGDIR_SHIFT, PMD_SHIFT
#include <asm/errno.h>      //error infomation
#include <linux/uaccess.h>    //where copy_to_user() is defined

#include <linux/mm.h>       //where we can find mm_walk
#include <linux/mm_types.h>
//#include <pagewalk.c>
#include <linux/slab.h>

#include <asm/page.h>

//#include <./pagewalk.c>

MODULE_LICENSE("Dual BSD/GPL");
#define __NR_GET 380
#define __NR_EXPOSE 381
#define MAX_PID 32767

struct pagetable_layout_info{
    uint32_t pgdir_shift;
    uint32_t pmd_shift;
    uint32_t page_shift;
};

static int (*oldcall_1)(void);
static int (*oldcall_2)(void);

static int get_pagetable_layout(struct pagetable_layout_info __user *pgtbl_info,
                                int size)
{
    printk("________________________________________\n");
    printk("start get_pagetable_layout() system call...\n");

    if(sizeof(struct pagetable_layout_info)!=size)
    {
        printk("struct size unmathed!\n");
        return -1;
    }

    struct pagetable_layout_info tmp;
    tmp.pgdir_shift=PGDIR_SHIFT;
    tmp.pmd_shift=PMD_SHIFT;
    tmp.page_shift=PAGE_SHIFT;

    if(copy_to_user(pgtbl_info,&tmp,sizeof(struct pagetable_layout_info)))
    {
        printk("Error copying page table layout to user!\n");
        return -2;
    }
    printk("end get_pagetable_layout() system call...\n");
    printk("________________________________________\n");
    return 0;
}


//info struct to construct fake pgd in kernel memory.
struct myPrivate
{
    unsigned long *fake_pgd_base;
    unsigned long pte_base;
};

int my_pgd_entry(pmd_t *pgd,unsigned long addr,unsigned long next,struct mm_walk *walk)
{
    //printk(KERN_INFO"pgd:%08x",pgd);
    unsigned long pgd_index=pgd_index(addr);
    unsigned long pgdpg=pmd_page(*pgd);

    //get the physical frame number.
    unsigned long pfn=page_to_pfn((struct page *)pgdpg);
    if(pgd_none(*pgd)||pgd_bad(*pgd)||!pfn_valid(pfn)) 
    {
        printk("failed to find pfn!\n");
        return 0;
    }
    struct myPrivate *base=walk->private;
    
    if(pfn)
        printk(KERN_INFO"pfn:%08X\n",pfn);

    struct vm_area_struct *vma=current->mm->mmap;
    //struct vm_area_struct *vm;

    //check error
    if(!vma)
    {
        printk("find_vma error!\n");
        return 0;
    }
    if(!pgd)
    {
        printk("no pmd!\n");
        return 0;
    }


    down_write(&current->mm->mmap_sem);
        int err=remap_pfn_range(vma,base->pte_base,pfn,PAGE_SIZE,vma->vm_page_prot);
    up_write(&current->mm->mmap_sem);

    //remap: can find pte_base according to pgd_base and pmd_index
    //fake_pgd_base + (pgd_index * sizeof(each_entry))
    //you will either get a null for non-exist pmd or the address of a fake pmd
    //pgd_index and pmd_index are the same here since only 2-level in this 32-bit OS
    base->fake_pgd_base[pgd_index] = base->pte_base;  
    //you can get the remapped address of a Page Table 
    //by reading the content at the address 
    //fake_pmd_base + (pmd_index * sizeof(each_entry))
    base->pte_base += PAGE_SIZE;
    return 0;
}




int expose_page_table(pid_t pid,
                      unsigned long fake_pgd,
                      unsigned long fake_pmds,
                      unsigned long page_table_addr,
                      unsigned long begin_vaddr,
                      unsigned long end_vaddr)
{
    printk("________________________________________\n");
    printk("start expose_page_table() system call...\n");
    if(begin_vaddr>end_vaddr)
    {
        printk("address boundary error!\n");
        return 1;
    }

    struct pid* current_pid=find_get_pid(pid);
    if(!current_pid)
    {
        printk("failed to find pid!\n");
        return 2;
    }
    struct task_struct *current_task=get_pid_task(current_pid,PIDTYPE_PID);
    if(!current_task)
    {
        printk("error finding task_struct!\n");
        return 2;
    }
    printk(KERN_INFO" pid: %d\tname: %s",current_task->pid,current_task->comm);

    printk("Virtual memory area:\n");
    if(!(current_task->mm&&current_task->mm->mmap))
    {
        printk(KERN_INFO"target process has no vm area!\n");
        return 5;
    }
    printk("Virtual memory area:\n");
    struct vm_area_struct* p;   //line 210 in mm_type.h
    down_write(&current_task->mm->mmap_sem);
        for(p = current_task->mm->mmap;p!=NULL;p=p->vm_next)
        {
            printk("0x%08lx - 0x%08lx\n", p->vm_start, p->vm_end);
        }
    up_write(&current_task->mm->mmap_sem);


    struct mm_walk walk;
    struct myPrivate my_private;
    //printk("GFP_KERNEL: %d\n",GFP_KERNEL);  //GFP_KERNEL: default value

    my_private.fake_pgd_base=kcalloc(PAGE_SIZE,sizeof(unsigned long),GFP_KERNEL);
    if(!my_private.fake_pgd_base)
    {
        printk("kcalloc error!\n");
        return 4;
    }
    my_private.pte_base=page_table_addr;

    walk.pgd_entry=&my_pgd_entry;//my_pgd_entry;
    walk.pud_entry=NULL;//my_pud_entry;
    walk.pmd_entry=NULL;//my_pmd_entry;    //my pmd entry function;
    walk.pte_entry=NULL;//my_pte_entry;
    walk.pte_hole=NULL;
    walk.hugetlb_entry=NULL;
    walk.mm=current_task->mm;
    walk.private=&my_private;



    //printk("GFP_KERNEL: %d\n",GFP_KERNEL);  //GFP_KERNEL: default value
    current->mm->mmap->vm_flags|=VM_SPECIAL;
    down_write(&current_task->mm->mmap_sem);
        printk("start traveling page table... \n");
        int err=walk_page_range(begin_vaddr,end_vaddr,&walk);
        printk("end traveling page table...\n"); 
    up_write(&current_task->mm->mmap_sem);
    if(err)
    {
        printk(KERN_INFO"walk_page_range failed!\n");
        return 6;
    }

    if(copy_to_user(fake_pgd,
                    my_private.fake_pgd_base,
                    sizeof(unsigned long)*PAGE_SIZE)) 
    {
        printk("Error copying pgd to user!\n");
        return 3;
    }
    printk("end expose_page_table() system call...\n");
    printk("________________________________________\n");
    return 0;
}

static int addsyscall_init(void)
{
    long *syscall = (long*)0xc000d8c4;

    printk(KERN_INFO"start loading system call: get_pagetabel_layout\n");
    oldcall_1 = (int(*)(void))(syscall[__NR_GET]);
    syscall[__NR_GET]= (unsigned long) get_pagetable_layout;
    printk(KERN_INFO"end loading system call: get_pagetabel_layout\n");

    printk(KERN_INFO"start loading system call: expose_page_table\n");
    oldcall_2 = (int(*)(void))(syscall[__NR_EXPOSE]);
    syscall[__NR_EXPOSE]= (unsigned long) expose_page_table;
    printk(KERN_INFO"end loading system call: expose_page_table\n");

    printk(KERN_INFO "module load!\n");
    return 0;
}

static void addsyscall_exit(void)
{
    long *syscall = (long*)0xc000d8c4;
    
    syscall[__NR_GET] = (unsigned long)oldcall_1;
    printk(KERN_INFO "module get_pagetable_layout exit!\n");
    syscall[__NR_EXPOSE] = (unsigned long)oldcall_2;
    printk(KERN_INFO "module expose page table exit!\n");
    printk(KERN_INFO "module exit!\n");
}

module_init(addsyscall_init);
module_exit(addsyscall_exit);
