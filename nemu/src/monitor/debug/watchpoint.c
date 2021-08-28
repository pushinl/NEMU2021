#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include "nemu.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;
int num = 0;

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp() {
	WP *new, *p;
	new = free_;
	free_ = free_->next;
	p = head;
	new->next = NULL;
	if(p == NULL) {
		head = new;
		p = head;
	} else {
		while(p->next != NULL) p = p->next;
		p->next = new;
	}
	return new;
}

void free_wp(WP *wp) {
	WP *p;
	p = free_;
	if(p == NULL) {
		free_ = wp;
		p = free_;
	} else {
		while(p->next != NULL) p = p->next;
		p->next = wp;
	}
	if(head == NULL)
		panic("Error: WatchPoint pool is empty.\n");
	if(head->NO == wp->NO) {
		head = head->next;
	} else {
		p = head;
		while(p->next != NULL && p->NO != wp->NO) p = p->next;
		if(p->next == NULL && p->NO == wp->NO) head = NULL;
		else if(p->next->NO == wp->NO) p->next = p->next->next;
		else panic("error free\n");
	}
	wp->next = NULL;
	wp->val = 0;
	printf("WatchPoint %d id released.\n", wp->NO);
}

void delete_wp(int num) {
	WP *wp;
	if(num >= 32 || num < 0) panic("Error WatchPoint!\n");
	wp = &wp_pool[num];
	free_wp(wp);
}

void info_wp() {
	WP *wp;
	wp = head;
	if(wp == NULL) 
		printf("There is no WatchPoint.\n");
	while(wp != NULL) {
		printf("WatchPoint %d : %s = %d\n", wp->NO, wp->expr, wp->val);
		wp = wp->next;
	}
}

bool check_wp() {
	WP *wp;
	wp = head;
	bool success = true, flag = false;
	while(wp != NULL) {
		uint32_t tmp_val = expr(wp->expr, &success);
		if(!success) 
			panic("Expression failed!(watchpoint)\n");
		if(tmp_val != wp->val) {
			flag = true;
			printf("Hint watchpoint %d at address 0x%08x\n", wp->NO, cpu.eip);
			printf("Old value: 0x%08x\n", wp->val);
			printf("New value: 0x%08x\n", tmp_val);
		}
		wp = wp->next;
	}
	return flag;
}