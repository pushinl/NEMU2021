#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 0,
	PLUS,
	MINUS,
	TIMES,
	DIV,
	EQ, NOTEQ,
	AND, OR, NOT,
	NEG, POINTER,
	LB, RB,
	HEX, DEC,
	REG, 

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
	int priority;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE, 0},
	{"\\+", PLUS, 4},
	{"-", MINUS, 4},
	{"\\*", TIMES, 5},
	{"/", DIV, 5},
	{"==", EQ, 3},
	{"!=", NOTEQ, 3},
	{"&&", AND, 1},
	{"\\|\\|", OR, 2},
	{"!", NOT, 6},
	{"\\(", LB, 6},
	{"\\)", RB, 6},
	{"0[xX][0-9a-zA-Z]+", HEX, 0},
	{"[0-9]+", DEC, 0},
	{"\\$[a-zA-Z]+", REG, 0}

};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
	int priority;
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				//char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				//Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
					case NOTYPE : break;
					case HEX : case DEC : case REG :
						strncpy(tokens[nr_token+1].str, e + position - substr_len, substr_len);
						tokens[nr_token+1].str[substr_len] = '\0';
					default :
						if(rules[i].token_type == MINUS) {	//negative
							if(nr_token == 0) tokens[++nr_token].type = NEG;
							else if(PLUS <= tokens[nr_token].type && tokens[nr_token].type <= LB) {
								tokens[++nr_token].type = NEG;
							} else tokens[++nr_token].type = MINUS;
						} else if(rules[i].token_type == TIMES) { //pointer
							if(nr_token == 0) tokens[++nr_token].type = POINTER;
							else if(PLUS <= tokens[nr_token].type && tokens[nr_token].type <= LB) {
								tokens[++nr_token].type = POINTER;
							} else tokens[++nr_token].type = TIMES;
						} else {
							tokens[++nr_token].type = rules[i].token_type;
						}
						tokens[nr_token].type = rules[i].priority;
						Log("priority: %d || match tokens[%d] = \"%s\" at position %d", tokens[nr_token].priority, nr_token, tokens[nr_token].str, position);
						break;
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}

bool check_parentheses(int l, int r, bool *success) {
	*success = true;
	if(l > r) return *success = false;
	int cnt = 0, flag = 1, i; // flag : the LB and RB is matched or not
	for(i = l; i <= r; i++) {
		if(tokens[i].type == LB) cnt++;
		if(tokens[i].type == RB) cnt--;
		if(cnt < 0) return *success = false;
		if(i != r && cnt == 0) flag = 0;
	}
	if(cnt != 0) return *success = false;
	return flag;
}

uint32_t eval(int l, int r, bool *success) {
	if(l > r) return *success = false;
	if(l == r) {
		uint32_t tmp;
		if(tokens[l].type == HEX) {
			sscanf(tokens[l].str, "%x", &tmp);
			return tmp;
		}else if(tokens[l].type == DEC) {
			sscanf(tokens[l].str, "%d", &tmp);
			return tmp;
		}else if(tokens[l].type == REG) {
			const char *RE[] = {"$eax", "$ecx", "$edx", "$ebx", "$esp", "$ebp", "$esi", "$edi"};
			const char *REB[] = {"$EAX", "$ECX", "$EDX", "$EBX", "$ESP", "$EBP", "$ESI", "$EDI"};
			int i;
			if(strcmp(tokens[l].str, "$eip") == 0 || strcmp(tokens[l].str, "$EIP") == 0) return cpu.eip;
			for(i = 0; i < 8; i++)
				if(strcmp(tokens[l].str, RE[i]) == 0 || strcmp(tokens[l].str, REB[i]) == 0)
					return cpu.gpr[i]._32;
			return *success = false;
		}
	}

	bool flag = check_parentheses(l, r, success);
	if(!success) {
		printf("ERROR!!!");
		return 0;
	}
	if(flag)
		return eval(l + 1, r - 1, success);
	int nxt = 10, i, cnt = 0;
	for(i = l; i <= r; i++) {
		if(tokens[i].type == LB) cnt++;
		if(tokens[i].type == RB) cnt--;
		if(cnt == 0) {
			if(tokens[i].type >= PLUS && tokens[i].type <= NOT && tokens[i].priority < nxt)
				nxt = i;
		}
	}
	assert(cnt == 0);
	uint32_t a = eval(l, nxt - 1, success);
	uint32_t b = eval(nxt + 1, r, success);
	switch (tokens[nxt].type) {
		case PLUS:
			return a + b;
			break;
		case MINUS:
			return a - b;
			break;
		case TIMES:
			return a * b;
			break;
		case DIV:
			return a / b;
			break;
		case EQ:
			return a == b;
			break;
		case NOTEQ:
			return a != b;
			break;
		case AND:
			return a && b;
			break;
		case OR:
			return a || b;
			break;
		
	}
	return 0;
}


uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	/* TODO: Insert codes to evaluate the expression. */
	//panic("please implement me");
	return eval(1, nr_token, success);
}

