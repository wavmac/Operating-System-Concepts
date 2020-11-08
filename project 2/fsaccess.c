/*
 * Author 1: Bo-Yu Huang
 * Net ID: bxh190000
 * Author 2: William Vrana
 * Net ID: wav031000
 * CS 5348.001 Operating Systems
 * Prof.S Venkatesan 
 * Project 2
 * 
 ***************************
 * Compilation: -$ gcc -o fsaccess fsaccess.c
 * Run using: -$ ./fsaccess
***********************************************************************/

/***********************************************************************
  
 This program allows user to do five things: 
   1. initfs: Initilizes the file system and redesigning the Unix file system to accept large 
      files of up tp 4GB, expands the free array to 152 elements, expands the i-node array to 
      200 elemnts, doubles the i-node size to 64 bytes and other new features as well.
   2. Quit: save all work and exit the program.
   3. cpin: copy the content of an external file into v6 file
   4. cpout: copy the content of a v6 file out to a new external file
   5. v6Name: set up the current working v6 file system
   6. mkdir: create the v6 directory. It should have two entries '.' and '..'
   7. remove: Delete the file v6_file from the v6 file system.
   8. cd: change working directory of the v6 file system to the v6 directory
   
 User Input:
     - initfs (file path) (# of total system blocks) (# of System i-nodes)
     - v6Name (v6 file system name)
     - cpin (external file name) (v6 file name)
     - cpout (v6 file name) (external file name)
     - q
     - rm (v6 file)
     - mkdir (v6 directory)
     - cd (v6 directory)
     - help
     
 File name is limited to 14 characters.
 ***********************************************************************/

#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>

#define FREE_SIZE 152  
#define I_SIZE 200
#define BLOCK_SIZE 1024    
#define ADDR_SIZE 11
#define INPUT_SIZE 256

// Superblock Structure
typedef struct {
  unsigned short isize;
  unsigned short fsize;
  unsigned short ninode;
  unsigned short nfree;
  unsigned int free[FREE_SIZE];
  unsigned short inode[I_SIZE];
  char flock;
  char ilock;
  unsigned short fmod;
  unsigned short time[2];
} superblock_type;

superblock_type superBlock;

// I-Node Structure
typedef struct {
  unsigned short flags;
  unsigned short nlinks;
  unsigned short uid;
  unsigned short gid;
  unsigned int size;
  unsigned int addr[ADDR_SIZE];
  unsigned short actime[2];
  unsigned short modtime[2];
} inode_type; 

inode_type inode;

// Directory structure
typedef struct {
  unsigned short inode;
  unsigned char filename[14];
} dir_type;

dir_type root;

char v6FileName[14];        // current working v6 file system

int fileDescriptor ;		//file descriptor 
const unsigned short inode_alloc_flag = 0100000;
const unsigned short dir_flag = 040000;
const unsigned short dir_large_file = 010000;
const unsigned short plain_file_flag = 000000;
const unsigned short dir_access_rights = 000777; // User, Group, & World have all access privileges 
const unsigned short INODE_SIZE = 64; // inode has been doubled

// function defined
void setFilename(char *parameters);
int preInitialization(char *parameters);
int initfs(char* path, unsigned short total_blcks,unsigned short total_inodes);
void add_block_to_free_list( int blocknumber , unsigned int *empty_buffer );
void create_root();
int copyIn(char *parameters);
int copyOut(char *parameters);
void showSuper();
void readSuper();
void readInode(unsigned int count);
unsigned int getFreeBlock();
/* new features*/
char *currDirPath;           // current working path in v6 file system
unsigned int getFreeInode();
unsigned int createDir(char *paramters);
void addDir(unsigned int par_Inode, unsigned int par_data_block, unsigned dir_pos_in_data_blcok, char *newDirName);
unsigned int removeFile(char *paramters);
void changeDir(char *newPath);

unsigned int createDir(char *paramters){
  /*
  * check the existence of each file and get the necessary parameters
  * go to addDir(...) for each directory creation
  */

  // does it support multiple creations?
  // ex: mkdir dir1/dir2
  
  // notice the currDirPath!
  unsigned int par_Inode = 1, par_data_block = 2 + superBlock.isize, dir_pos_in_data_blcok = 0;
  char *par_dir_name, *curr_dir_name;

  // go to root data block
  char existFilename[14];
  unsigned int inode_entry=1;
  
  lseek(fileDescriptor, (2 + superBlock.isize)*BLOCK_SIZE+2, 0);
  read(fileDescriptor, &existFilename, 14);
  while(existFilename[0] != '\0'){
    printf("Existing filename: %s\n",existFilename);
    if (strcmp(existFilename, par_dir_name) == 0){
      unsigned short flag;
      lseek(fileDescriptor, 2*BLOCK_SIZE+inode_entry*INODE_SIZE,0);
      read(fileDescriptor,&flag,2);
      if(flag >= (inode_alloc_flag || dir_flag))
        printf("\n filename of the v6-directory is found!\n");
      else {
        printf("\n filename is v6-file, not directory type!\n");
        return -1;
      }
      break;
    }
    
    read(fileDescriptor, inode_entry, 2);
    read(fileDescriptor, existFilename, 14);
    ++dir_pos_in_data_blcok;
  }

  addDir(par_Inode, par_data_block, dir_pos_in_data_blcok, curr_dir_name);

  /************************************************************************/
  // go to the current directory path to check if directory already exist
  /*dir_pos_in_data_blcok = 0;
  while (){
    
  }


  addDir(par_Inode, par_data_block, dir_pos_in_data_blcok, curr_dir_name);
  
  // go through creation 
  while(0){
    
  }*/
}

void addDir(unsigned int par_Inode, unsigned int par_data_block, unsigned dir_pos_in_data_blcok, char *newDirName){
  /* 
  * Create new directory: 
  * Initialize its data block with two directory entries (16 bytes) and its i-node in i-node blocks
  * Add a new directory entry (16 bytes) in its parent data block
  */

  int new_data_block = getFreeBlock();
  int i;
  
  dir_type new_dir;
  if (getFreeInode != -1)
    new_dir.inode = getFreeInode();   // get available inode number.
  new_dir.filename[0] = '.';
  new_dir.filename[1] = '\0';
  
  inode.flags = inode_alloc_flag | dir_flag | dir_access_rights;   		// flag for directory type
  inode.nlinks = 0; 
  inode.uid = 0;
  inode.gid = 0;
  inode.size = INODE_SIZE;
  inode.addr[0] = new_data_block;
  
  for( i = 1; i < ADDR_SIZE; i++ ) {
    inode.addr[i] = 0;
  }
  
  inode.actime[0] = 0;
  inode.modtime[0] = 0;
  inode.modtime[1] = 0;
  
  // wrtie the i-node entry
  lseek(fileDescriptor, 2 * BLOCK_SIZE+(new_dir.inode-1)*INODE_SIZE, 0);
  write(fileDescriptor, &inode, INODE_SIZE);
  
  // wrtie on the self data block
  lseek(fileDescriptor, new_data_block*BLOCK_SIZE, 0);
  write(fileDescriptor, &new_dir, 16);
  
  dir_type par_dir;
  par.inode = par_Inode;
  par_dir.filename[0] = '.';
  par_dir.filename[1] = '.';
  par_dir.filename[2] = '\0';
  
  write(fileDescriptor, &par_dir, 16);
  
  // wrtie on the parent data block
  int i;
  
  for (i = 0; i < 14; i++){
    if (newDirName[i] == '\0'){
      new_dir.filename[i] = '\0';
      break;
    }
    new_dir.filename[i] = newDirName[i];
  }
  
  lseek(fileDescriptor, par_data_block*BLOCK_SIZE + dir_pos_in_data_blcok*16, 0);
  write(fileDescriptor, &new_dir, 16);
}

unsigned int getFreeInode(){
  /*
  * omit superBlock.ninode and superBlock inode[]
  * instead, search through the i-node block 
  */
  
  unsigned int count = 1;
  unsigned int curr = 2;
  lseek(fileDescriptor, 2 * BLOCK_SIZE + INODE_SIZE, 0);
  
  while (curr != 0){
      // if curr(flag) == 0, available i-node space is found
      read(fileDescriptor, &curr, 2);
      if (curr == 1){
          // if curr == 1, reach the first data block (i-node entry of root in root data block)
          printf("The i-node block for the v6 file system is full!");
          return -1;
      }
      lseek(fileDescriptor, 62, SEEK_CUR);
      ++count;
  }
  
  return count;
}


unsigned int removeFile(char *paramters){
  /*
  * remove a file (filepath) starts from current directory path
  * 
  */  

  // go to the current directory path to check if v6 file exist, if file not exist, abort
  
  /***** go to the corresdoning v6 file and start deleting *****/
  
  // go to the inode entry of the v6 file, find data blocks in inode.addr[] and free them
  // do we add those freed data blocks into superBlock.free?
  
  
  
  
  // free the inode entry and directory type in working directory data block
  
  
}

void changeDir(char *newPath){
  /*
  * change the global variable "currDirPath"
  * support cd .././..
  */
    
  // looping through the entire directory command path


    
}

int main() {
 
  char input[INPUT_SIZE];
  char *splitter;
  unsigned int numBlocks = 0, numInodes = 0;
  char *filepath;
  printf("Size of super block = %d , size of i-node = %d\n",sizeof(superBlock),sizeof(inode));
  printf("");
  while(1) {
    printf("\nEnter command or 'help' for command instruction:\n");
    printf(currDirPath);
    printf("\n");

    scanf(" %[^\n]s", input);
    splitter = strtok(input," ");
    
    if(strcmp(splitter, "initfs") == 0){
    
        preInitialization(splitter);
        splitter = NULL;
                       
    } else if (strcmp(splitter, "v6Name") == 0){
        splitter = strtok(NULL, " ");
        setFilename(splitter);
        splitter = NULL;

    } else if (strcmp(splitter, "cpin") == 0) {

        if (v6FileName == NULL || v6FileName[0] == '\0'){
          printf("Choose a v6-file system by entering filename of it using command: v6Name <filename>\n");
          continue;
        }

        if (copyIn(splitter)){
          printf("\nSuccessfully copy in!\n");
        }
        
        splitter = NULL;

    } else if (strcmp(splitter, "cpout") == 0) {

        if (v6FileName == NULL || v6FileName[0] == '\0'){
          printf("Choose a v6-file system by entering filename of it using command: v6Name <filename>\n");
          continue;
        }

        if (copyOut(splitter)){
          printf("\nSuccessfully copy out!\n");
        }
        
        splitter = NULL;

    } else if (strcmp(splitter, "help") == 0){

        printf("\nInitalization v6 file system: initfs <filename> <# of block> <# of i-nodes>\n");
        printf("\nChoose the v6 file system: v6Name <filename>\n");
        printf("\ncopy external file into v6 file system: cpin <externalfile> <v6-file>\n");
        printf("\ncopy file in v6 file system out to external file: cpout <v6-file> <externalfile>\n");
        printf("\nexit the program: q\n");
        splitter = NULL;

    } else if (strcmp(splitter, "q") == 0) {
   
       lseek(fileDescriptor, BLOCK_SIZE, SEEK_SET);
       write(fileDescriptor, &superBlock, BLOCK_SIZE);
       return 0;
    }
  }
}


void setFilename(char *parameters){
  /* 
  * Set up the name of the v6 file system
  * Read the superBlock info in the corresponding v6 file system
  */

  int i,j;
  for (i = 0; i < 14; i++){
    if (parameters[i] == '\0'){
      for (j = i; j < 14; j++)
        v6FileName[j] = '\0';
      break;
    }
    v6FileName[i] = parameters[i];
  }

  if((fileDescriptor = open(v6FileName,O_RDWR,0700))== -1) {
    printf("\n No such file exist or open() failed with the following error [%s]\n",strerror(errno));
    v6FileName[0] = '\0';
    return;
  }

  printf("The current v6 file path (name) is %s\n",v6FileName);
  
  readSuper();
  showSuper();
}

int preInitialization(char *parameters){
  /* 
  * Decode the input parameters
  * finish initializing the v6 file system 
  */

  char *n1, *n2;
  unsigned int numBlocks = 0, numInodes = 0;
  char *filepath;
  
  parameters = strtok(NULL, " ");
  filepath = parameters;
  parameters = strtok(NULL, " ");
  n1 = parameters;
  parameters = strtok(NULL, " ");
  n2 = parameters;
  
  if(access(filepath, F_OK) != -1) {
      
      if(fileDescriptor = open(filepath, O_RDWR, 0600) == -1){
      
         printf("\n filesystem already exists but open() failed with error [%s]\n", strerror(errno));
         return 1;
      }
      printf("filesystem already exists and the same will be used.\n");
  
  } else {
  
        	if (!n1 || !n2)
              printf(" All arguments(path, number of inodes and total number of blocks) have not been entered\n");
            
       		else {
          		numBlocks = atoi(n1);
          		numInodes = atoi(n2);
          		
          		if( initfs(filepath, numBlocks, numInodes )){
                setFilename(filepath);
          		  printf("The file system is initialized\n");	
          		} else {
            		printf("Error initializing file system. Exiting... \n");
            		return 1;
          		}
       		}
  }
}

int initfs(char* path, unsigned short blocks,unsigned short inodes) {
  /* 
  * finish initializing the v6 file system 
  * create superBlock and corresponding INode and data block
  * create buffer and reuse them to write data block
  */
  
  unsigned int buffer[BLOCK_SIZE/4];
  int bytes_written;
  
  // superblock creation
  unsigned short i = 0;
  superBlock.fsize = blocks;
  unsigned short inodes_per_block= BLOCK_SIZE/INODE_SIZE; // = 16
  
  if((inodes%inodes_per_block) == 0)
  superBlock.isize = inodes/inodes_per_block;
  else
  superBlock.isize = (inodes/inodes_per_block) + 1;
  
  if((fileDescriptor = open(path,O_RDWR|O_CREAT,0700))== -1) {
    printf("\n open() failed with the following error [%s]\n",strerror(errno));
    return 0;
  }
      
  for (i = 0; i < FREE_SIZE; i++)
    superBlock.free[i] =  0;			//initializing free array to 0 to remove junk data. free array will be stored with data block numbers shortly.
  
  superBlock.nfree = 0;
  superBlock.ninode = I_SIZE;
  
  for (i = 0; i < I_SIZE; i++)
    superBlock.inode[i] = i + 1;		//Initializing the inode array to inode numbers
  
  superBlock.flock = 'a'; 					//flock,ilock and fmode are not used.
  superBlock.ilock = 'b';					
  superBlock.fmod = 0;
  superBlock.time[0] = 0;
  superBlock.time[1] = 1970;
  
  // writing zeroes to reusable buffer
  for (i = 0; i < BLOCK_SIZE/4; i++) 
    buffer[i] = 0;
      
  for (i = 0; i < superBlock.isize; i++)
    write(fileDescriptor, buffer, BLOCK_SIZE);
  
  int data_blocks = blocks - 2 - superBlock.isize;
  int data_blocks_for_free_list = data_blocks - 1;
  
  // Create root directory
  create_root();

  for ( i = 0; i < data_blocks_for_free_list; i++ ) {
    add_block_to_free_list(i + 2 + superBlock.isize + 1, buffer);
  }
  
  lseek(fileDescriptor, BLOCK_SIZE, SEEK_SET);
  write(fileDescriptor, &superBlock, BLOCK_SIZE); // writing superblock to file system
  
  return 1;
}

void add_block_to_free_list(int block_number,  unsigned int *empty_buffer){
  /* 
  * Add Data blocks to free list 
  * change nfree and free in superBlock simultaneously
  */

  if ( superBlock.nfree == FREE_SIZE ) {

    int free_list_data[BLOCK_SIZE / 4], i;
    free_list_data[0] = FREE_SIZE;
    
    for ( i = 0; i < BLOCK_SIZE / 4; i++ ) {
       if ( i < FREE_SIZE ) {
         free_list_data[i + 1] = superBlock.free[i];
       } else {
         free_list_data[i + 1] = 0; // getting rid of junk data in the remaining unused bytes of header block
       }
    }
    
    lseek( fileDescriptor, (block_number) * BLOCK_SIZE, 0 );
    write( fileDescriptor, free_list_data, BLOCK_SIZE ); // Writing free list to header block
    
    superBlock.nfree = 0;
    
  } else {

	lseek( fileDescriptor, (block_number) * BLOCK_SIZE, 0 );
    write( fileDescriptor, empty_buffer, BLOCK_SIZE );  // writing 0 to remaining data blocks to get rid of junk data
  }

  superBlock.free[superBlock.nfree] = block_number;  // Assigning blocks to free array
  ++superBlock.nfree;
}


void create_root() {
  /* 
  * Create root directory 
  * create superBlock and corresponding INode and directory entry in root data block
  */

  int root_data_block = 2 + superBlock.isize; // Allocating first data block to root directory
  int i;
  
  root.inode = 1;   // root directory's inode number is 1.
  root.filename[0] = '.';
  root.filename[1] = '\0';
  
  inode.flags = inode_alloc_flag | dir_flag | dir_large_file | dir_access_rights;   		// flag for root directory 
  inode.nlinks = 0; 
  inode.uid = 0;
  inode.gid = 0;
  inode.size = INODE_SIZE; // 64 bits (if 32 bits => 4GB)
  inode.addr[0] = root_data_block;
  
  for( i = 1; i < ADDR_SIZE; i++ ) {
    inode.addr[i] = 0;
  }
  
  inode.actime[0] = 0;
  inode.modtime[0] = 0;
  inode.modtime[1] = 0;
  
  lseek(fileDescriptor, 2 * BLOCK_SIZE, 0);
  write(fileDescriptor, &inode, INODE_SIZE);
  
  lseek(fileDescriptor, root_data_block*BLOCK_SIZE, 0);
  write(fileDescriptor, &root, 16);
  
  root.filename[0] = '.';
  root.filename[1] = '.';
  root.filename[2] = '\0';
  
  write(fileDescriptor, &root, 16);
 
}

int copyIn(char *parameters) {
  /* 
  * copy external file into v6 file 
  * check if the v6-filename already exist
  * open external file and check if exist
  * create a new v6 file in file system 
  * copy the content byte by byte from external file into v6 file
  * update superBlock and store new v6 file I-node
  */

  char *extFilePath, *vFile;

  parameters = strtok(NULL, " ");
  extFilePath = parameters;
  parameters = strtok(NULL, " ");
  vFile = parameters;
  
  if((fileDescriptor = open(v6FileName,O_RDWR,0700))== -1){
    printf("\n v6 file system open() failed with the following error [%s]\n",strerror(errno));
    printf("The current v6 file path (name) is %s\n",v6FileName);
    return 0;
  }
  
  // load the superblock data
  readSuper();
  
  // go to first data block to see if the v6-filename already exist (future improvement: handle number of root data blocks > 1)
  char existFilename[14];
  unsigned int countInode = 0;
  
  lseek(fileDescriptor, (2 + superBlock.isize)*BLOCK_SIZE + 2, 0);
  read(fileDescriptor, &existFilename, 14);
  
  while(existFilename[0] != '\0'){
    /* if (countInode == BLOCK_SIZE / 16){
        // only when number of root data blocks > 1
        countInode = 0;
        num_root_data_block += 1;  // initialized to 1
        lseek(fileDescriptor, 2*BLOCK_SIZE + 12 + 4*(num_root_data_block-1), SEEK_SET);  //inode.addr[num_root_data_block-1] of superBlock
        int tmp;
        read(fileDescriptor, &tmp, 4);
        lseek(fileDescriptor, tmp*BLOCK_SIZE, SEEK_SET);
    }*/

    printf("Existing filename: %s\n",existFilename);
    if (strcmp(existFilename, vFile) == 0){
      printf("\n filename of the v6-File is already exist! copy in failed! Abort....\n");
      return 0;
    }
    lseek(fileDescriptor, 2, SEEK_CUR);
    read(fileDescriptor, &existFilename, 14);
    ++countInode;
  }

  // open the external file and check if exist
  int extFileDescriptor;
  if((extFileDescriptor = open(extFilePath,O_RDONLY))== -1){
    printf("\n external file open() failed with the following error [%s]\n",strerror(errno));
    return 0;
  }

  // initialize the v6-file inode
  unsigned int vFile_data_block = getFreeBlock(); // should look up superBlock.free
  if (vFile_data_block == -1){
    printf("\n The v6 file system is full!\n");
    return 0;
  }
  //printf("free block number: %i\n",vFile_data_block);

  inode.flags = inode_alloc_flag | plain_file_flag | dir_access_rights; // flag for new plain small file
  inode.nlinks = 0;
  inode.uid = 0;
  inode.gid = 0;
  inode.size = BLOCK_SIZE;  // (unsolved, but occupy at least one data block)
  inode.addr[0] = vFile_data_block;
  
  int i;
  for( i = 1; i < ADDR_SIZE; i++ ) {
    inode.addr[i] = 0;
  }
  
  inode.actime[0] = 0;
  inode.modtime[0] = 0;
  inode.modtime[1] = 0;
  
  // copy the content, update nfree and free in superblock (update 'addr' and 'size' in vFile inode)
  char ch[1];
  int curr_block_bytes = 0;
  short num_addr = 1;
  lseek(fileDescriptor, vFile_data_block*BLOCK_SIZE, SEEK_SET);
  while (read(extFileDescriptor, ch, 1)) { 
    if (curr_block_bytes == BLOCK_SIZE){
      // if the current data block is full
      vFile_data_block = getFreeBlock();
      if (vFile_data_block == -1){
        printf("\n The v6 file system is full!\n");
        return 0;
      }
      inode.addr[num_addr] = vFile_data_block;
      inode.size += BLOCK_SIZE;
      ++num_addr;
      curr_block_bytes = 0;
      lseek(fileDescriptor, vFile_data_block*BLOCK_SIZE, SEEK_SET);
    }

    write(fileDescriptor, ch, 1);
    ++curr_block_bytes;
  }
  
  // update the directory in root data block
  dir_type vFile_dir;
  vFile_dir.inode = countInode;
  for (i = 0; i < 14; i++){
    if (vFile[i] == '\0'){
      vFile_dir.filename[i] = '\0';
      break;
    }
    vFile_dir.filename[i] = vFile[i];
  }

  // change the i-node entry of the root (specifically only its addr, because the number of file in the root could be larger than 64)
  /* if (countInode == BLOCK_SIZE / 16){
    // the current root data block happens to be full, get the new free block for root
    // num_root_data_block: current number of full data blocks that belongs to root
    // update Inode of root
    
    unsigned int new_root_data_block = getFreeBlock();
    lseek(fileDescriptor, 2 * BLOCK_SIZE + 8, 0);
    write(fileDescriptor, (num_root_data_block+1)*BLOCK_SIZE, 4);
    lseek(fileDescriptor, 4*num_root_data_block, SEEK_CUR);
    write(fileDescriptor, new_root_data_block, 4);

    lseek(fileDescriptor, new_root_data_block*BLOCK_SIZE, 0);
    write(fileDescriptor, &vFile_dir, 16);
  } else {
    // the current root data block is not full
    // num_root_data_block: current number of full data blocks that belongs to root
    // last_root_data_block = inode.addr[num_root_data_block-1] of superBlock
    // update data block of root

    lseek(fileDescriptor, last_root_data_block*BLOCK_SIZE+countInode*16, 0);
    write(fileDescriptor, &vFile_dir, 16);
  }
  */
  
  lseek(fileDescriptor, (2 + superBlock.isize)*BLOCK_SIZE+countInode*16, 0);
  write(fileDescriptor, &vFile_dir, 16);
  
  // store new v6-file inode
  lseek(fileDescriptor, 2 * BLOCK_SIZE + (countInode-1)*INODE_SIZE, 0);
  write(fileDescriptor, &inode, INODE_SIZE);
  
  // update superBlock
  lseek(fileDescriptor, BLOCK_SIZE, SEEK_SET);
  write(fileDescriptor, &superBlock, BLOCK_SIZE);

  return 1;
}

void showSuper(){
  /*
   * display the superBlock info
  */

  printf("SuperBlock info \n\n");

  printf("superBlock.isize: %hu\n", superBlock.isize);
  printf("superBlock.fsize: %hu\n", superBlock.fsize);
  printf("superBlock.nfree: %hu\n", superBlock.nfree);
  printf("superBlock.free[%hu]: %i\n", superBlock.nfree-1, superBlock.free[superBlock.nfree-1]);
  printf("superBlock.ninode: %hu\n", superBlock.ninode);
  printf("superBlock.inode: %hu\n", superBlock.inode[I_SIZE-1]);
  printf("superBlock.flock: %c\n", superBlock.flock);
  printf("superBlock.ilock: %c\n", superBlock.ilock);
  printf("superBlock.fmod: %hu\n", superBlock.fmod);
  printf("superBlock.time[1]: %hu\n\n", superBlock.time[1]);
}


void readSuper(){
  /*
   * load the superBlock info
  */

  lseek(fileDescriptor, BLOCK_SIZE, 0);
  
  read(fileDescriptor, &superBlock.isize, 2);
  read(fileDescriptor, &superBlock.fsize, 2);
  read(fileDescriptor, &superBlock.ninode, 2);
  read(fileDescriptor, &superBlock.nfree, 2);
  read(fileDescriptor, &superBlock.free, 4*FREE_SIZE);
  read(fileDescriptor, &superBlock.inode, 2*I_SIZE);
  read(fileDescriptor, &superBlock.flock, 1);
  read(fileDescriptor, &superBlock.ilock, 1);
  read(fileDescriptor, &superBlock.fmod, 2);
  read(fileDescriptor, &superBlock.time, 2*2);
}

void readInode(unsigned int count){
  /*
   * load the I-node info according to the I-node number
  */

  lseek(fileDescriptor, 2*BLOCK_SIZE+INODE_SIZE*(count-1), SEEK_SET);

  read(fileDescriptor, &inode.flags, 2);
  read(fileDescriptor, &inode.nlinks, 2);
  read(fileDescriptor, &inode.uid, 2);
  read(fileDescriptor, &inode.gid, 2);
  read(fileDescriptor, &inode.size, 4);
  read(fileDescriptor, &inode.addr, 4*ADDR_SIZE);
  read(fileDescriptor, &inode.actime, 4);
  read(fileDescriptor, &inode.modtime, 4);
}

unsigned int getFreeBlock(){
  /*
   * get the new free data block
   * if it's empty, go to next data block that stores indexes of free data block 
  */

  --superBlock.nfree;
  int acquired_free_data_block = superBlock.free[superBlock.nfree];
  if (superBlock.nfree == 0){
    printf("check if nfree is empty!\n");
    if (superBlock.free[0] == 0){
      printf("\n The v6 file system is full! Write failed! Abort...\n");
      return -1;
    }
    
    lseek(fileDescriptor, acquired_free_data_block*BLOCK_SIZE+6, SEEK_SET);
    read(fileDescriptor, &superBlock.nfree, 2);
    read(fileDescriptor, &superBlock.free, 4*FREE_SIZE);

    return superBlock.free[--superBlock.nfree];
    
    // free the block originally contain indexes of free blocks
    // add_block_to_free_list(index_data_block, buffer) 
    
  }

  return acquired_free_data_block;
}


int copyOut(char *parameters) {
  /* 
  * copy v6 file out to an external file 
  * check if the v6-filename exist
  * open external file and overwrite it
  * copy the content byte by byte from v6 file out to the external file
  * stop when enters '\0' character
  */

  char *extFilePath, *vFile;

  parameters = strtok(NULL, " ");
  vFile = parameters;
  parameters = strtok(NULL, " ");
  extFilePath = parameters;
  
  if((fileDescriptor = open(v6FileName,O_RDWR,0700))== -1){
    printf("\n v6 file system open() failed with the following error [%s]\n",strerror(errno));
    return 0;
  }
  
  // load the superblock data
  readSuper();

  // go to first data block to see if the v6-filename exist
  char existFilename[14];
  unsigned int countInode = 0;
  
  lseek(fileDescriptor, (2 + superBlock.isize)*BLOCK_SIZE + 2, 0);
  read(fileDescriptor, &existFilename, 14);
  while(existFilename[0] != '\0'){
    printf("Existing filename: %s\n",existFilename);
    if (strcmp(existFilename, vFile) == 0){
      printf("\n filename of the v6-File is found! Ready to copy out!\n");
      break;
    }
    lseek(fileDescriptor, 2, SEEK_CUR);
    read(fileDescriptor, existFilename, 14);
    ++countInode;
  }

  // if no such file name exist in v6 file system, break
  if (existFilename[0] == '\0'){
    printf("\n filename of the v6-File is not found! Abort...\n");
    return 0;
  }
  
  // create the new external file
  int extFileDescriptor;
  extFileDescriptor = open(extFilePath,O_RDWR|O_CREAT,0700);
  
  // read the inode entry of the v6-file and get the addr array
  readInode(countInode);

  // copy the contentinto the new external file
  char ch[1];
  int curr_block_bytes = 0;
  unsigned short num_addr = 0;
  lseek(fileDescriptor, inode.addr[0]*BLOCK_SIZE, SEEK_SET);
  read(fileDescriptor, ch, 1);
  while (ch[0] != 0) { 
    write(extFileDescriptor, ch, 1);
    ++curr_block_bytes;
    
    if (curr_block_bytes == BLOCK_SIZE){
      // if the current data block is copied completely
      ++num_addr;
      if (inode.addr[num_addr] == 0)
        break;
      lseek(fileDescriptor, inode.addr[num_addr]*BLOCK_SIZE, SEEK_SET);
      curr_block_bytes = 0;
    }
    
    read(fileDescriptor, ch, 1);
  }

  return 1;
}