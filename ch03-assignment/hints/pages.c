
#define _GNU_SOURCE
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "pages.h"
#include "slist.h"
#include "util.h"

const int NUFS_SIZE  = 1024 * 1024; // 1MB
const int PAGE_COUNT = 256;
const int PAGE_SIZE = 4096;

static int   pages_fd   = -1;
static void* pages_base =  0;

superBlock* sb;

void
pages_free()
{
    int rv = munmap(pages_base, NUFS_SIZE);
    assert(rv == 0);
}

void*
pages_get_page(int pnum)
{
    return pages_base + 4096 * pnum;
}

myNode*
pages_get_node(int node_id, int pnum)
{
    myNode* idx = (myNode*) pages_get_page(pnum);
    return &(idx[node_id]);
}

// TODO 
int
pages_find_empty()
{
    int pnum = -1;
    for (int ii = 2; ii < PAGE_COUNT; ++ii) {
        if (0) { // if page is empty
            pnum = ii;
            break;
        }
    }
    return pnum;
}

// mode:
//  0 = directory
//  1 = file
int
createMyNode(const char* path, int mode, int uid, size_t dataSize, enum myFlag flag)
{
    // SCAN inode bitmap
    bmap* nodeMap = (bmap*) pages_get_page(sb->myNodeTable_pnum);
    int index = setFirstAvailable(nodeMap);
    if (index < 0) {
        perror("No free myNodes");
    }
    myNode* myNode = pages_get_node(index, sb->myNodeTable_pnum);
    // fill in myNode data
    unsigned long nowTime = (unsigned long) time(NULL);
    myNode->fileData = (file_data) { path, mode, uid, dataSize, nowTime, nowTime, 1, (int) ceil(dataSize / 4096), flag };


    // figure out size things
    int pages_needed = ceil(dataSize / PAGE_SIZE);
    if (dataSize <= PAGE_SIZE * 12) {
        // use direct pointer(s)

        //TODO: getBitmap and map pages
        for (int i = 0; i < pages_needed; i++) {
            int mapEntry = setFirstAvailable(pages_get_page(sb->dataBlockMap_pnum));
            if (mapEntry < 0) {
                perror("No free data blocks.");
            }
            myNode->dataBlockNumber[i] = mapEntry;
        }
    
    } else {
        // use indirect pointers
        int outterBlocks = (int) (ceil(pages_needed / 12));
        for (int i = 0; i < outterBlocks; i++) {
             for (int z = 0; z < 12; z++) {
                int mapEntry = setFirstAvailable(pages_get_page(sb->dataBlockMap_pnum));
                if (mapEntry < 0) {
                    perror("No free data blocks.");
                }
                myNode->dataBlockNumber[z] = mapEntry;
            }
        }
    }
}

// TODO
void
print_node(myNode* node)
{
    if (node) {
        printf("node{refs: %d, mode: %04o, size: %d, xtra: %d}\n",
               node->refs, node->mode, node->size, node->xtra);
    }
    else {
        printf("node{null}\n");
    }
}

void
pages_init(const char* path)
{
    pages_fd = open(path, O_CREAT | O_RDWR, 0644);
    assert(pages_fd != -1);

    int rv = ftruncate(pages_fd, NUFS_SIZE);
    assert(rv == 0);

    pages_base = mmap(0, NUFS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, pages_fd, 0);
    assert(pages_base != MAP_FAILED);
    // stages init
    storage_init(pages_base);
}

void
storage_init(void* pages_base)
{
    *sb = superBlock_init(pages_base);
    sb->myNodeMap_pnum = 1;
    sb->dataBlockMap_pnum = 2;
    sb->myNodeTable_pnum = 3;
    bmap myNodeMap = createBitMap(pages_get_page(sb->myNodeMap_pnum));
    bmap myDataMap = createBitMap(pages_get_page(sb->dataBlockMap_pnum));
    // create the root inode
    if (createMyNode() < 0) {
        perror("Failed to create root myNode");
    }
}

superBlock
superBlock_init(void* pages_base)
{
    superBlock *sb = (superBlock *)pages_base;
    return *sb;
}

static int
streq(const char* aa, const char* bb)
{
    return strcmp(aa, bb) == 0;
}

static file_data*
get_file_data(const char* path) {
    for (int ii = 0; 1; ++ii) {
        file_data row = file_table[ii];

        if (file_table[ii].path == 0) {
            break;
        }

        if (streq(path, file_table[ii].path)) {
            return &(file_table[ii]);
        }
    }

    return 0;
}

int
get_stat(const char* path, struct stat* st)
{
    file_data* dat = get_file_data(path);
    if (!dat) {
        return -1;
    }

    memset(st, 0, sizeof(struct stat));
    st->st_uid  = getuid();
    st->st_mode = dat->mode;
    if (dat->data) {
        st->st_size = strlen(dat->data);
    }
    else {
        st->st_size = 0;
    }
    return 0;
}

const char*
get_data(const char* path)
{
    file_data* dat = get_file_data(path);
    if (!dat) {
        return 0;
    }

    return dat->data;
}