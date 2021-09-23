#include <nemu.h>

lnaddr_t seg_translate(swaddr_t addr, size_t len, uint8_t sreg) {
	//printf("here %d",cpu.cr0.protect_enable);
	if (cpu.cr0.protect_enable == 0)return addr;
	//printf("%d, %d\n",(int)(addr+len), ((int)cpu.sr[sreg].seg_limit));
	Assert(addr+len < cpu.sr[sreg].seg_limit, "cs segment out limit");
	return cpu.sr[sreg].seg_base + addr;	
}
