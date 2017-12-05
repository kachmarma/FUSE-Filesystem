#ifndef STORAGE_H
#define STORAGE_H

/**
 * Flags for file data.
 */
enum myFlag {
    READ_ONLY = 0,
    WRITE_ONLY = 1,
    EXECUTE_ONLY = 2,
    READ_WRITE = 3,
    READ_EXECUTE = 4,
    WRITE_EXECUTE = 5,
    READ_WRITE_EXECUTE = 6,
    NO_PERMISSION = -1
} myFlag;

// important file/directory Data
typedef struct file_data {
    const char*           path;
    int                   mode;
    int                   uid;
    size_t                dataSize;
    struct timespec       createTime[2];
    struct timespec       modTime[2];
    int                   ref_count;
    int                   blockCount;
    enum myFlag           flag;
} file_data;

// points to 1024 data Blocks
typedef struct indiBlock {
    size_t dBlocks[1024];
} indiBlock;

// represents an inode containing important file data and pointers to data Blocks
typedef struct inode {
    file_data fileData;                  // contains important file data
    int dataBlockNumber[12];     // assigned directly to data blocks
    int indiBlock[12][12];  // pointer to indirect blocks
} inode;

// contains important file system pointers
// by default, root inode is the first inode
typedef struct superBlock {
    int inodeMap_pnum; // page number of bit map of free and used data block
    int dataBlockMap_pnum; // page number of bit map of free and used inodes
    int inodeTable_pnum; // page numeber of the table of 256 inodes
    int pathToNode_pnum; // page number of the mapping of paths to nodes
} superBlock;

int createInode(const char* path, int mode, int uid, size_t dataSize, enum myFlag flag);

void
initPathToNode();

inode*
create_inode_literal(const char* path, int mode, int uid, size_t dataSize, enum myFlag flag, int* outIndex);

int
logToFile(const char* path, int index);

void
printAll();



inode* retrieve_inode(const char* path);

void storage_init(void* pages_base);
int         get_stat(const char* path, struct stat* st);
const char* get_data(const char* path);

superBlock* superBlock_init(void* position);

int storage_set_time(const char* path, const struct timespec ts[2]);

void   pages_init(const char* path);
void   pages_free();
void*  pages_get_page(int pnum);
inode* pages_get_node(int node_id, int pnum);
int    pages_find_empty();
void   print_node(inode* node);

#endif
