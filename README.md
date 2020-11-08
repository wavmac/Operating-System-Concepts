# Operating-System-Concepts

### Project 1 is about fork system call.

- using for loop to create five children process
- identify the order of each children process
- parent process waits until all the children processes terminates

### Project 2 is about v6-file system

***************************
 * Compilation: -$ gcc -o fsaccess fsaccess.c  
 * Run using: -$ ./fsaccess  
***********************************************************************

> main functions

- *initfs (filepath) (# of blocks) (# of I-nodes)*

  This function initializes the v6-file system. All data blocks are in the free list except the root block.

- *cpin (externalfile) (v6-file)*  

  copy the content of an external file into v6 file

- *cpout (v6-file) (externalfile)*  

  copy the content of a v6 file out to a new external file

- *v6Name (v6 file system name)*  
  
  set up the current working v6 file system
  
- *help*

- *q*

  This command saves all changes and quit.
