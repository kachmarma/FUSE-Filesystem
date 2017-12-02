//
// Created by Julian Hirn on 11/27/17.
//
#include "bmap.h"

bmap createBitMap(void* location) {
       bmap* bmap = (bmap *)location;
       return *bmap;
}

void freeBitMap(bmap* bitmap)
{
        for (int ii = 0; ii < 8; ii++)
        {
                free(bitmap->data[ii]);
        }

        free(bitmap);
}

int getBit(int n, int i)
{
        return ((n & (1 << i)) != 0);
}

int setFirstAvailable(bmap* bitmap)
{
        int current = 0;

        for (int whichInt = 0; whichInt < 8; whichInt++)
        {
                for (int intIndex = 0; intIndex < 32; intIndex++)
                {
                        if (getBit(bitmap->data[whichInt], intIndex) == 0)
                        {
                                return whichInt * 32 + intIndex;
                        }
                }
        }

        perror("rasietn"); // TODO throw a perror
        return -1;
}

int getIntNum(int n)
{
        // throw if out of range
        if (n < 0 || n > 255) {
                perror("");
        }
        return ceil(n * 8 / 256);
}

void clearBit(bmap* bitmap, int i)
{
        int whichInt = getIntNum(i);
        i %= 32;

        int mask = ~(1 << i);
        bitmap->data[i] = bitmap->data[i] & mask;
}

void printBitMap(bmap* bitmap)
{
        for(int i = 0; i < 3; i++) {
                for(int z = 31; z >= 0; z--) {
                        printf(getBit(bitmap->data[i], z));
                }
        }
}