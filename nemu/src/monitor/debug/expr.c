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

	{" +",	NOTYPE, 0},				//spaces
	{"\\+", PLUS, 4},					//plus
	{"-", MINUS, 4},					//minus
	{"\\*", TIMES, 5},					//times
	{"/", DIV, 5},						//divide
	{"==", EQ, 3},						//equal
	{"!=", NOTEQ, 3},					//not eq
	{"&&", AND, 1},						//and
	{"\\|\\|", OR, 2},					//or
	{"!", NOT, 6},						//not
	{"\\(", LB, 6},					//lb
	{"\\)", RB, 6},					//rb
	{"0[xX][0-9a-zA-Z]+", HEX, 0},		//hex
	{"[0-9]+", DEC, 0},				//dec

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
				char *substr_start = e + position;
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
						strncpy(tokens[nr_token].str, e + position - substr_len, substr_len);
						tokens[nr_token].str[substr_len] = '\0';
					default :
						if(rules[i].token_type == MINUS) {	//negative
							if(nr_token == 0) tokens[++nr_token].type = NEG;
							else if(PLUS <= tokens[nr_token - 1].type && tokens[nr_token - 1].type <= LB) {
								tokens[++nr_token].type = NEG;
							} else tokens[++nr_token].type = MINUS;
						} else if(rules[i].token_type == TIMES) { //pointer
							if(nr_token == 0) tokens[++nr_token].type = POINTER;
							else if(PLUS <= tokens[nr_token - 1].type && tokens[nr_token - 1].type <= LB) {
								tokens[++nr_token].type = POINTER;
							} else tokens[++nr_token].type = TIMES;
						} else {
							tokens[++nr_token].type = rules[i].token_type;
						}
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
	*success ;
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
			const char *RE[] = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"};
			int i;
			for(i = 0; i < 8; i++)
				if(strcmp(tokens[l].str, RE[i]) == 0)
					return cpu.gpr[i]._32;
			return *success = false;
		}
		bool flag = check_parentheses(l, r, success);
		if(!success) {
			printf("ERROR!!!");
			return 0;
		}
		if(flag)
			return eval(l + 1, r - 1, success);
		int now = -1, i, cnt = 0;
		for(i = l; i <= r; i++) {
			if(tokens[i].type == LB) cnt++;
			if(tokens[i].type == RB) cnt--;
			if(cnt == 0) {
				
			}
		}

	}
}


uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	/* TODO: Insert codes to evaluate the expression. */
	//panic("please implement me");
	return 0;
}

