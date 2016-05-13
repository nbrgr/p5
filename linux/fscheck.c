#include "fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

char getbit(char byte, int position)
{
	return ((byte >> position) & 0x01); 
}

int main(int argc, char* argv[]) {
	int fs;
	int fsize;
	void* map;
	struct stat fstats;
	struct block* blocks;
	struct block* datablocks;
	struct superblock* superblock;
	struct dinode* inodes;
	int ninodes;
	int ninodeblocks;
	int bitmaps;
	int nbitmapblocks;
	int ndatablocks;
	struct dinode* rootnode;
	int maxblock;
	int mindatablock;
	char* addrsinuse;

	if(argc != 2)
	{
		fprintf(stderr, "Incorrect number of arguments\n");
		return 1;
	}

	fs = open(argv[1], O_RDONLY);

	if(fs == -1)
	{
		fprintf(stderr, "image not found.\n");
		return 1;
	}

	if(fstat(fs, &fstats) != 0)
	{
		fprintf(stderr, "Failed to retrieve stats\n");
		return 1;
	}

	fsize = fstats.st_size;

	map = mmap(NULL, fsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fs, 0);

	if(map == MAP_FAILED)
	{
		fprintf(stderr, "Failed to map\n");
		return 1;
	}

	blocks = (struct block*)map; // Converts our mapping into an array of blocks;
	maxblock = fsize / BSIZE; // This should be the total number of blocks / when indexing all iterators should be below this value.
	addrsinuse = (char*)malloc(maxblock);

	superblock = (struct superblock*)(&blocks[1]); // Gets a pointer to the superblock;
	inodes = (struct dinode*)(&blocks[2]);
	rootnode = &inodes[1];

        
	ninodes = superblock->ninodes; // Gets the number of inodes in the system;

	ninodeblocks = ninodes / IPB; // Computes the number of inode blocks there are;
	bitmaps = BBLOCK(0, ninodes); // Finds the first block number of the bitmaps 

	ndatablocks = superblock->nblocks; // Gets the number of datablocks there are

	nbitmapblocks = (ndatablocks / (BSIZE * BPB)) + 1; // Computes the number of bit map blocks there are.

	mindatablock = 1 + ninodeblocks + nbitmapblocks;
	datablocks = &blocks[mindatablock];

	//printf("Number of inodes: %d, Number of inode blocks: %d\n", ninodes, ninodeblocks);
	//printf("Location of bitmaps: %d, Number of data blocks: %d, Number of bitmap blocks: %d\n", bitmaps, ndatablocks, nbitmapblocks);
	//printf("Root node type: %d\n", rootnode->type);
        int i, j = 0;
        //DIR* dir;
        
	for(i = 1; i < ninodes + 1 && inodes[i].type != 0; i++)
	{
		//fprintf(stderr, "inode type: %d\n", inodes[i].type);
		if((inodes[i].type != T_DIR) && (inodes[i].type != T_FILE) && (inodes[i].type != T_DEV))
		{
			fprintf(stderr, "ERROR: bad inode.\n");
			//printf("bad inode.\n");
			return 1;
		}
		if((inodes[i].type == T_DIR) || (inodes[i].type == T_FILE) || (inodes[i].type == T_DEV)) {
			for(j = 0; j < NDIRECT; j++) {
				fprintf(stderr, "min: %i, max: %i, addr: %i\n", mindatablock, maxblock, inodes[i].addrs[j]);
				if(((inodes[i].addrs[j] < mindatablock) || inodes[i].addrs[j] >= maxblock) && inodes[i].addrs[j] != 0) {
					
					fprintf(stderr, "ERROR: bad address in inode.\n");
					//printf("bad address\n");
					return 1;
				}
				else 
				{
					if (addrsinuse[j] != 0)
					{
						fprintf(stderr, "ERROR: address used more than once.");
						return 1;
					}
					else
					{
						addrsinuse[j] = 1;
					}
				}
			}
			if(inodes[i].addrs[NDIRECT])
				{
					for(j = 0; j < NINDIRECT; j++)
					{
						struct indirect* indiraddrs = (struct indirect*)&(blocks[inodes[i].addrs[NDIRECT]]);
						fprintf(stderr, "indirect min: %i, max: %i, addr: %i\n", mindatablock, maxblock, indiraddrs->addrs[j]);
						if(indiraddrs->addrs[j] < mindatablock || indiraddrs->addrs[j] >= maxblock) {
							fprintf(stderr, "ERROR: bad address in inode.\n");
							//printf("bad address\n");
							return 1;
						}
					}
				}
		}
		if(inodes[i].type == T_DIR) {
			for(j = 0; j < NDIRECT; j++) {
				if(inodes[i].addrs[j] == 0) {
					
				}
			}
			//fprintf(stderr, "ERROR: directory not properly formatted.\n");
		}
		
		
	}
	if(rootnode == NULL || rootnode != &inodes[1] || rootnode.type != T_DIR) {
		fprintf(stderr, "ERROR: root directory does not exist.\n");
		return 1;
	}
	

    return 0;
}
