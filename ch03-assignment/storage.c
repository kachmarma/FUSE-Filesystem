#define _GNU_SOURCE
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h> // TODO we might need this
#include <stdio.h>

#include "storage.h"
#include "util.h"

const int NUFS_SIZE  = 1024 * 1024; // 1MB
const int PAGE_COUNT = 256;
const int PAGE_SIZE = 4096;

static int   pages_fd   = -1;
static void* pages_base =  0;

superBlock* sb;

void
initPathToNode()
{
    pathToNode* pathToNode = (pathToNode*) pages_get_page(sb->pathToNode_pnum);
    for (int i = 0; i < 256; i++) {
        pathToNode->fileName[i] = "";
        pathToNode->nodeNumber[i] = -1;
    }
}

/**
 * Check if two strings are equal.
 * @param aa first string.
 * @param bb second string.
 * @return If the two strings are equal.
 */
int get_inode_index(const char* path)
{
    pathToNode* pathToNode = (pathToNode*) pages_get_page(sb->pathToNode_pnum);
    for (int i = 0; i < 256; i++)
    {
        if (streq(pathToNode->fileName[i], path))
        {
            return i;
        }
    }
    
    perror(sprintf("No inode found for given path: %s\n", path));
    return -1;
}

inode* retrieve_inode(const char* path)
{
    int inode_index = get_inode_index(path);
    return pages_get_node(inode_index, sb->inodeTable_pnum);
}


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

    // assign path to inode number
    pathToNode* pathToNode = (pathToNode*) pages_get_page(sb->pathToNode_pnum);
    for (int i = 0; i < 256; i++)
    {
        if (pathToNode->nodeNumber[i] == -1)
        {
            pathToNode->nodeNumber[i] = index;
            pathToNode->fileName[i] = strdup(path);
        }

        if (i == 255) {
            perror("Unable to find free location for file");
            break;
        }

    }

    // fill in inode data
    unsigned long nowTime = (unsigned long) time(NULL);
    inode->fileData = (file_data) { path, mode, uid, dataSize, nowTime, nowTime, 1, (int) ceil(dataSize / 4096), flag };


    // figure out size things
    int pages_needed = (int)ceil(dataSize / PAGE_SIZE);
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

    return 0;
}

void
printAll()
{
    pathToNode* pathToNode = (pathToNode*) pages_get_page(sb->pathToNode_pnum);
    for (int i = 0; i < 256; i++)
    {
        if (pathToNode->nodeNumber[i] != -1)
        {
            printf("%s", pathToNode->fileName[i]);
            print_node(pages_get_node(pathToNode->nodeNumber[i], sb->inodeTable_pnum));
        }
    }
}

// TODO
void
print_node(inode* node)
{
    if (node) {
        printf("node{refs: %d, mode: %04o, size: %ld, xtra: %d}\n",
               node->fileData.ref_count, node->fileData.mode, node->fileData.dataSize, node->fileData.blockCount);
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
    initPathToNode();
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
    sb->pathToNode_pnum = 4;
    bmap inodeMap = createBitMap(pages_get_page(sb->inodeMap_pnum));
    bmap myDataMap = createBitMap(pages_get_page(sb->dataBlockMap_pnum));
    // create the root inode
    if (createInode("/", 0, 1, 0, 0) < 0) { // TODO fix this
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
 * @param path ??
 * @return ??
 */
//static file_data*
//get_file_data(const char* path) {
//    for (int ii = 0; 1; ++ii) {
//        file_data row = file_table[ii];
//
//        if (file_table[ii].path == 0) {
//            break;
//        }
//
//        if (streq(path, file_table[ii].path)) {
//            return &(file_table[ii]);
//        }
//    }
//
//    return 0;
//}


/**
 * Gets the stats for the given path.
 * @param path the given path
 * @param st the struct to fill with stats
 * @return on success
 */
int
get_stat(const char* path, struct stat* st)
{
    
    inode* node = retrieve_inode(path);
    if (!node) {
        return -1;
    }

    file_data dat = node->fileData;

    memset(st, 0, sizeof(struct stat));
    st->st_dev = 1;
    st->st_ino = get_inode_index(path);
    st->st_mode = dat.mode;
    st->st_nlink = -1; // TODO update this.
    st->st_uid  = dat.uid;
    st->st_gid = dat.uid;
    st->st_rdev = -1;
    st->st_size = dat.dataSize;
    st->st_blksize = PAGE_SIZE;
    st->st_blocks = dat.blockCount;
    return 0;
}

/**
 * idk, TODO what does this do
 * @param path ??
 * @return ??
 */
//const char*
//get_data(const char* path)
//{
//    file_data* dat = get_file_data(path);
//    if (!dat) {
//        return 0;
//    }
//    return dat->data;
//}