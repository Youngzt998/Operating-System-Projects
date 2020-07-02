/*
    Student Name: Ziteng Yang
    StudentID: 517021910683
*/

/*
    Note: This system call can print process tree in DFS sequences, on dmesg screen.
    It can also give this sequence to a user program using the struct prinfo.
    We use a depth varieble to denote how many tabs we need to print to denote a tree
*/

#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/sched.h>
#include<linux/unistd.h>
//#include <stdlib.h>
MODULE_LICENSE("Dual BSD/GPL");

#define __NR_hellocall 356
#define MAX_PID 32767
struct prinfo{
    pid_t parent_pid;        /* process id of parent */
    pid_t pid;               /* process id */
    pid_t first_child_pid;   /* pidof youngest child */
    pid_t next_sibling_pid;  /* pidof older sibling */
    long state;              /* current state of process */
    long uid;                /* user id of process owner */
    char comm[64];           /* name of program executed */
    int depth;               /* number of tabs to printf/printk; can be used for ptree program*/
};


//static bool visited[2000];

static int(*oldcall)(void);


static void ptree_dfs(struct task_struct *p,struct prinfo*info, int *nr, int *process_counter,int *depth)
{   
    //for each node

    //get information of it self
        struct task_struct* ts;
        if(p){
            //print tab
            int i;
            for(i=0;i<*depth;++i) printk("\t-");
            info[*process_counter].depth=*depth;

            //get name
            for(i=0;i<TASK_COMM_LEN;++i){
                info[*process_counter].comm[i]=p->comm[i];
            }
            
            //get pid
            info[*process_counter].pid=p->pid;
            printk("i:%d\tpid:%d\t",*process_counter,p->pid);
            
            //get parent_pid
            printk("parent_pid:%d\t",p->real_parent->pid);
            info[*process_counter].parent_pid=p->real_parent->pid;
            
            //get first child's pid
            if(!list_empty(&(p->children))) {     //has a child
                ts=list_entry(p->children.next, struct task_struct, sibling);  //children.next points to child's silbling field 
                info[*process_counter].first_child_pid=ts->pid;
            }
            else info[*process_counter].first_child_pid=0;
            printk("first_child_pid:%d\t",info[*process_counter].first_child_pid);

            //get next sibling's pid
            if(p->sibling.next!=&(p->parent->children)){//has a sibling
                ts=list_entry(p->sibling.next,struct task_struct,sibling);
                info[*process_counter].next_sibling_pid=ts->pid;
            }
            else info[*process_counter].next_sibling_pid=0;
            printk("next_sibling_pid:%d\t",info[*process_counter].next_sibling_pid);

            // get state
            info[*process_counter].state=p->state;
            printk("state:%ld\t",p->state);
            

            //get uid
               if(p->real_cred)
                   info[*process_counter].uid=p->real_cred->uid;
            
            printk("uid:%ld\t",info[*process_counter].uid);
            
            printk("name:%s\n",p->comm);
        }
        ++(*process_counter);
        if((*process_counter)>=(*nr)) return;   //exceeded pu-bound


    //get into its every child
    if(!list_empty(&(p->children))){    //has a child
        struct task_struct *tmp=list_entry(p->children.next,struct task_struct, sibling);   //go to child
        while(true) 
        {
            (*depth)++;
            ptree_dfs(tmp,info,nr,process_counter,depth); 
            (*depth)--;
            if(tmp->sibling.next==&(tmp->parent->children))break;  //no sibling
            tmp=list_entry(tmp->sibling.next,struct task_struct, sibling);
        }
    }

}

static void get_ptree(struct prinfo*info, int *nr,int* process_counter)
{
    int depth_counter=0;    //how many "tab" to print
    int *depth=&depth_counter;
    ptree_dfs(&init_task, info, nr,process_counter,depth);	//start from init_task
}


/*working place*/
/*main working function*/
static int ptree(struct prinfo *info,int *nr)
{
    if(!info || !nr) return 0;	//in case of panic caused by other process

    int process_counter=0;
    int* p_counter=&process_counter;

    struct task_struct*p=&init_task;

	//go into the DFS function
    read_lock(&tasklist_lock);
        get_ptree(info,nr,p_counter);
    read_unlock(&tasklist_lock);

    printk("system call end!\n");
    return process_counter;
}

static int addsyscall_init(void)
{
    long *syscall = (long*)0xc000d8c4;
    oldcall = (int(*)(void))(syscall[__NR_hellocall]);
    syscall[__NR_hellocall]= (unsigned long) ptree;
    printk(KERN_INFO "module load!\n");
    return 0;
}

static void addsyscall_exit(void)
{
    long *syscall = (long*)0xc000d8c4;
    syscall[__NR_hellocall] = (unsigned long)oldcall;
    printk(KERN_INFO "module exit!\n"); 
}


module_init(addsyscall_init);
module_exit(addsyscall_exit);


    /*****************
        previous code
    ******************/

    //struct task_struct *ts;
    // info[process_counter].pid=p->pid;

    //  if(p->real_parent)
    //      info[process_counter].parent_pid=p->real_parent->pid;
    //  else info[process_counter].parent_pid=0;

    // ++process_counter;


	// read_lock(&tasklist_lock);
	// printk("nr = %d,\n root process id = %d\n", *nr, p->pid);

	// for_each_process(p) //except init
	// {
    //     ++process_counter;
    //     if(process_counter>=(*nr)) break;   //exceeded pu-bound

    //     if(p){
    //         //get name
    //         int i;
    //         for(i=0;i<TASK_COMM_LEN;++i){
    //             info[process_counter].comm[i]=p->comm[i];
    //         }
            
    //         //get pid
    //         info[process_counter].pid=p->pid;
    //         printk("i:%d\tpid:%d\t",process_counter,p->pid);
            
    //         //get parent_pid
    //         printk("parent_pid:%d\t",p->real_parent->pid);
    //         info[process_counter].parent_pid=p->real_parent->pid;
            
    //         //get first child's pid
    //         if(!list_empty(&(p->children))) {     //has a child
    //             ts=list_entry(p->children.next, struct task_struct, sibling);  //children.next points to child's silbling field 
    //             info[process_counter].first_child_pid=ts->pid;
    //         }
    //         else info[process_counter].first_child_pid=0;
    //         printk("first_child_pid:%d\t",info[process_counter].first_child_pid);

    //         //get next sibling's pid
    //         if(p->sibling.next!=&(p->real_parent->children)){
    //             ts=list_entry(p->sibling.next,struct task_struct,sibling);
    //             info[process_counter].next_sibling_pid=ts->pid;
    //         }
    //         else info[process_counter].next_sibling_pid=0;
    //         printk("next_sibling_pid:%d\t",info[process_counter].next_sibling_pid);

    //         // get state
    //         info[process_counter].state=p->state;
    //         printk("state:%ld\n",p->state);

    //         printk("name:%s\t",p->comm);
    //     }
        
    //     if(process_counter>=(*nr)) break; //up-bound

	// }
	// read_unlock(&tasklist_lock);

    // return process_counter;
