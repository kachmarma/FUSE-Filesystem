#ifndef NUFS_STORAGE_H
#define NUFS_STORAGE_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "bmap.h"

// flags for file Data
enum myFlag {
    READ_ONLY = 0,
    WRITE_ONLY = 1,
    EXECUTE_ONLY = 2,
    READ_WRITE = 3,
    READ_EXECUTE = 4,
    WRITE_EXECUTE = 5,
    READ_WRITE_EXECUTE = 6,
    NO_PERMISION = -1
} myFlag;

// important file/directory Data
typedef struct file_data {
    const char* path;
    int         mode;
    int         uid;
    size_t      dataSize;
    int         createTime;
    int         modTime;
    int         ref_count;
    int         blockCount;
    enum myFlag flag;
} file_data;

// points to 1024 data Blocks
typedef struct indiBlock {
    size_t dBlocks[1024];
} indiBlock;

// represents an iNode containing important file data and pointers to data Blocks
typedef struct myNode {
    file_data data;                  // contains important file data
    size_t dataBlockOffsets[12];     // assigned directly to data blocks
    size_t indiBlock;  // pointer to indirect blocks
} myNode;

// contains important file system pointers
typedef struct superBlock {
    size_t myNodeMap_offset; // bit map of free and used data block
    size_t dataBlockMap_offset; // bit map of free and used inodes
    size_t rootMyNode_offset; // the root inode offset in bytes
    size_t myNodeTable_offset; // the table of 256 myNodes
} superBlock;

void storage_init(const char* path);
int         get_stat(const char* path, struct stat* st);
const char* get_data(const char* path);

#endif
