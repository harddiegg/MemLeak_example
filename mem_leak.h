#ifndef _MEM_LEAK_
#define _MEM_LEAK_

#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

typedef struct cp_mem_item{
	int ID;
	void *ptr;
	ssize_t size;
	const char* place_func;
	int place_line;
	char time_stamp[32];
	struct cp_mem_item *next;
	struct cp_mem_item *prev;
} cp_mem_item;

typedef struct cp_mem_monitor{
	ssize_t IDCounter;
	ssize_t total_alloc;
	int itemCnt;
	pthread_mutex_t lock;
	cp_mem_item *first;
	cp_mem_item *last;
} cp_mem_monitor;
extern cp_mem_monitor *memory_list;

void cp_del_mem_entry(cp_mem_monitor *memory_list, void *ptr, const char *func, const int line, const char* file);
void cp_add_mem_entry(cp_mem_monitor *memory_list, void *ptr, ssize_t size, const char *func, const int line, const char* file);
void cp_show_mem_status(cp_mem_monitor *memory_list);
void cp_mem_monitor_init(void);
void cp_mem_monitor_deinit(void);
void* leak_malloc(ssize_t size, const char* func, int line, const char* file);
void* leak_calloc(ssize_t nmemb, ssize_t size, const char* func, int line, const char* file);
void* leak_realloc(void* ptr, ssize_t size, const char* func, int line, const char* file);
char* leak_strdup(const char* ptr, const char* func, int line, const char* file);
void leak_free(void* ptr, const char* func, int line, const char* file);
void leak_free_only(void* ptr);

#define LEAK_MALLOC(size) leak_malloc(size, __func__, __LINE__, __FILE__)
#define LEAK_CALLOC(nmemb, size) leak_calloc(nmemb, size, __func__, __LINE__, __FILE__)
#define LEAK_REALLOC(ptr, size) leak_realloc(ptr, size, __func__, __LINE__, __FILE__)
#define LEAK_FREE(ptr) leak_free(ptr, __func__, __LINE__, __FILE__)
#define LEAK_STRDUP(ptr) leak_strdup(ptr, __func__, __LINE__, __FILE__)
#define LEAK_ONLY_FREE leak_free_only
#define LEAK_STATUS cp_show_mem_status(memory_list);

#endif
