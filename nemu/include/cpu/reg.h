#ifndef __REG_H__
#define __REG_H__

#include "common.h"
#include "../../../lib-common/x86-inc/cpu.h"

enum { R_EAX, R_ECX, R_EDX, R_EBX, R_ESP, R_EBP, R_ESI, R_EDI };
enum { R_AX, R_CX, R_DX, R_BX, R_SP, R_BP, R_SI, R_DI };
enum { R_AL, R_CL, R_DL, R_BL, R_AH, R_CH, R_DH, R_BH };
enum { R_ES, R_CS, R_SS, R_DS, R_FS, R_GS };

/* TODO: Re-organize the `CPU_state' structure to match the register
 * encoding scheme in i386 instruction format. For example, if we
 * access cpu.gpr[3]._16, we will get the `bx' register; if we access
 * cpu.gpr[1]._8[1], we will get the 'ch' register. Hint: Use `union'.
 * For more details about the register encoding scheme, see i386 manual.
 */
struct SREG{
		uint16_t selector;
		union {
			struct {
				uint32_t seg_base1 :16;
				uint32_t seg_base2 :8;
				uint32_t seg_base3 :8;
			};
			uint32_t seg_base;
		};
		union {
			struct {
				uint32_t seg_limit1 :16;
				uint32_t seg_limit2 :4;
				uint32_t seg_limit3 :12;
			};
			uint32_t seg_limit;
		};
};

typedef struct {
     union{
        union {
		uint32_t _32;
		uint16_t _16;
		uint8_t _8[2];
	} gpr[8];

	/* Do NOT change the order of the GPRs' definitions. */

	struct{
            uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
        };
     };

     swaddr_t eip;

     union {
		struct {
			uint32_t CF		:1;
			uint32_t pad0	:1;
			uint32_t PF		:1;
			uint32_t pad1	:1;
			uint32_t AF		:1;
			uint32_t pad2	:1;
			uint32_t ZF		:1;
			uint32_t SF		:1;
			uint32_t TF		:1;
			uint32_t IF		:1;
			uint32_t DF		:1;
			uint32_t OF		:1;
			uint32_t IOPL	:2;
			uint32_t NT		:1;
			uint32_t pad3	:1;
			uint16_t pad4;
		};
		uint32_t val;
	} eflags;

    struct GDTR{
		uint32_t base_addr;
		uint16_t seg_limit;
	}gdtr;

	CR0 cr0;
	CR3 cr3;

	union{
		struct SREG sr[6];
		struct{
			struct SREG es, cs, ss, ds, fs, gs;
		};
	};


} CPU_state;
typedef  union {
	struct {
		uint16_t rpl	:2;
		uint16_t ti	:1;
		uint16_t index 	:13;
	};
	uint16_t val;
}SELECTOR;
typedef struct {
	union {
		struct {
			uint32_t seg_limit1	:16;
			uint32_t seg_base1	:16;
		};
		uint32_t first_part;
	};
	union {
		struct {
			uint32_t seg_base2 	:8;
			uint32_t type		:5;
			uint32_t dpl		:2;
			uint32_t p		:1;
			uint32_t seg_limit2	:4;
			uint32_t avl		:1;
			uint32_t 		:1;
			uint32_t b		:1;
			uint32_t g		:1;
			uint32_t seg_base3	:8;
		};
		uint32_t second_part;
	};
}SEG_descriptor;
typedef struct {
	union {
		struct {
			uint32_t p 	:1;
			uint32_t rw	:1;
			uint32_t us	:1;
			uint32_t 	:2;
			uint32_t a	:1;
			uint32_t d 	:1;
			uint32_t 	:2;
			uint32_t avail	:3;
			uint32_t addr 	:20;
		};
		uint32_t page_val;
	};
}PAGE_descriptor;
CPU_state cpu;
uint8_t current_sreg;
SEG_descriptor *seg_des;

static inline int check_reg_index(int index) {
	assert(index >= 0 && index < 8);
	return index;
}

#define reg_l(index) (cpu.gpr[check_reg_index(index)]._32)
#define reg_w(index) (cpu.gpr[check_reg_index(index)]._16)
#define reg_b(index) (cpu.gpr[check_reg_index(index) & 0x3]._8[index >> 2])

extern const char* regsl[];
extern const char* regsw[];
extern const char* regsb[];

void sreg_load();

#endif
