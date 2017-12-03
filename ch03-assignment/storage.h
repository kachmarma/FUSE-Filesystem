#ifndef STORAGE_H
#define STORAGE_H

#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "bmap.h"
#include <math.h>
#include <time.h>

typedef struct pathToNode {
    const char* fileName[256];
    int nodeNumber[256];
} pathToNode;

void
initPathToNode();

void
printAll();

inode* retrieve_inode(const char* path);

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
    unsigned long         createTime;
    unsigned long         modTime;
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
    size_t dataBlockNumber[12];     // assigned directly to data blocks
    size_t indiBlock[12][12];  // pointer to indirect blocks
} inode;

// contains important file system pointers
// by default, root inode is the first inode
typedef struct superBlock {
    size_t inodeMap_pnum; // page number of bit map of free and used data block
    size_t dataBlockMap_pnum; // page number of bit map of free and used inodes
    size_t inodeTable_pnum; // page numeber of the table of 256 inodes
    size_t pathToNode_pnum; // page number of the mapping of paths to nodes
} superBlock;

void storage_init(void* pages_base);
int         get_stat(const char* path, struct stat* st);
const char* get_data(const char* path);

superBlock superBlock_init(void* position);

void   pages_init(const char* path);
void   pages_free();
void*  pages_get_page(int pnum);
inode* pages_get_node(int node_id, int pnum);
int    pages_find_empty();
void   print_node(inode* node);

#endif
