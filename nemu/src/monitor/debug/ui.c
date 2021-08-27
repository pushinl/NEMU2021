#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args) {
	if(args != NULL) cpu_exec(atoi(args));
	else cpu_exec(1);
	return 0;
}

static int cmd_info(char *args) {
	if(args == NULL) return 0;
	char opt;
	sscanf(args, " %c", &opt);
	if(opt == 'r') {
		printf("%%eax:    0x%x    %d\n", cpu.eax, cpu.eax);
		printf("%%ecx:    0x%x    %d\n", cpu.ecx, cpu.ecx);
		printf("%%edx:    0x%x    %d\n", cpu.edx, cpu.edx);
		printf("%%ebx:    0x%x    %d\n", cpu.ebx, cpu.ebx);
		printf("%%esp:    0x%x    %d\n", cpu.esp, cpu.esp);
		printf("%%ebp:    0x%x    %d\n", cpu.ebp, cpu.ebp);
		printf("%%esi:    0x%x    %d\n", cpu.esi, cpu.esi);
		printf("%%edi:    0x%x    %d\n", cpu.edi, cpu.edi);
	}
	return 0;
}

static int cmd_x(char *args) {
	if(args == NULL) return 0;
	int n = 0;
	bool success = true;
	uint32_t ram_addr_start;
	while(args[0] == ' ') args++;
	while('0' <= args[0] && args[0] <= '9') n = (n << 3) + (n << 1) + (args[0] & 15), args++;
	ram_addr_start = expr(args, &success);
	if(!success) {
		printf("Expression Error!\n");
	}
	//printf("%x", ram_addr_start);
	int i, cnt = 0;
	for(i = 0; i < n; i++) {
		cnt++;
		if(cnt % 4 == 1) printf("0x%x: ", ram_addr_start);
		/*
		for(j = 0; j < 4; j++){
			uint32_t data = swaddr_read(ram_addr_start++, 1);
			printf("0x%02x ", data & 0xff);
		}*/
		uint32_t data = swaddr_read(ram_addr_start, 4);
		printf("0x%08x ", data);
		ram_addr_start += 4;
		if(cnt % 4 == 0) printf("\n");
	}
	printf("\n");
	return 0;
}

static int cmd_p(char *args){
	if(args == NULL) return 0;
	bool success = true;
	uint32_t EXPR = expr(args, &success);
	if(!success) {
		printf("Expression Error!\n");
		return 0;
	}
	printf("0x%x\n", EXPR);
	return 0;
}

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si", "Excecute the program N steps", cmd_si },
	{ "info", "Print the value of registers", cmd_info },
	{ "x", "Scan the RAM", cmd_x },
	{ "p", "Expression evaluation", cmd_p }
	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
