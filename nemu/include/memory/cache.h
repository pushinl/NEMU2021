#ifndef __CACHE_H__
#define __CACHE_H_

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
//lnaddr_t seg_translate(swaddr_t, size_t, uint8_t);
//hwaddr_t page_translate(lnaddr_t);
//CPU_state cpu;
extern uint8_t current_sreg;


struct Cache{
	bool valid;
	int tag;
	uint8_t data[BLOCK_SIZE];
}cache[STO_SIZE_L1/BLOCK_SIZE];

struct SecondaryCache{
	bool valid, dirty;
	int tag;
	uint8_t data[BLOCK_SIZE];
}cache2[STO_SIZE_L1/BLOCK_SIZE];

void init_cache();
uint32_t Secondary_Cache_read(hwaddr_t addr);
uint32_t cache_read(hwaddr_t addr);
void Secondary_Sache_Write(hwaddr_t addr, size_t len,uint32_t data) ;
void Cache_Write(hwaddr_t addr, size_t len,uint32_t data);

#endif