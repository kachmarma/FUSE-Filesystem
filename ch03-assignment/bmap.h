//
// Created by Julian Hirn on 11/27/17.
//

#ifndef CH03_BMAP_H
#define CH03_BMAP_H
typedef struct bmap {
    long data[4];
};

/**
 * Create new bitmap.
 * @return the newly created bitmap.
 */
bmap createBitMap();

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

#endif //CH03_BMAP_H
