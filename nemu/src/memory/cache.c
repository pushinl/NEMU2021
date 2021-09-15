#include "common.h"
#include "memory/cache.h"
#include <stdlib.h>
#include "burst.h"


void init_cache(){

	int i;
	for(i = 0 ; i < STO_SIZE_L1/BLOCK_SIZE; i++ ){
		cache[i].valid = 0;
		cache[i].tag = 0;
		//memset(cache[i].data,0,BLOCK_SIZE);
	}

	for (i = 0;i < STO_SIZE_L2/BLOCK_SIZE;i ++){
		cache2[i].valid = 0;
		cache2[i].dirty = 0;
		cache2[i].tag = 0;
		//memset (cache2[i].data,0,BLOCK_SIZE);
	}

    printf("qwq\n");

}

uint32_t Secondary_Cache_read(hwaddr_t addr)
{
	uint32_t g = (addr >> 6) & ((1<<12) - 1); //group number
	uint32_t block = (addr >> 6)<<6;
	int i;
	bool v = false;
	for (i = g * SIXTEEN_WAY ; i < (g + 1) * SIXTEEN_WAY ;i ++)
	{
		if (cache2[i].tag == (addr >> 18)&& cache2[i].valid)
			{
				v = true;
				break;
			}
	}
	if (!v)
	{
		int j;
		for (i = g * SIXTEEN_WAY ; i < (g + 1) * SIXTEEN_WAY ;i ++)
		{
			if (!cache2[i].valid)break;
		}
		if (i == (g + 1) * SIXTEEN_WAY)//ramdom
		{
			srand (0);
			i = g * SIXTEEN_WAY + rand() % SIXTEEN_WAY;
			if (cache2[i].dirty)
			{
				uint8_t mask[BURST_LEN * 2];
				memset(mask, 1, BURST_LEN * 2);
				for (j = 0;j < BLOCK_SIZE/BURST_LEN;j ++)
				ddr3_write(block + j * BURST_LEN, cache2[i].data + j * BURST_LEN, mask);
			}
		}
		cache2[i].valid = true;
		cache2[i].tag = addr >> 18;
		cache2[i].dirty = false;
		for (j = 0;j < BURST_LEN;j ++)
		ddr3_read(block + j * BURST_LEN , cache2[i].data + j * BURST_LEN);
	}
	return i;
}


uint32_t cache_read(hwaddr_t addr){
	uint32_t g = (addr>>6) & 0x7f;
	int i;
	bool v = 0;
	for( i = g * EIGHT_WAY ; i < (g+1)* EIGHT_WAY ; i++ ){
		if(cache[i].tag==(addr>>13)&&cache[i].valid){
			v = 1;
			break;
		}
	}
	if (!v)
	{
		int j = Secondary_Cache_read(addr);
		for (i = g * EIGHT_WAY ; i < (g+1) * EIGHT_WAY ;i ++)
		{
			if (!cache[i].valid)break;
		}
		if (i == (g + 1) * EIGHT_WAY)//ramdom
		{
			srand (0);
			i = g * EIGHT_WAY + rand() % EIGHT_WAY;
		}
		cache[i].valid = true;
		cache[i].tag = addr >> 13;
		memcpy (cache[i].data,cache2[j].data,BLOCK_SIZE);
	}
	return i;
}

void Secondary_Sache_Write(hwaddr_t addr, size_t len,uint32_t data) {
	uint32_t g = (addr >> 6) & ((1<<12) - 1);  //group number
	uint32_t offset = addr & (BLOCK_SIZE - 1); // inside addr
	int i;
	bool v = false;
	for (i = g * SIXTEEN_WAY ; i < (g + 1) * SIXTEEN_WAY ;i ++)
	{
		if (cache2[i].tag == (addr >> 13) && cache2[i].valid)
			{
				v = true;
				break;
			}
	}
	if (!v)i = Secondary_Cache_read (addr);
	cache2[i].dirty = true;
	memcpy (cache2[i].data + offset , &data , len);
}

void Cache_Write(hwaddr_t addr, size_t len,uint32_t data) {
	uint32_t g = (addr>>6) & 0x7f; //group number
	uint32_t offset = addr & (BLOCK_SIZE - 1); // inside addr
	int i;
	bool v = false;
	for (i = g * EIGHT_WAY ; i < (g + 1) * EIGHT_WAY ;i ++)
	{
		if (cache[i].tag == (addr >> 13)&& cache[i].valid)
			{
				v = true;
				break;
			}
	}
	if (v)
	{
		memcpy (cache[i].data + offset , &data , len);
	}
	Secondary_Sache_Write(addr,len,data);
}