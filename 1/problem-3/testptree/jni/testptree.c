#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
int main()
{
    pid_t pid=getpid();
    pid_t child_pid;

    /* generate a new process */
    child_pid = fork();
    if (child_pid < 0) { /* error occurred */
        printf("Create child process failed\n");
        return 1;
    }
    else if (child_pid == 0) { /* child process */
        execl("/data/misc/ptree","ptree",NULL);
    }
    else { /* parent process */
        /* parent will wait for the child to complete */
        wait(NULL);
        //printf("Child Complete");
        printf("StudentID: 517021910683 Parent PID is: %d\n",pid);
        printf("StudentID: 517021910683 Child PID is: %d\n",child_pid);
    }
    return 0;
}