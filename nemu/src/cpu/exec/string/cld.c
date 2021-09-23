#include "cpu/exec/helper.h"

make_helper(cld)
{
	cpu.eflags.DF = 0;
	return 1;
}
