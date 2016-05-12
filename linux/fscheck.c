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

	blocks = (struct block*)map;
	superblock = (struct superblock*)blocks[1];
	ninodes = superblock->ninodes;
	ninodeblocks = ninodes / IPB;

    return 0;
}
