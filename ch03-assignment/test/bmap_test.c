//
//  bmap_test.c
//  z
//
//  Created by Julian Hirn on 12/5/17.
//

#include <stdio.h>

#include "bmap.h"

int main(int _argc, char* _argv[])
{
  bmap myBmap;
  printf("Should print empty bitmap: ");
  printBitMap(&myBmap);
  
  printf("\n===============================================\nNow looping through and setting all spots on bmap.");
  for (int ii = 0; ii < 256; ii++)
  {
    setFirstAvailable(&myBmap);
    printf("\n[%3d]: ", ii);
    printBitMap(&myBmap);
  }
  
  printf("\n===============================================\nNow looping through and clearing all spots on bmap.");
  for (int ii = 0; ii < 256; ii++)
  {
    clearBit(&myBmap, ii);
    printf("\n[%3d]: ", ii);
    printBitMap(&myBmap);
  }
  
  printf("\n\nFirst 64 and clearing every third.\n");
  for (int ii = 0; ii < 64; ii++) {
    printf("[%d]", setFirstAvailable(&myBmap));
  }
  
  clearBit(&myBmap, 53);
  printf("\n\n[%d] = 53 ?\n\n", setFirstAvailable(&myBmap));
  
//  for (int ii = 0; ii < 64; ii += 3) {
//    clearBit(&myBmap, ii);
//  }
  
  printBitMap(&myBmap);
}
