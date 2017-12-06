#define _GNU_SOURCE
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h> // TODO we might need this
#include <stdio.h>
#include <assert.h>
#include <time.h>

#include "storage.h"
#include "util.h"
#include "bmap.h"

const int NUFS_SIZE  = 1024 * 1024; // 1MB
const int PAGE_COUNT = 256;
const int PAGE_SIZE = 4096;

static int   pages_fd   = -1;
static void* pages_base =  0;

superBlock* sb;

int
initPathToNode()
{
	pathToNode* pathToNode = (struct pathToNode*) pages_get_page(sb->pathToNode_pnum);
	for (int i = 0; i < 256; i++)
	{
		pathToNode->inodeNumber[i] = -1;
	}
	return 0;
}

/**
 * Check if two strings are equal.
 * @param aa first string.
 * @param bb second string.
 * @return If the two strings are equal.
 */
int get_inode_index(const char* path)
{	
    printf("Get inode index: %s\n", path);
	pathToNode* pathToNode = (struct pathToNode*) pages_get_page(sb->pathToNode_pnum);
    for (int i = 0; i < 256; i++)
    {
        if (streq(pathToNode->paths[i], path))
        {
            return i;
        }
    }
    
    perror("No inode found for given path.\n");
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
    return (void*) (pages_base + 4096 * pnum);
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

inode* create_inode_literal(const char* path, int mode, int uid, size_t dataSize, enum myFlag flag, int* outIndex)
{
	  // SCAN inode bitmap
    printf("Creating inode with path {%s}, mode {%d}, uid: {%d}, size {%ld} and flag {%d}\n",
    path, mode, uid, dataSize, flag);

    bmap* nodeMap = (struct bmap*) pages_get_page(sb->inodeMap_pnum);
    int index = setFirstAvailable(nodeMap);
	printf("Bitmap page: %p\n", pages_get_page(sb->inodeMap_pnum));
	printBitMap(nodeMap);
    printf("Index = %d\n", index);
	if (index < 0) {
        perror("No free inodes");
    }
	*outIndex = index;

	inode* inode = pages_get_node(index, sb->inodeTable_pnum);
	
    // fill in inode data
    inode->fileData = (file_data) { path, mode, uid, dataSize, 0, 0, 1, (int) ceil(dataSize / 4096), flag };
	clock_gettime(CLOCK_REALTIME, inode->fileData.createTime);
	clock_gettime(CLOCK_REALTIME, inode->fileData.modTime);

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

	printBitMap((struct bmap*) pages_get_page(sb->dataBlockMap_pnum));

    return inode;
}

int
logToFile(const char* path, int index)
{
	pathToNode* pathToNode = (struct pathToNode*) pages_get_page(sb->pathToNode_pnum);
	for (int i = 0; i < 256; i++)
    {
        if (pathToNode->inodeNumber[i] == -1)
        {
            pathToNode->inodeNumber[i] = index;
			int charindex = 64;
			if (strlen(path) < 64)
			{
				charindex = strlen(path);
			}
			for (int z = 0; z < charindex; z++)
			{
				pathToNode->paths[i][z] = path[z];
			}       
			break;
        }
        if (i == 255) {
            perror("Unable to find free location for file\n");
            return -1;
        }
    }
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
	int index = 0;
    inode* inode = create_inode_literal(path, mode, uid, dataSize, flag, &index);

	// assign path to inode number	
	logToFile(path, index);
 	
    return 0;
}

void storage_read_dir(const char* path, void *buf, struct stat* st, fuse_fill_dir_t filler)
{
	pathToNode* pathToNode = (struct pathToNode*) pages_get_page(sb->pathToNode_pnum);
	int path_length = strlen(path);
	for (int i = 1; i < 256; i++)
	{
		if (pathToNode->inodeNumber[i] != -1)
		{
			char* str = pathToNode->paths[i];
			if (!(strlen(str) < path_length))
			{
			int valid = 1;
			
			int z = 0;
			for (z; z < path_length; z++)
			{
				if(str[z] != str[z])
				{
					valid = 0;
					break;
				}
			}

			int q = z + 1;
			for (q; q < strlen(str); q++)
			{
				if (str[q] == '/' && (q != (strlen(str) - 1)))
				{
					valid = 0;
					break;
				}
			}
			
			if (valid == 1)
			{
			 str = str + path_length;
			 str[strlen(str)] = '\0';
			 printf("Valid path: %s\n", str);
			 filler(buf, str, NULL, 0);	
			}
			
			}
		}
	}
}

char*
getDataFromNode(inode* inode, off_t offset)
{
	// TODO use offset
	int buffer_size = inode->fileData.dataSize;
	char* buff[inode->fileData.blockCount * 4096];

	// if using direct pointers
	if (inode->fileData.blockCount <= 12)
	{
		for (int ii = 0; ii < inode->fileData.blockCount; ii++)
		{
//			memcpy(buff + ii * 4096, sb->??? inode->dataBlockNumber[ii], 4096);
		}
	}
	// if using indirect
	else
	{
		int left = inode->fileData.blockCount;
        int outterBlocks = (int) (ceil(left / 12));

        for (int i = 0; i < outterBlocks; i++) {
            for (int z = 0; z < min(12, left); z++) {
              //  memcpy(buff + (i * 12 + z) * 4096, inode->indiBlock[i][z], 4096);
                left--;
			}
		}
	}
	return 0;
	//return buff;
}

int
storage_set_time(const char* path, const struct timespec ts[2])
{
	inode* inode = retrieve_inode(path);
	inode->fileData.modTime[0] = ts[0];
	inode->fileData.modTime[1] = ts[1];
	return 0;
}

int
writeToIndirectPointerBlock(inode* node, int pblock_num, int val)
{
	int retVal = node->indiBlock[pblock_num / 12][(pblock_num % 12)];
	node->indiBlock[pblock_num / 12][(pblock_num % 12)] = val;
	return retVal;
}

int
storage_shrink(inode* inode, size_t inode_size, off_t size)
{	
	int pages_needed = (int) ceil(size / PAGE_SIZE) - inode->fileData.blockCount;	

	if (inode->fileData.blockCount < 12)
	{
		// set less direct blocks
		for (int i = pages_needed; i < inode->fileData.blockCount; i++)
		{
			clearBit((struct bmap*) pages_get_page(sb->dataBlockMap_pnum), inode->dataBlockNumber[i]);
			inode->dataBlockNumber[i] = 0;
		}
	}
	else {
		if (pages_needed < 12) // going from indirect to direct, must copy data over
		{
			for (int i = 0; i < pages_needed; i++)
			{
				inode->dataBlockNumber[i] = inode->indiBlock[0][i];
			}

			int z = 0;
			if (pages_needed != 0)
			{
				z = 1;
			}
			for (z; z < 12; z++)
			{
				for (int x = 0; x < 12; x++)
				{
					if (inode->indiBlock[z][x] != 0)
					{
						clearBit((struct bmap*) pages_get_page(sb->dataBlockMap_pnum), inode->indiBlock[z][x]);
						inode->indiBlock[z][x] = 0;
					}
				}
			}
		}
		else // going from indirect to less indirect
		{
			for (int i = pages_needed; i < inode->fileData.blockCount; i++)
			{
				int bitmapNumber = writeToIndirectPointerBlock(inode, i, 0);
				clearBit((struct bmap*) pages_get_page(sb->dataBlockMap_pnum), bitmapNumber);
			}
		}
	}

	// change size in fileData to requested size
	inode->fileData.dataSize = size;
	// update BlockCount
	inode->fileData.blockCount = pages_needed;
	return 0;
}

int
storage_expand(inode* inode, size_t inode_size, off_t size) {
        int pages_needed = (int)ceil(size / PAGE_SIZE) - inode->fileData.blockCount;
		if (size >= 1024 * 1000)
		{
			perror("Trying to expand to bigger size than filesystem allows\n");
			return -1;
		}
        // add data blocks or redirect to indirect
        if (size > (PAGE_SIZE * 12))
        {
            // already using indirect -> assigning more indirect
            if (inode->fileData.blockCount > 12)
            {
                // just assign more indirect
                for (int ii = inode->fileData.blockCount; ii < pages_needed; ii++)
                {
                    int firstAvailable = setFirstAvailable(pages_get_page(sb->dataBlockMap_pnum));
                    if (firstAvailable < 1)
                    {
                        perror("No more free data blocks for file expand with truncate\n");
                        return -1;
                    }   
                    writeToIndirectPointerBlock(inode, ii, firstAvailable);
                }
            }
			else // not using indirect yet -> copying direct to indirect and assigning more indirect
			{
            // copy over data from direct block to first direct block in indirect block and clear direct block
            for (int ii = 0; ii < inode->fileData.blockCount; ii++)
            {
                inode->indiBlock[0][ii] = inode->dataBlockNumber[ii];
                inode->dataBlockNumber[ii] = 0;
            }
            // allocate extra space
            for (int ii = inode->fileData.blockCount; ii < pages_needed; ii++)
            {
                int firstAvailable = setFirstAvailable(pages_get_page(sb->dataBlockMap_pnum));
                if (firstAvailable < 1)
                {
                    perror("No more free data blocks for file expand with truncate\n");
                    return -1;
                }
                writeToIndirectPointerBlock(inode, ii, firstAvailable);
            }
        }
    } else
    {
        // use direct and assign more direct blocks
        for (int ii = inode->fileData.blockCount; ii < pages_needed + inode->fileData.blockCount; ii++)
        {
            int firstAvailable = setFirstAvailable(pages_get_page(sb->dataBlockMap_pnum));
            if (firstAvailable < 1)
            {
                perror("No more free data blocks for file expand with truncate\n");
                return -1;
            }
            inode->dataBlockNumber[ii] = firstAvailable;
        }
    }
	// change size in fileData to requested size
	inode->fileData.dataSize = size;
	// update BlockCount
	inode->fileData.blockCount = pages_needed;
	return 0;
}

void
storage_truncate(const char* path, off_t size)
{
	// get inode
	inode* inode = retrieve_inode(path);
	// get inode size
	size_t inode_size = inode->fileData.dataSize;
	
	if (inode_size == size)
	{
		return;
	}

	// condition for expand
	if (inode_size < size)
	{	
	
	} else // condition for shrink
	{
		 storage_shrink(inode, inode_size, size);
	}


	
}


void
printAll()
{
//    pathToNode* pathToNode = (struct pathToNode*) pages_get_page(sb->pathToNode_pnum);
//    for (int i = 0; i < 256; i++)
//    {
//        if (pathToNode->nodeNumber[i] != -1)
//        {
//            printf("%s\n", pathToNode->fileName[i]);
//            print_node(pages_get_node(pathToNode->nodeNumber[i], sb->inodeTable_pnum));
//        }
//    }
	
	printBitMap((struct bmap*) pages_get_page(sb->inodeMap_pnum));
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
    printf("%s\n", path);
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
    sb = superBlock_init(pages_base);
    sb->inodeMap_pnum = 1;
    sb->dataBlockMap_pnum = 2;
    sb->pathToNode_pnum = 3;
	sb->inodeTable_pnum = 8;
    bmap inodeMap = createBitMap(pages_get_page(sb->inodeMap_pnum));
    bmap myDataMap = createBitMap(pages_get_page(sb->dataBlockMap_pnum));
    // create the root inode
	initPathToNode();
	
    if (createInode("/", 0040000, 1, 0, READ_WRITE_EXECUTE) < 0) { // TODO log root if necessary
        perror("Failed to create root inode");
    }
	
	pathToNode* pathToNode = (struct pathToNode*) pages_get_page(sb->pathToNode_pnum);
//	logToFile("/.", 0);
//	logToFile("/..", 0); 
}

/**
 * Initialize the super block.
 * @param pages_base The pointer to the beginning of the file system.
 * @return
 */
superBlock*
superBlock_init(void* pages_base)
{
    superBlock *sb = (superBlock *)pages_base;
    return sb;
}

/**
 * idk, TODO what does this do
 * @param path ??
 * @return ??
 */
static file_data*
get_file_data(const char* path) {
//    for (int ii = 0; 1; ++ii) {
//        file_data row = file_table[ii];

//        if (file_table[ii].path == 0) {
//            break;
//        }

//        if (streq(path, file_table[ii].path)) {
//            return &(file_table[ii]);
//        }
//    }

   return 0;
}


/**
 * Gets the stats for the given path.
 * @param path the given path
 * @param st the struct to fill with stats
 * @return on success
 */
int
get_stat(const char* path, struct stat* st)
{
    int inode_index = get_inode_index(path);

    if (inode_index < 0) {
        return -1;
    }

    inode* node = retrieve_inode(path);

    file_data dat = node->fileData;

    printf("Getting file data for path {%s} with inode num %d\n", path, inode_index);

    memset(st, 0, sizeof(struct stat));
    st->st_dev = 1;
    st->st_ino = inode_index;
    st->st_mode = dat.mode;
    st->st_nlink = dat.ref_count;
    st->st_uid  = dat.uid;
    st->st_gid = dat.uid;
    st->st_rdev = 1;
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
const char*
get_data(const char* path)
{
   file_data* dat = get_file_data(path);
   if (!dat) {
       return 0;
   }
   return NULL; // TODO fix this
}
