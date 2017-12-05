#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <bsd/string.h>

#include "storage.h"


// implementation for: man 2 access
// Checks if a file exists.
int
nufs_access(const char *path, int mask)
{
    printf("access(%s, %04o)\n", path, mask);
    return 0;
}

// implementation for: man 2 stat
// gets an object's attributes (type, permissions, size, etc)
int
nufs_getattr(const char *path, struct stat *st)
{
   // printf("getattr(%s)\n", path);
    int rv = get_stat(path, st);
    if (rv == -1) {
        return -ENOENT;
    }
    else {
        return 0;
    }
}

// implementation for: man 2 readdir
// lists the contents of a directory
int
nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi)
{
    struct stat st;

    printf("readdir(%s)\n", path);

    get_stat(path, &st);
    // filler is a callback that adds one item to the result
    // it will return non-zero when the buffer is full
    filler(buf, ".", &st, 0);
    return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
int
nufs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    printf("mknod(%s, %04o)\n", path, mode);
	printf("Is this a file: %d\n", S_ISREG(mode));
	printf("Is this a directory: %d\n", S_ISDIR(mode));
	if (createInode(path, mode, 1, 0, 6) < 0) {
		perror("no pls");
		return -1;
	}
	return 0;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int
nufs_mkdir(const char *path, mode_t mode)
{
    printf("mkdir(%s)\n", path);
	nufs_mknod(path, 0040000, 1);
    return 0;
}

int
nufs_unlink(const char *path)
{
    printf("unlink(%s)\n", path);
	// remove path from directory... i think
	// decrement link count... i think
    return -1;
}

int
nufs_rmdir(const char *path)
{
	// throw error saying must remove all contents first if there is stuff in directory... i think
    printf("rmdir(%s)\n", path);
    return -1;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int
nufs_rename(const char *from, const char *to)
{
    printf("rename(%s => %s)\n", from, to);
    return -1;
}

int
nufs_chmod(const char *path, mode_t mode)
{
    printf("chmod(%s, %04o)\n", path, mode);
    inode* inode = retrieve_inode(path);
	if (inode != NULL) {
		inode->fileData.mode = mode;
		return 0;
	}
	perror("Inode not found\n");
	return -ENOENT;
}

int
nufs_truncate(const char *path, off_t size)
{
    printf("truncate(%s, %ld bytes)\n", path, size);
    return -1;
}

// this is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
int
nufs_open(const char *path, struct fuse_file_info *fi)
{
    printf("open(%s)\n", path);
	struct stat* st = malloc(sizeof(struct stat));
    int rv = nufs_getattr(path, st);
	free(st);
	return rv;
}

// Actually read data
int
nufs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("read(%s, %ld bytes, @%ld)\n", path, size, offset);
    const char* data = get_data(path);

    int len = strlen(data) + 1;
    if (size < len) {
        len = size;
    }

    strlcpy(buf, data, len);
    return len;
}

// Actually write data
int
nufs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("write(%s, %ld bytes, @%ld)\n", path, size, offset);
    return -1;
}

// Update the timestamps on a file or directory.
int
nufs_utimens(const char* path, const struct timespec ts[2])
{
    int rv = storage_set_time(path, ts);
    printf("utimens(%s, [%ld, %ld; %ld %ld]) -> %d\n",
           path, ts[0].tv_sec, ts[0].tv_nsec, ts[1].tv_sec, ts[1].tv_nsec, rv);
	return rv;
}

void
nufs_init_ops(struct fuse_operations* ops)
{
    memset(ops, 0, sizeof(struct fuse_operations));
    ops->access   = nufs_access;
    ops->getattr  = nufs_getattr;
    ops->readdir  = nufs_readdir;
    ops->mknod    = nufs_mknod;
    ops->mkdir    = nufs_mkdir;
    ops->unlink   = nufs_unlink;
    ops->rmdir    = nufs_rmdir;
    ops->rename   = nufs_rename;
    ops->chmod    = nufs_chmod;
    ops->truncate = nufs_truncate;
    ops->open	  = nufs_open;
    ops->read     = nufs_read;
    ops->write    = nufs_write;
    ops->utimens  = nufs_utimens;
};

struct fuse_operations nufs_ops;

int
main(int argc, char *argv[])
{
    assert(argc > 2 && argc < 6);

    pages_init(argv[--argc]);
    nufs_init_ops(&nufs_ops);
    return fuse_main(argc, argv, &nufs_ops, NULL);
}

