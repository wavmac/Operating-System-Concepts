// Name: Bo-Yu Huang
// UTD id: 2021485527, NET id: bxh190000
// Operating System Concepts project 1

/**
     * if a program contains a call to fork( ), 
     * the execution of the program results in the execution of two processes. 
     * One process is created to start executing the program. 
     * When the fork( ) system call is executed, another process is created. 
     * The original process is called the parent process and the second process is 
     * called the child process.
     
     * The child process is an almost exact copy of the parent process. 
     * Both processes continue executing from the point where the fork( ) calls returns 
     * execution to the main program. Since UNIX is a time-shared operating system, 
     * the two processes can execute concurrently.
     *
*/
#include <stdio.h>
#include <unistd.h>

int main(){
    int p = -5;
    
    // create 5 child process
    for (int i = 0; i < 5; i++){
        p = fork();
        
        if (p == 0){
            // To avoid looping through again
            // if the return p value is 0 (child process), break out.
            break;
        }
    }
    
    if (p == 0){
        // child process
        
        printf("I am child process number %d. Terminating now.\n", getpid()-getppid());
        // print the difference between the PID of original parent process and the PID 
        // of child process created by fork() call in the parent
    }
    else {
        // parent process, p equals to the PID of the last child 
        
        printf(""); // print nothing if it's the origin parent process
        sleep(1);
        // wait();  // guarantee to wait for all the children to finish first
        
    }
    
    //printf("Common part\n");
}