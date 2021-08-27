#include "monitor/watchpoint.h"
#include "monitor/expr.h"

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
	
}

void delete_wp(int num) {
	WP *wp;
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

