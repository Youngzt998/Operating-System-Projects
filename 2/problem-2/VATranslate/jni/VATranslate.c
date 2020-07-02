/*
    Code of Problem 3: vm_inspector
    Student Name:Ziteng Yang
    Student ID: 517021910683
*/

#include<stdio.h>
#include<stdlib.h>
#include<sys/mman.h>

/*
*Get the index for fake pgd by calling pgd_index(VA).
*Use the index to find the corresponding entry in fake pgd, and fetch the base address of the fake pmd.
*Get the index for fake pmd by calling pmd_index(VA).
*
*Use the index to find the corresponding entry in fake pmd, 
*and fetch the base address of the remapped page table.
*Get the index for the 3rd level page table, 
*and find the corresponding PTE (page table entry) of the VA.
*Interpret the PTE and get the PA for the VA.
*/
struct pagetable_layout_info{
    uint32_t pgdir_shift;
    uint32_t pmd_shift;
    uint32_t page_shift;
};
/*
*You will have to first get the pgd_index: (pgd_index = pgd_index(ADDR)).
*From the entry fake_pgd_base + (pgd_index * sizeof(each_entry)), 
*you will either get a null for non-exist pmd or the address of a fake pmd.
*Repeat the above process with the pmd_index: (pmd_index = pmd_index(ADDR)), 
*you can get the remapped address of a Page Table 
*by reading the content at the address fake_pmd_base + (pmd_index * sizeof(each_entry)).
Finally, you have the read access of the remapped Page Table.
*/
#define pgd_index(va,info) ((va)>>info.pgdir_shift)
#define pte_index(va,info) (((va)>>info.page_shift)&((1<<(info.pmd_shift-info.page_shift))-1))
#define page_offset(va,info) ((va)& (  (1<<info.page_shift) -1 )  )

struct pagetable_layout_info layout_info;

pid_t pid;
unsigned long *fake_pgd;
unsigned long fake_pmds;
unsigned long *page_table_addr;
unsigned long begin_vaddr;
unsigned long end_vaddr;

int main(int argc,char **argv)
{
    //unsigned long *table_addr;
    //unsigned long *fake_pgd_addr;
    //unsigned long pgd_ind,phy_addr;
    //unsigned long *phy_base;
    printf("------------------------------------------------\n");
    printf("VATranslate\n\n");
    if(argc!=3)
    {
        printf("argument unmathed!\n");
        return -1;
    }

    pid_t pid=atoi(argv[1]);
    begin_vaddr=strtoul(argv[2],NULL,16);

    //first system call: get_pagetable_layout
    if(!syscall(380,&layout_info,sizeof(struct pagetable_layout_info)))
    {
        printf("pgdir_shift: %d\tpmd_shift: %d\tpage_shift: %d\n",
        layout_info.pgdir_shift, layout_info.pmd_shift, layout_info.page_shift);
    }
    else
    {
        printf("System call to get pagetable layout infomation failed!\n ");
    }

    const unsigned long page_size=1<<(layout_info.page_shift);
    const unsigned long pte_size = 1<<(layout_info.pmd_shift-layout_info.page_shift);
    const unsigned long pgd_size = 1<<(32-layout_info.pgdir_shift);

    unsigned long mask=page_size-1;

    //allocate memory for fake pgd
    fake_pgd=malloc(sizeof(unsigned long)*page_size);

    //this allocates virtual memory
    page_table_addr=mmap(NULL,
                         1<<22,
                         PROT_READ | PROT_WRITE, 
                         MAP_SHARED | MAP_ANONYMOUS, 
                         -1, 
                         0);

    if(!page_table_addr||!fake_pgd)
    {
        printf("allocate memory failed!\n");
        return -1;
    }

    int err=syscall(381,pid,fake_pgd,0,page_table_addr,begin_vaddr,begin_vaddr+1);
    //error handler.
    switch (err)
    {
        case 1:
            printf("expose_page_table: address boundary error!\n");
            return 1;
            
        case 2:
            printf("expose_page_table: failed to find pid!\n");
            return 2;
        
        case 3:
            printf("expose_page_table: failed to copy fake pgd!\n");
            return 3;
        case 4:
            printf("expose_page_table: kmalloc error!\n");
            return 4;
        case 5: 
            printf("target process has no vm area!\n");
            return 5;
        case 6:
            printf("walk_page_range failed!\n");
    default:
        break;
    }


    unsigned long pgd_index=pgd_index(begin_vaddr,layout_info);
    unsigned long pte_index=pte_index(begin_vaddr,layout_info);
    unsigned long page_offset = begin_vaddr & 0x0fff;

    unsigned long* p=fake_pgd[pgd_index];
    if(!p)
    {
        printf("invalid pgd entry!\n");
        return -3;
    }

    //get the entry in the table.
    unsigned long frame=p[pte_index];

    frame &= ~(page_size-1);     //mask higher bit
    if(!frame)
    {
        printf("target virtual address is not in memory!\n");
        return -4;
    }

    //add the offset.
    unsigned long physical_address = frame + page_offset;
    printf("Virtual address: 0x%08lx\t", begin_vaddr);
    printf("Physical address: 0x%08lx\n", physical_address);


    //else printf("virtual address:0x%08lx is not in the memory.\n",begin_vaddr);
    //free memory space.
    free(fake_pgd);
    munmap(page_table_addr,1<<22);
    printf("------------------------------------------------\n");
    return 0;
}
