/***********************************************************************
 
 
 
 This program allows user to do two things: 
   1. initfs: Initilizes the file system and redesigning the Unix file system to accept large 
      files of up tp 4GB, expands the free array to 152 elements, expands the i-node array to 
      200 elemnts, doubles the i-node size to 64 bytes and other new features as well.
   2. Quit: save all work and exit the program.
   
 User Input:
     - initfs (file path) (# of total system blocks) (# of System i-nodes)
     - q
     
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
//#pragma pack(1)
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

typedef struct {
  unsigned short inode;
  unsigned char filename[14];
} dir_type;

dir_type root;

int fileDescriptor ;		//file descriptor 
const unsigned short inode_alloc_flag = 0100000;
const unsigned short dir_flag = 040000;
const unsigned short dir_large_file = 010000;
const unsigned short plain_file_flag = 000000;
const unsigned short dir_access_rights = 000777; // User, Group, & World have all access privileges 
const unsigned short INODE_SIZE = 64; // inode has been doubled

// function defined
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
unsigned short getFreeInode();

int main() {
 
  char input[INPUT_SIZE];
  char *splitter;
  unsigned int numBlocks = 0, numInodes = 0;
  char *filepath;
  printf("Size of super block = %d , size of i-node = %d\n",sizeof(superBlock),sizeof(inode));
  
  while(1) {
    printf("\nEnter command:\n");

    scanf(" %[^\n]s", input);
    splitter = strtok(input," ");
    
    if(strcmp(splitter, "initfs") == 0){
    
        preInitialization(splitter);
        splitter = NULL;
                       
    } else if (strcmp(splitter, "cpin") == 0) {
        
        copyIn(splitter);
        splitter = NULL;

    } else if (strcmp(splitter, "cpout") == 0) {
        
        copyOut(splitter);
        splitter = NULL;

    } else if (strcmp(splitter, "q") == 0) {
   
       lseek(fileDescriptor, BLOCK_SIZE, SEEK_SET);
       write(fileDescriptor, &superBlock, BLOCK_SIZE);
       return 0;
    }
  }
}

int preInitialization(char *parameters){

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
          		  printf("The file system is initialized\n");	
          		} else {
            		printf("Error initializing file system. Exiting... \n");
            		return 1;
          		}
       		}
  }
}

int initfs(char* path, unsigned short blocks,unsigned short inodes) {

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
  
  // writing zeroes to all inodes in ilist
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

// Add Data blocks to free list
void add_block_to_free_list(int block_number,  unsigned int *empty_buffer){

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

// Create root directory
void create_root() {

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
  write(fileDescriptor, &inode, INODE_SIZE);   // 
  
  lseek(fileDescriptor, root_data_block*BLOCK_SIZE, 0);
  write(fileDescriptor, &root, 16);
  
  root.filename[0] = '.';
  root.filename[1] = '.';
  root.filename[2] = '\0';
  
  write(fileDescriptor, &root, 16);
 
}

int copyIn(char *parameters) {
  char *extFilePath, *vFile;

  parameters = strtok(NULL, " ");
  extFilePath = parameters;
  parameters = strtok(NULL, " ");
  vFile = parameters;
  
  if((fileDescriptor = open("disk",O_RDWR,0700))== -1){
    printf("\n open() failed with the following error [%s]\n",strerror(errno));
    return 0;
  }
  
  // load the superblock data
  readSuper();
  showSuper();
  
  // go to first data block to see if the v6-filename already exist
  char existFilename[14];
  unsigned int countInode = 0;
  
  lseek(fileDescriptor, (2 + superBlock.isize)*BLOCK_SIZE + 2, 0);
  read(fileDescriptor, &existFilename, 14);
  
  while(existFilename[0] != '\0'){
    printf("%s\n",existFilename);
    if (strcmp(existFilename, vFile) == 0){
      printf("\n filename of the v6-File is already exist!");
      return 0;
    }
    lseek(fileDescriptor, 2, SEEK_CUR);
    read(fileDescriptor, &existFilename, 14);
    ++countInode;
  }

  // open the external file and check if exist
  int extFileDescriptor;
  if((extFileDescriptor = open(extFilePath,O_RDONLY))== -1){
    printf("\n open() failed with the following error [%s]\n",strerror(errno));
    return 0;
  }

  // initialize the v6-file inode
  unsigned int vFile_data_block = getFreeBlock(); // should look up superBlock.free
  if (vFile_data_block == -1){
    printf("\n The v6 file system is full!");
    return 0;
  }
  //printf("free block number: %i\n",vFile_data_block);

  // vFile_data_block = 2 + superBlock.isize + countInode - 1;
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
      inode.addr[num_addr] = vFile_data_block;
      inode.size += BLOCK_SIZE;
      ++num_addr;
      curr_block_bytes = 0;
    }

    write(fileDescriptor, ch, 1);
    ++curr_block_bytes;
  }
  
  // update the directory in root data block
  dir_type vFile_dir;
  vFile_dir.inode = countInode;
  for (i = 0; i < 14; i++){
    vFile_dir.filename[i] = vFile[i];
  }
  
  lseek(fileDescriptor, (2 + superBlock.isize)*BLOCK_SIZE+countInode*16, 0);
  write(fileDescriptor, &vFile_dir, 16);
  
  // store new v6-file inode
  lseek(fileDescriptor, 2 * BLOCK_SIZE + (countInode-1)*INODE_SIZE, 0);
  write(fileDescriptor, &inode, INODE_SIZE);

  // change the i-node entry of the root (specifically only its addr)
  /*lseek(fileDescriptor, 2 * BLOCK_SIZE + 12 + 4*countInode, 0);
  write(fileDescriptor, &vFile_data_block, 4);*/
    
  return 1;
}

void showSuper(){
  printf("superBlock.isize: %hu\n", superBlock.isize);
  printf("superBlock.fsize: %hu\n", superBlock.fsize);
  printf("superBlock.nfree: %hu\n", superBlock.nfree);
  printf("superBlock.free[%hu]: %i\n", superBlock.nfree-1, superBlock.free[superBlock.nfree-1]);
  printf("superBlock.ninode: %hu\n", superBlock.ninode);
  printf("superBlock.inode: %hu\n", superBlock.inode[I_SIZE-1]);
  printf("superBlock.flock: %c\n", superBlock.flock);
  printf("superBlock.ilock: %c\n", superBlock.ilock);
  printf("superBlock.fmod: %hu\n", superBlock.fmod);
  printf("superBlock.time[1]: %hu\n", superBlock.time[1]);
}


void readSuper(){
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
  
  //read(fileDescriptor, &ch, 2);
  //superBlock.time[1] = (unsigned short)*ch;
}

void readInode(unsigned int count){
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
  --superBlock.nfree;
  int acquired_free_data_block = superBlock.free[superBlock.nfree];
  if (superBlock.nfree == 0){
    printf("check if nfree is empty!");
    if (superBlock.free[0] == 0){
      printf("\n The v6 file system is full! Write failed!");
      return -1;
    }
    
    lseek(fileDescriptor, acquired_free_data_block*BLOCK_SIZE+6, SEEK_SET);
    read(fileDescriptor, &superBlock.nfree, 2);
    read(fileDescriptor, &superBlock.free, 4*FREE_SIZE);

    return superBlock.free[--superBlock.nfree];
    
    // free the block originally contain indexes of free blocks
    // add_block_to_free_list(index_data_block, buffer) //don't implement this time??
    
  }

  return acquired_free_data_block;
}

unsigned short getFreeInode(){
  // only use it when the superBlock.isize*16 > 200(superBlock.ninode)

  return 0;
}
int copyOut(char *parameters) {
  char *extFilePath, *vFile;

  parameters = strtok(NULL, " ");
  extFilePath = parameters;
  parameters = strtok(NULL, " ");
  vFile = parameters;
  
  if((fileDescriptor = open("/disk",O_RDWR,0700))== -1){
    printf("\n open() failed with the following error [%s]\n",strerror(errno));
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
    printf("%s\n",existFilename);
    if (strcmp(existFilename, vFile) == 0){
      printf("\n filename of the v6-File is found!");
      break;
    }
    lseek(fileDescriptor, 2, SEEK_CUR);
    read(fileDescriptor, existFilename, 14);
    ++countInode;
  }

  // create the new external file
  int extFileDescriptor;
  if((extFileDescriptor = open(extFilePath,O_RDWR,0700))== -1){
    printf("\n open() failed with the following error [%s]\n",strerror(errno));
    return 0;
  }
  
  // read the inode entry of the v6-file and get the addr array
  readInode(countInode);

  // copy the contentinto the new external file
  char ch[1];
  int curr_block_bytes = 0;
  short num_addr = 1;
  lseek(fileDescriptor, inode.addr[0]*BLOCK_SIZE, SEEK_SET);
  while (read(fileDescriptor, ch, 1)) { 
    write(extFileDescriptor, ch, 1);
    ++curr_block_bytes;
    
    if (curr_block_bytes == BLOCK_SIZE){
      // if the current data block is copied completely
      ++num_addr;
      lseek(fileDescriptor, inode.addr[num_addr]*BLOCK_SIZE, SEEK_SET);
      curr_block_bytes = 0;
    }
  }
}