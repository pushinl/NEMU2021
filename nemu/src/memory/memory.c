#include "common.h"
#include  "cpu/reg.h"
#include <stdlib.h>
#include "burst.h"


#define BLOCK_SIZE 64
#define STO_SIZE_L1  64*1024
#define STO_SIZE_L2  4*1024*1024

#define EIGHT_WAY  8
#define SIXTEEN_WAY  16
uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);
void ddr3_read(hwaddr_t, void*);
void ddr3_write(hwaddr_t, void*,uint8_t*);
int is_mmio(hwaddr_t);
uint32_t mmio_read(hwaddr_t, size_t, int);
void mmio_write(hwaddr_t, size_t, uint32_t, int);
lnaddr_t seg_translate(swaddr_t, size_t, uint8_t);
hwaddr_t page_translate(lnaddr_t);
CPU_state cpu;
extern uint8_t current_sreg;


struct Cache{
	bool valid;
	int tag;
	uint8_t data[BLOCK_SIZE];
}cache[STO_SIZE_L1/BLOCK_SIZE];

struct Secondary_Cache{
	bool valid, dirty;
	int tag;
	uint8_t data[BLOCK_SIZE];
}cache2[STO_SIZE_L2/BLOCK_SIZE];


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
		if (cache2[i].tag == (addr >> 13)&& cache2[i].valid)
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
/* Memory accessing interfaces */

uint32_t hwaddr_read(hwaddr_t addr, size_t len) {
	int index = is_mmio(addr);
	if ( index >= 0)
	{
		return mmio_read(addr, len, index);
	}
	uint32_t offset = addr & (BLOCK_SIZE - 1); // inside addr
	uint32_t block = cache_read(addr);
	uint8_t temp[4];
	memset (temp,0,sizeof (temp));

	if (offset + len >= BLOCK_SIZE)
	{
		uint32_t _block = cache_read(addr + len);
		memcpy(temp,cache[block].data + offset, BLOCK_SIZE - offset);
		memcpy(temp + BLOCK_SIZE - offset,cache[_block].data, len - (BLOCK_SIZE - offset));
	}
	else
	{
		memcpy(temp,cache[block].data + offset,len);
	}
	int zero = 0;
	uint32_t tmp = unalign_rw(temp + zero, 4) & (~0u >> ((4 - len) << 3));
	return tmp;
}

void hwaddr_write(hwaddr_t addr, size_t len, uint32_t data) {
	int index = is_mmio(addr);
	if ( index >= 0)
	{
		mmio_write(addr, len, data, index);
		return ;
	}
	Cache_Write(addr, len, data);
}

uint32_t lnaddr_read(lnaddr_t addr, size_t len) {
	#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	size_t max_len = ((~addr) & 0xfff) + 1;
    	if (len > max_len) 
    	{
        		uint32_t low = lnaddr_read(addr, max_len);
        		uint32_t high = lnaddr_read(addr + max_len, len - max_len);
        		return (high << (max_len << 3)) | low;
    	}
	hwaddr_t hwaddr = page_translate(addr);
	return hwaddr_read(hwaddr, len);
}

void lnaddr_write(lnaddr_t addr, size_t len, uint32_t data) {
	#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	size_t max_len = ((~addr) & 0xfff) + 1;
    	if (len > max_len) 
    	{
        		lnaddr_write(addr, max_len, data & ((1 << (max_len << 3)) - 1));
        		lnaddr_write(addr + max_len, len - max_len, data >> (max_len << 3));
        		return;
    	}
	hwaddr_t hwaddr = page_translate(addr);
	hwaddr_write(hwaddr, len, data);
}

uint32_t swaddr_read(swaddr_t addr, size_t len) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
    uint32_t Inaddr;
	Inaddr =seg_translate(addr,len,current_sreg);
	return lnaddr_read(addr, len);
}

void swaddr_write(swaddr_t addr, size_t len, uint32_t data) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
    uint32_t Inaddr;
	Inaddr  = seg_translate(addr,len,current_sreg);
	return lnaddr_write(addr, len, data);
}

hwaddr_t page_translate_additional(lnaddr_t addr,int* flag){
	if (cpu.cr0.protect_enable == 1 && cpu.cr0.paging == 1){
		//printf("%x\n",addr);
		uint32_t dir = addr >> 22;
		uint32_t page = (addr >> 12) & 0x3ff;
		uint32_t offset = addr & 0xfff;

		// get dir position
		uint32_t dir_start = cpu.cr3.page_directory_base;
		uint32_t dir_pos = (dir_start << 12) + (dir << 2);
		PAGE_descriptor first_content;
		first_content.page_val = hwaddr_read(dir_pos,4);
		if (first_content.p == 0) {
			*flag = 1;
			return 0;
		}

		// get page position
		uint32_t page_start = first_content.addr;
		uint32_t page_pos = (page_start << 12) + (page << 2);
		PAGE_descriptor second_content;
		second_content.page_val =  hwaddr_read(page_pos,4);
		if (second_content.p == 0){
			*flag = 2;
			return 0;
		}

		// get hwaddr
		uint32_t addr_start = second_content.addr;
		hwaddr_t hwaddr = (addr_start << 12) + offset;
		return hwaddr;
	}else return addr;
}