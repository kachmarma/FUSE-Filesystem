//
// Created by Julian Hirn on 11/27/17.
//
#include <stdlib.h>
#include <stdio.h>
#include "bmap.h"
#include "util.h"

bmap createBitMap(void* location) {
  bmap* bmap = (struct bmap* )location;
  return *bmap;
}

void freeBitMap(bmap* bitmap)
{
  for (int ii = 0; ii < 8; ii++)
  {
    free(&bitmap->data[ii]);
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
        int mask = 1 << intIndex;
        bitmap->data[whichInt] = bitmap->data[whichInt] | mask;
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
  return (int)ceil(n * 8 / 256);
}

void clearBit(bmap* bitmap, int i)
{
  int whichInt = getIntNum(i);
  i %= 32;
  
  int mask = ~(1 << i);
  bitmap->data[whichInt] = bitmap->data[whichInt] & mask;
}

void printBitMap(bmap* bitmap)
{
  for(int i = 0; i < 8; i++) {
    for(int z = 31; z >= 0; z--) {
      printf("%ld", getBit(bitmap->data[i], z));
    }
  }
}
