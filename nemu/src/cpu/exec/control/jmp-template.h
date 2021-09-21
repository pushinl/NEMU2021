#include "cpu/exec/template-start.h"

#define instr jmp

static void do_execute() {
	cpu.eip += op_src->val;
	print_asm(str(instr) " %x", cpu.eip + 1 + DATA_BYTE);
}

make_instr_helper(si)
#if DATA_BYTE == 4
make_helper(jmp_rm_l) {
	int len = decode_rm_l(eip + 1);
	cpu.eip = op_src->val - (len + 1);
	print_asm(str(instr) " *%s", op_src->str);
	return len + 1;
}
#endif

#if DATA_BYTE == 4
make_helper(ljmp) {
	extern SEG_descriptor *seg_des;
	SEG_descriptor seg;
	seg_des = &seg;
	uint32_t op1 = instr_fetch(eip+1, 4)-7;
	uint16_t op2 = instr_fetch(eip +5, 2);
	cpu.eip = op1;
	cpu.cs.selector = op2;
	Assert(((cpu.cs.selector>>3)<<3) <= cpu.gdtr.seg_limit, "segment out limit %d, %d", ((cpu.cs.selector>>3)<<3), cpu.gdtr.seg_limit);
	seg_des->first_part = instr_fetch(cpu.gdtr.base_addr + ((cpu.cs.selector>>3)<<3), 4);
	seg_des->second_part = instr_fetch(cpu.gdtr.base_addr + ((cpu.cs.selector>>3)<<3)+4, 4);
	Assert(seg_des->p == 1, "segment error");
	cpu.cs.seg_base1 = seg_des->seg_base1;
	cpu.cs.seg_base2 = seg_des->seg_base2;
	cpu.cs.seg_base3 = seg_des->seg_base3;
	cpu.cs.seg_limit1 = seg_des->seg_limit1;
	cpu.cs.seg_limit2 = seg_des->seg_limit2;
	cpu.cs.seg_limit3 = 0xfff;
	print_asm("ljmp %x,%x", op2, op1+7);
	return 7;
}
#endif

#include "cpu/exec/template-end.h"
