//
// Created by Julian Hirn on 11/27/17.
//

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
#include <stdlib.h>

#ifndef CH03_BMAP_H
#define CH03_BMAP_H

typedef struct bmap {
    int data[8];
} bmap;

/**
 * Create new bitmap.
 * @return the newly created bitmap.
 */
bmap createBitMap(void* location);

/**
 * Free the bitmap.
 * @param bitmap the bitmap to free.
 */
void freeBitMap(bmap* bitmap);

/**
 * Sets the first available bit in the map.
 * @param bitmap the bitmap.
 * @return the index of the first available position.
 */
int setFirstAvailable(bmap* bitmap);

/**
 * Clear the bitmap at index.
 * @param bitmap the bitmap to clear from.
 * @param i index to clear in the bitmap.
 */
void clearBit(bmap* bitmap, int i);

/**
 * Print the bitmap. 
 */
void printBitMap(bmap* bitmap);

#endif //CH03_BMAP_H
