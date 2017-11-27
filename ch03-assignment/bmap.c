//
// Created by Julian Hirn on 11/27/17.
//

#include "bmap.h"
#include <stdlib.h>

bmap createBitMap() {
        bmap newMap = calloc(sizeof(bmap));
        return newMap;
}

void freeBitMap(bmap* bitmap)
{
        for (int ii = 0; ii < 4; ii++)
        {
                free(bitmap->data[ii]);
        }

        free(bitmap);
}

int getBit(long n, int i)
{
        return ((n & (1 << i)) != 0);
}

int setFirstAvailable(bmap* bitmap)
{
        int current = 0;

        for (int whichLong = 0; whichLong < 4; whichLong++)
        {
                for (int longIndex = 0; longIndex < 64; longIndex++)
                {
                        if (getBit(bitmap->data[whichLong], longIndex) == 0)
                        {
                                return whichLong * 64 + longIndex;
                        }
                }
        }

        perror("rasietn"); // TODO throw a perror
        return -1;
}

int getLongNum(int n)
{
        // throw if out of range
        if (n < 0 || n > 255) {
                perror("");
        }

        if (n < 64) {
                return 0;
        } else if (n < 128) {
                return 1;
        } else if (n < 256) {
                return 2;
        } else {
                return 3;
        }
}

void clearBit(bmap* bitmap, int i)
{
        int whichLong = getLongNum(i);
        i %= 64;

        int mask = ~(1 << i);
        bitmap->data[i] = bitmap->data[i] & mask;
}