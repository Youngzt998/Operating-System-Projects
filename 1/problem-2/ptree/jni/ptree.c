/*
    Code of Problem 2
    Student Name:Ziteng Yang
    Student ID: 517021910683
*/

/*
    Note:
    This user program call a system call to get a process tree information with DFS sequence.
    The DFS was done in the system call, sequencing all process information and pass information including depth to this program.
    (We can also realize this in a user program by build a hash table of pid, but it might use too much unnecessary memory.)
*/

#include<stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NR 100

struct prinfo{
    pid_t parent_pid;       /* process id of parent */
    pid_t pid;              /* process id */
    pid_t first_child_pid;  /* pidof youngest child */
    pid_t next_sibling_pid; /* pidof older sibling */
    long state;             /* current state of process */
    long uid;               /* user id of process owner */
    char comm[64];          /* name of program executed */
    int depth;
};

int main()
{
    //memory allocation
    struct prinfo info[MAX_NR];
    int *nr;
    nr = (int*)malloc(sizeof(int));

    (*nr)=MAX_NR;
    
    printf("*nr=:%d\n",*nr);
    int process_counter =syscall(356,info,nr);

    printf("Number of process: %d\n",process_counter);
    
    printf("Name\tPID\tState\tPPID\tFCPID\tNSPID\tUID\n");
    int i,j;
    for(i=0;i<process_counter;++i)
    {   
            for(j=0;j<info[i].depth;++j)printf("\t");
            printf("%s\t",info[i].comm);
            printf("%d\t",info[i].pid);
            printf("%ld\t",info[i].state);
            printf("%d\t",info[i].parent_pid);
            printf("%d\t",info[i].first_child_pid);
            printf("%d\t",info[i].next_sibling_pid);
            printf("%ld\t",info[i].uid);
            printf("\n");
    }


    
    return 0;
}
