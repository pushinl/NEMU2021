#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include <stdlib.h>

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

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

static WP* new_WP() {
	assert(free_ != NULL);   //如果没有空闲监视点，返回错误
	WP *p = free_;
	free_ = free_->next;  //调出一个监视点
	return p;
}

static void free_WP(WP *p) {
	assert(p >= wp_pool && p < wp_pool + NR_WP);//首先要free的监视点需要合法 
	free(p->expr);    
	p->next = free_;  //回收到free_中,链表头插法
	free_ = p;
}

int set_watchpoint(char *e) {
	uint32_t val;
	bool success;
	val = expr(e, &success); //首先要找到这个值
	if(!success) return -1;  //找不到的话返回错误

	WP *p = new_WP();    //然后建立一个新的监视点
	p->expr = strdup(e);  
	p->old_val = val;   //设置值 以后做对比用

	p->next = head;    //加入head 头插法  
	head = p;

	return p->NO;     //返回监视点号
}

bool delete_watchpoint(int NO) {
	WP *p, *prev = NULL;
	for(p = head; p != NULL; prev = p, p = p->next) {
		if(p->NO == NO) { break; }    //找到要删除的监视点
	}

	if(p == NULL) { return false; }    //不合法
	if(prev == NULL) { head = p->next; }  //删的是第一个
	else { prev->next = p->next; }   //链表中的删除表中元素

	free_WP(p);  //free掉
	return true;
}

void list_watchpoint() {
	if(head == NULL) {
		printf("No watchpoints\n");   //head是空的 当然不能list
		return;
	}

	printf("%8s\t%8s\t%8s\n", "NO", "Address", "Enable");
	WP *p;
	for(p = head; p != NULL; p = p->next) {  //链表中的打印各个元素值，过于熟悉了
		printf("%8d\t%s\t%#08x\n", p->NO, p->expr, p->old_val);
	}
}

WP* scan_watchpoint() {  //检查有没有监视点监视的式子值发生改变
	WP *p;
	for(p = head; p != NULL; p = p->next) {
		bool success;
		p->new_val = expr(p->expr, &success);
		if(p->old_val != p->new_val) {
			return p;
		}
	}

	return NULL;
}

