#include <stdio.h>
#include <string.h>

#include "storage.h"

size_t PAGE_SIZE = 4096;
superBlock* sb;

void
storage_init(const char* path)
{
    printf("TODO: Store file system data in: %s\n", path);
    sb = mmap(0, PAGE_SIZE * 256, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); // split up into 4k chunks
    sb->dataBlockMap_offset = PAGE_SIZE * 1; // 4096
    sb->myNodeMap_offset = PAGE_SIZE * 2; // 8192
    sb->myNodeTable_offset = PAGE_SIZE * 3; // 12288
    (size_t) (sb + sb->dataBlockMap_offset) = createBitMap();
    (size_t) (sb + sb->dataBlockMap_offset) = createBitMap();
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