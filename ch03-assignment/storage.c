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

#include "storage.h"

const int NUFS_SIZE  = 1024 * 1024; // 1MB
const int PAGE_COUNT = 256;
const int PAGE_SIZE = 4096;

static int   pages_fd   = -1;
static void* pages_base =  0;

superBlock* sb;

/**
 * Free the allocated pages.
 */
void
pages_free()
{
    int rv = munmap(pages_base, NUFS_SIZE);
    assert(rv == 0);
}

/**
 * Get a pointer to page pnum from the storage.
 * @param pnum Which page to retrieve.
 * @return Pointer to the page.
 */
void*
pages_get_page(int pnum)
{
    return pages_base + 4096 * pnum;
}

/**
 * Get a pointer to the inode at the given page.
 * @param node_id The id of the node.
 * @param pnum The page the node is in.
 * @return A pointer to the node.
 */
inode*
pages_get_node(int node_id, int pnum)
{
    inode* idx = (inode*) pages_get_page(pnum);
    return &(idx[node_id]);
}

/**
 * Finds an empty page for data.
 * @return The index of the empty page. -1 if no empty pages are available.
 */
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
/**
 * Create a new inode for a file with the given arguments.
 * @param path The path of the file.
 * @param mode The mode of the file (directory or file)
 * @param uid The id of the user.
 * @param dataSize The size of the file.
 * @param flag The flags for the file.
 * @return idk, TODO The index of the newly created inode or if the operation was successful?
 */
int
createInode(const char* path, int mode, int uid, size_t dataSize, enum myFlag flag)
{
    // SCAN inode bitmap
    bmap* nodeMap = (bmap*) pages_get_page(sb->inodeTable_pnum);
    int index = setFirstAvailable(nodeMap);
    if (index < 0) {
        perror("No free inodes");
    }
    inode* inode = pages_get_node(index, sb->inodeTable_pnum);
    // fill in inode data
    unsigned long nowTime = (unsigned long) time(NULL);
    inode->fileData = (file_data) { path, mode, uid, dataSize, nowTime, nowTime, 1, (int) ceil(dataSize / 4096), flag };


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
            inode->dataBlockNumber[i] = mapEntry;
        }

    } else {
        // use indirect pointers
        int left = pages_needed;
        int outterBlocks = (int) (ceil(pages_needed / 12));
        for (int i = 0; i < outterBlocks; i++) {
            for (int z = 0; z < min(12, left); z++) {
                int mapEntry = setFirstAvailable(pages_get_page(sb->dataBlockMap_pnum));
                if (mapEntry < 0) {
                    perror("No free data blocks.");
                }
                inode->indiBlock[i][z] = mapEntry;
                left--;
            }
        }
    }
}

// TODO
void
print_node(inode* node)
{
    if (node) {
        printf("node{refs: %d, mode: %04o, size: %d, xtra: %d}\n",
               node->ref_count, node->mode, node->dataSize, node->blockCount); 
    }
    else {
        printf("node{null}\n");
    }
}

/**
 * Creates the pages into a file with the named in path.
 * @param path The path and file name of the file.
 */
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

/**
 * Initialize the storage of the file system.
 * @param pages_base The pointer to the beginning of the file system.
 */
void
storage_init(void* pages_base)
{
    *sb = superBlock_init(pages_base);
    sb->inodeMap_pnum = 1;
    sb->dataBlockMap_pnum = 2;
    sb->inodeTable_pnum = 3;
    bmap inodeMap = createBitMap(pages_get_page(sb->inodeMap_pnum));
    bmap myDataMap = createBitMap(pages_get_page(sb->dataBlockMap_pnum));
    // create the root inode
    if (createinode() < 0) {
        perror("Failed to create root inode");
    }
}

/**
 * Initialize the super block.
 * @param pages_base The pointer to the beginning of the file system.
 * @return
 */
superBlock
superBlock_init(void* pages_base)
{
    superBlock *sb = (superBlock *)pages_base;
    return *sb;
}

/**
 * idk, TODO what does this do
 * @param aa ??
 * @param bb ??
 * @return ??
 */
static int
streq(const char* aa, const char* bb)
{
    return strcmp(aa, bb) == 0;
}


/**
 * idk, TODO what does this do
 * @param path ??
 * @return ??
 */
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


/**
 * idk, TODO what does this do
 * @param path ??
 * @param st ??
 * @return ??
 */
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

/**
 * idk, TODO what does this do
 * @param path ??
 * @return ??
 */
const char*
get_data(const char* path)
{
    file_data* dat = get_file_data(path);
    if (!dat) {
        return 0;
    }
    return dat->data;
}