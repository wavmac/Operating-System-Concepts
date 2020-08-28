#include <stdio.h>

main(){
    int p = fork();
    
    for (int i = 0; i < 5; i++){
        p = fork();
    }
    
    if (p == 0){
        printf("P value of the children is %d\n", p);
        printf("The PID of the children %d\n", getpid());
        printf("The PID of the parent %d\n", getppid());
        printf("I am child process number %d. Terminating now.\n", getpid()-getppid());
    }
    
    //printf("Common part\n");
}