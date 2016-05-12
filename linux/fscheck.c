#include "fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

int main(int argc, char* argv[]) {
	int fs;
	int fsize;
	void* map;
	struct stat fstats;
	struct block* blocks;
	struct superblock* superblock;
	int ninodes;
	int ninodeblocks;
	int bitmaps;
	int nbitmapblocks;
	int ndatablocks;

	if(argc != 2)
	{
		return -1;
	}

	fs = open(argv[1], O_RDRW);

	if(fs == -1)
	{
		return -1;
	}

	if(fstat(fs, &fstats) != 0)
	{
		return -1;
	}

	fsize = fstats.st_size;

	map = mmap(NULL, fsize, PROT_READ | PROT_WRITE, 0, 0);

	if(map == MAP_FAILED)
	{
		return -1;
	}

	blocks = (struct block*)map; // Converts our mapping into an array of blocks;

	superblock = (struct superblock*)blocks[1]; // Gets a pointer to the superblock;

	ninodes = superblock->ninodes; // Gets the number of inodes in the system;

	ninodeblocks = ninodes / IPB; // Computes the number of inode blocks there are;

	bitmaps = BBLOCK(0, ninodes); // Finds the first block number of the bitmaps 

	ndatablocks = superblock->nblocks; // Gets the number of datablocks there are

	nbitmapblocks = ndatablocks / (BSIZE * BPB); // Computes the number of bit map blocks there are.

	printf("Number of inodes: %d, Number of inode blocks: %d\n", ninodes, ninodeblocks);
	printf("Location of bitmaps: %d, Number of data blocks: %d, Number of bitmap blocks: %d\n", bitmaps, ndatablocks, nbitmapblocks);

    return 0;
}
