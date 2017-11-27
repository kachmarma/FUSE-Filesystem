#ifndef NUFS_STORAGE_H
#define NUFS_STORAGE_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "bmap.h"

// flags for file Data
typedef struct myFlag {

} myFlag;

// important file/directory Data
typedef struct file_data {
    const char* path;
    int         mode;
    int         uid;
    int         dataSize;
    int         accessTime;
    int         createTime;
    int         modTime;
    int         groupID;
    int         ref_count;
    int         blockCount;
    myFlag      flag;
} file_data;

// points to 1024 data Blocks
typedef struct indiBlock {
    void* dBlocks[1024];
} indiBlock;

// represents an iNode containing important file data and pointers to data Blocks
typedef struct myNode {
    file_data data;        // contains important file data
    void* dBlocks[12];     // assigned directly to data blocks
    indiBlock* indiBlock;  // pointer to indirect blocks
} myNode;

// contains important file system pointers
typedef struct superBlock {
    bmap* myNodesMap; // bit map of free and used data blocks
    bmap* dataBlockMap; // bit map of free and used inodes
    myNode* rootMyNode; // the root inode
    void* myNodeTable; // the table of 256 myNodes
} superBlock;

void storage_init(const char* path);
int         get_stat(const char* path, struct stat* st);
const char* get_data(const char* path);

#endif
