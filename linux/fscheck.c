
#include "fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

char readbitmap(int block, int ninodes, struct block* fs)
{
	struct bitmap* bp = (struct bitmap*)&fs[BBLOCK(block, ninodes)];
	int bi, m;
  	bi = block % BPB;
  	m = 1 << (bi % 8);
  	if(bp->bitchunk[bi/8] & m)
  	{
  		return 1;
  	}
  	return 0;
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
	struct block* bitmap;
	int ninodes;
	int ninodeblocks;
	int bitmaps;
	int nbitmapblocks;
	int ndatablocks;
	struct dinode* rootnode;
	int maxblock;
	int mindatablock;
	char* addrsinuse;
	char* imrk;
	char* link;
	char* dironce;


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
	bzero(addrsinuse, maxblock);

	superblock = (struct superblock*)(&blocks[1]); // Gets a pointer to the superblock;
	inodes = (struct dinode*)(&blocks[2]);
	rootnode = &inodes[1];

        
	ninodes = superblock->ninodes; // Gets the number of inodes in the system;
	imrk = (char *)malloc(ninodes);
	link = (char* )malloc(ninodes);
	bzero(imrk, ninodes);
	bzero(link, ninodes);

	ninodeblocks = ninodes / IPB; // Computes the number of inode blocks there are;
	bitmaps = BBLOCK(0, ninodes); // Finds the first block number of the bitmaps 
	bitmap = &blocks[bitmaps];

	ndatablocks = superblock->nblocks; // Gets the number of datablocks there are

	nbitmapblocks = (ndatablocks / (BSIZE * BPB)) + 1; // Computes the number of bit map blocks there are.

	mindatablock = ninodeblocks + nbitmapblocks;
	datablocks = &blocks[mindatablock];

    int i, j = 0, k = 0, found = 0;
  
        
    if(rootnode == NULL || rootnode != &inodes[1] || rootnode->type != T_DIR)
    {
		fprintf(stderr, "ERROR: root directory does not exist.\n");
		return 1;
	}
        
	for(i = 1; i < ninodes && inodes[i].type != 0; i++)
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
				//fprintf(stderr, "min: %i, max: %i, addr: %i\n", mindatablock, maxblock, inodes[i].addrs[j]);
				if(((inodes[i].addrs[j] < mindatablock) || inodes[i].addrs[j] >= maxblock) && inodes[i].addrs[j] != 0) {
					
					fprintf(stderr, "ERROR: bad address in inode.\n");
					//printf("bad address\n");
					return 1;
				}
				else if(inodes[i].addrs[j] != 0) 
				{
					if (addrsinuse[inodes[i].addrs[j]] != 0)
					{
						fprintf(stderr, "ERROR: address used more than once.\n");
						return 1;
					}
					else
					{
						addrsinuse[inodes[i].addrs[j]] = 1;
					}
				}
			}
			if(inodes[i].addrs[NDIRECT])
				{
					for(j = 0; j < NINDIRECT; j++)
					{
						struct indirect* indiraddrs = (struct indirect*)&(blocks[inodes[i].addrs[NDIRECT]]);
						//fprintf(stderr, "indirect min: %i, max: %i, addr: %i\n", mindatablock, maxblock, indiraddrs->addrs[j]);
						if((indiraddrs->addrs[j] < mindatablock || indiraddrs->addrs[j] >= maxblock) && indiraddrs->addrs[j] != 0) {
							fprintf(stderr, "ERROR: bad address in inode.\n");
							//printf("bad address\n");
							return 1;
						}
						else if(indiraddrs->addrs[j] != 0)
						{

							if (addrsinuse[indiraddrs->addrs[j]] != 0)
							{
								fprintf(stderr, "ERROR: address used more than once.\n");
								return 1;
							}
							else
							{
								addrsinuse[indiraddrs->addrs[j]] = 1;
							}
						}
						
					}
				}
		}
		if(inodes[i].type == T_DIR)
		{
			found = 0;
			struct dirent* toparent = NULL;
			for(j = 0; j < NDIRECT; j++) {
				for(k = 0; k < DIRENTS; k++) {
					if(((struct dirent*)&(blocks[inodes[i].addrs[j]]))[k].inum != 0)
					{
						if(link[((struct dirent*)&(blocks[inodes[i].addrs[j]]))[k].inum] == 1);
							else {
								link[((struct dirent*)&(blocks[inodes[i].addrs[j]]))[k].inum]++;
							}
					}
					if(strcmp(((struct dirent*)&(blocks[inodes[i].addrs[j]]))[k].name, ".") == 0) {
						found++;
				        }
					else if(strcmp(((struct dirent*)&(blocks[inodes[i].addrs[j]]))[k].name, "..") == 0) {
						found++;
						toparent = &((struct dirent*)&(blocks[inodes[i].addrs[j]]))[k];
					}
				}
			}
			if(inodes[i].addrs[NDIRECT]) {
				struct indirect* indiraddrs = (struct indirect*)&(blocks[inodes[i].addrs[NDIRECT]]);
				for(j = 0; j < NINDIRECT; j++) {
					for(k = 0; k < DIRENTS; k++) {
						if((((struct dirent*)&(blocks[indiraddrs->addrs[j]]))[k].inum) != 0)
						{
							if(link[((struct dirent*)&(blocks[indiraddrs->addrs[j]]))[k].inum] == 1);
							else {
								link[((struct dirent*)&(blocks[indiraddrs->addrs[j]]))[k].inum]++;
							}
						}
						if(strcmp(((struct dirent*)&(blocks[indiraddrs->addrs[j]]))[k].name, ".") == 0) {
							found++;
				        	}
						else if(strcmp(((struct dirent*)&(blocks[indiraddrs->addrs[j]]))[k].name, "..") == 0) {
							found++;
							toparent = &((struct dirent*)&(blocks[indiraddrs->addrs[j]]))[k];
						}
					}
				}
			}
			if (found != 2) {
				fprintf(stderr, "ERROR: directory not properly formatted.\n");
				return 1;
			}
			/*else {
				int index = 0;
				for(j = 0; j < NDIRECT; j++) {
					for(k = 0; k < DIRENTS; k++) {
						if(((struct dirent*)&(blocks[inodes[toparent->inum].addrs[j]]))[k].inum != 0) {
							printf("index: %i\n", index);
							index = ((struct dirent*)&(blocks[inodes[toparent->inum].addrs[j]]))[k].inum;
						}
					}
				}
				if(inodes[toparent->inum].addrs[NDIRECT]) {
					struct indirect* indiraddrs = (struct indirect*)&(blocks[inodes[toparent->inum].addrs[NDIRECT]]);
					for(j = 0; j < NINDIRECT; j++) {
						for(k = 0; k < DIRENTS; k++) {
							if(((struct dirent*)&(blocks[indiraddrs->addrs[j]]))[k].inum != 0) {
								printf("index: %i\n", index);
								index = ((struct dirent*)&(blocks[indiraddrs->addrs[j]]))[k].inum;
							}
						}
					}
				}
				printf("toparent->inum: %i\n", toparent->inum);
				printf("toparent->name: %s\n", toparent->name);
				printf("i: %i\n", i);
				printf("index: %i\n", index);
				if(i != index) {
					fprintf(stderr, "ERROR: parent directory mismatch.\n");
					return 1;
				}
			}*/
		}
	}


	for(i = mindatablock; i < maxblock; i++)
	{
		int addr = addrsinuse[i];
		int bitmap = readbitmap(i, ninodes, blocks);
		if(addr ^ bitmap)
		{
			if(addr == 1)
			{
				//fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
				//return 1;
			}
			if(bitmap == 1)
			{
				//fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
				//return 1;
			}
		}
	}

	for(i = 0; i < ninodes; i++)
	{
		if(inodes[i].type == 0 && imrk[i] == 1)
		{
			fprintf(stderr, "ERROR: inode referred to in directory but marked free.\n");
			return 1;
		}
		
		//printf("%ith time --> links: %i, counted: %i\n", i, inodes[i].nlink, imrk[i]);
		if(inodes[i].nlink != imrk[i]) {
			if(inodes[i].type == T_DIR)
			{
				fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
				return 1;
			}			
			fprintf(stderr, "ERROR: bad reference count for file.\n");
			return 1;
		}
		
		if((inodes[i].type == T_DEV || inodes[i].type == T_DIR || inodes[i].type == T_FILE) && imrk[i] == 0)
		{
			fprintf(stderr, "ERROR: inode marked use but not found in a directory.\n");
			return 1;	
		}
	}

	dironce = (char*)malloc(ninodes);
	bzero(dironce, ninodes);
	/*for(i = 1; i < ninodes && inodes[i].type == T_DIR; i++)
	{
		if(inodes[i].type == T_DIR)
		{
			for(j = 0; j < NDIRECT; j++) {
				for(k = 0; k < DIRENTS; k++) {
					if(((struct dirent*)&(blocks[inodes[i].addrs[j]]))[k].inum != 0)
					{
						if(dironce[((struct dirent*)&(blocks[inodes[i].addrs[j]]))[k].inum] == 0)
						{
							dironce[((struct dirent*)&(blocks[inodes[i].addrs[j]]))[k].inum] = 1;
						}
						else
						{
							fprintf(stderr,"ERROR: directory appears more than once in file system.\n");
							return 1;
						}
					}
				}
			}
			if(inodes[i].addrs[NDIRECT]) {
				struct indirect* indiraddrs = (struct indirect*)&(blocks[inodes[i].addrs[NDIRECT]]);
				for(j = 0; j < NINDIRECT; j++) {
					for(k = 0; k < DIRENTS; k++) {
						if((((struct dirent*)&(blocks[indiraddrs->addrs[j]]))[k].inum) != 0)
						{
							if(dironce[((struct dirent*)&(blocks[indiraddrs->addrs[j]]))[k].inum] == 0)
							{
								dironce[((struct dirent*)&(blocks[indiraddrs->addrs[j]]))[k].inum] = 1;
							}
							else {
								fprintf(stderr,"ERROR: directory appears more than once in file system.\n");
								return 1;
							}
						}
					}
				}
			}
		}
	}*/
	

    return 0;
}
