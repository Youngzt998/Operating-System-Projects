/*
    Code of Problem 3: vm_inspector
    Student Name:Ziteng Yang
    Student ID: 517021910683
*/

#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>         //option

//#include <sys/types.h>
#include <sys/mman.h>       //where PAGE_SIZE are found


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
unsigned long begin_vaddr;
unsigned long end_vaddr;

unsigned long *fake_pgd;
unsigned long *page_table_addr;



int main(int argc,char **argv)
{
    printf("------------------------------------------------\n");
    printf("vm_inspector\n\n");


    //check the arguments.
    if(argc!=4)
    {
        printf("Wrong arguments!\n");
        printf("Usage: ./vm_inspector pid begin_vaddr end_vaddr\n");
        return -1;
    }
    pid_t pid=atoi(argv[1]);
    begin_vaddr=strtoul(argv[2],NULL,16);
    end_vaddr=strtoul(argv[3],NULL,16);

    //first system call
    if(!syscall(380,&layout_info,sizeof(struct pagetable_layout_info)))
    {
        printf("pgdir_shift: %d\npmd_shift: %d\npage_shift: %d\n",
        layout_info.pgdir_shift, layout_info.pmd_shift, layout_info.page_shift);
    }
    else
    {
        printf("System call to get pagetable layout infomation failed!\n ");
    }
    const unsigned long page_size=1<<(layout_info.page_shift);
    const unsigned long pte_size = 1<<(layout_info.pmd_shift-layout_info.page_shift);
    const unsigned long pgd_size = 1<<(32-layout_info.pgdir_shift);
    const unsigned long mask = page_size-1;

    //this allocates virtual memory
    page_table_addr=mmap(NULL,
                        1<<22,//page_size*count,
                        PROT_READ|PROT_WRITE,
                        MAP_SHARED|MAP_ANONYMOUS,
                        -1,
                        0);
    fake_pgd=malloc(sizeof(unsigned long)*page_size);

    if(!page_table_addr||!fake_pgd)
    {
        printf("allocate memory failed!\n");
        return -1;
    }

    int err=syscall(381,pid,fake_pgd,0,page_table_addr,begin_vaddr,end_vaddr);
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


    printf("\npage - frame \n");
    unsigned long i;
    for(i=begin_vaddr>>layout_info.page_shift;i< end_vaddr>>layout_info.page_shift;++i)
    {
        unsigned long pgd_index=pgd_index(i<<layout_info.page_shift,layout_info);
        unsigned long pte_index=pte_index(i<<layout_info.page_shift,layout_info);
        unsigned long* p=fake_pgd[pgd_index];
        if(p)
        {
            unsigned long frame=p[pte_index];
            if(frame>>layout_info.page_shift)
            {
                printf("0x%lx - 0x%lx\n",i,frame>>layout_info.page_shift);
            }
        }
    }

    //free memory space.
    free(fake_pgd);
    munmap(page_table_addr,1<<22);
    printf("------------------------------------------------\n");

    return 0;
}
