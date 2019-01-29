#include "mem_leak.h"

#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

#define xfree(ptr) if (ptr){free(ptr); ptr=NULL;}

#define LIST_INIT(list, type)                               \
        {                                                   \
            list = (type *)calloc(1, sizeof(type));         \
            list->first = list->last = NULL;                \
            list->IDCounter = 1;                            \
        }
#define LIST_CREATE_ITEM(item, type)                        \
        {                                                   \
            item = (type *)calloc(1, sizeof(type));         \
        }
#define LIST_ADD_ITEM(list, newItem)                        \
        if (list && newItem)                                \
        {                                                   \
            newItem->ID = list->IDCounter++;                \
            if (!list->first)                               \
            {                                               \
                list->first = list->last = newItem;         \
            }                                               \
            else                                            \
            {                                               \
                list->last->next = newItem;                 \
                newItem->prev = list->last;                 \
                list->last = newItem;                       \
            }                                               \
            list->itemCnt++;                                \
        }
#define LIST_DEL_ITEM(list, itemToDel, type)                \
        if (list && itemToDel)                              \
        {                                                   \
            type *__tmp = itemToDel->next;                  \
            while(__tmp)                                    \
            {                                               \
                __tmp->ID--;                                \
                __tmp = __tmp->next;                        \
            }                                               \
            if (itemToDel->prev == NULL)                    \
            {                                               \
                list->first = itemToDel->next;              \
            }                                               \
            else                                            \
            {                                               \
                itemToDel->prev->next = itemToDel->next;    \
            }                                               \
            if (itemToDel->next == NULL)                    \
            {                                               \
                list->last = itemToDel->prev;               \
            }                                               \
            else                                            \
            {                                               \
                itemToDel->next->prev = itemToDel->prev;    \
            }                                               \
            xfree(itemToDel);                               \
            list->itemCnt--;                                \
            list->IDCounter--;                              \
        }
#define LIST_CLEAR(list, type)                              \
        {                                                   \
            if (list)                                       \
            {                                               \
                type *__tmp = list->first;                  \
                while(__tmp)                                \
                {                                           \
                        type *__del = __tmp;                \
                        __tmp = __tmp->next;                \
                        xfree(__del);                       \
                }                                           \
                list->IDCounter = 0;                        \
                list->itemCnt = 0;                          \
            }                                               \
        }
#define LIST_DEINIT(list)                                   \
        xfree(list);

cp_mem_monitor *memory_list;

#if 1
#define DEBUG_FILE "/tmp/leak_logfile"
#define OPEN_DEBUG FILE* leak_out = fopen( DEBUG_FILE, "a" )
#define CLOSE_DEBUG fclose( leak_out )
#define CLEAN_DEBUG FILE* leak_file = fopen( DEBUG_FILE, "a" ); fprintf( leak_file, "Start:\n" ); fclose( leak_file )
#else
#define DEBUG_FILE
#define OPEN_DEBUG FILE* leak_out = stderr
#define CLOSE_DEBUG
#define CLEAN_DEBUG
#endif

#define PRINT_ALLOC_INFO 1

void cp_del_mem_entry(cp_mem_monitor *memory_list, void *ptr, const char *func, const int line, const char* file)
{
	if (!memory_list || !ptr)
		return;
	OPEN_DEBUG;
	cp_mem_item *tmp = memory_list->first;
	while(tmp) {
		if (tmp->ptr == ptr) {
			break;
		}
		tmp = tmp->next;
	}
	if (tmp) {
		if (PRINT_ALLOC_INFO) {
			fprintf(leak_out, "@%d %s:[%p] - [%s:%d]\n", getpid(), file, ptr, tmp->place_func, tmp->place_line);
			fflush(NULL);
		}
		memory_list->total_alloc-=tmp->size;
		LIST_DEL_ITEM(memory_list, tmp, cp_mem_item);
	} else {
		fprintf(leak_out, "Can't find:%s:[%p] at %s:%d\n", file, ptr, func, line);
		fflush(NULL);
	}
	CLOSE_DEBUG;
}
void cp_add_mem_entry(cp_mem_monitor *memory_list, void *ptr, ssize_t size, const char *func, const int line, const char* file)
{
	if (!memory_list || !ptr)
		return;
	OPEN_DEBUG;
	cp_mem_item *new_entry = NULL;
	LIST_CREATE_ITEM(new_entry, cp_mem_item);
	if (new_entry) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		strftime(new_entry->time_stamp, sizeof(new_entry->time_stamp), "%Y-%m-%d %H:%M:%S", localtime(&tv.tv_sec));
		new_entry->place_func = func;
		new_entry->place_line = line;

		if (PRINT_ALLOC_INFO) {
			fprintf(leak_out, "@%d %s:[%p] + [%d] [%s:%d]\n", getpid(), file, ptr, (int)size, new_entry->place_func, new_entry->place_line);
			fflush(NULL);
		}
		new_entry->ptr = ptr;
		new_entry->size = size;
		LIST_ADD_ITEM(memory_list, new_entry);
		memory_list->total_alloc +=size;
	}
	CLOSE_DEBUG;
}
void cp_show_mem_status(cp_mem_monitor *memory_list)
{
	if (memory_list) {
		pthread_mutex_lock(&memory_list->lock);
		OPEN_DEBUG;
		fprintf(leak_out, "@%d START PRINT MEMORY STATE:\n", getpid());
		fprintf(leak_out, "List itemCnt: %d IDCounter:%d TotalMem %d\n", memory_list->itemCnt, (int)memory_list->IDCounter, (int)memory_list->total_alloc);
		fprintf(leak_out, "@%d STOP PRINT MEMORY STATE\n\n\n", getpid());
		fflush( NULL );
		cp_mem_item *__tmp = memory_list->first;
		while(__tmp) {
			fprintf(leak_out, "ID: %d ptr %p, size %d, alloced at %s:%d [%s]\n", __tmp->ID, __tmp->ptr, (int)__tmp->size, __tmp->place_func, __tmp->place_line, __tmp->time_stamp);
			__tmp = __tmp->next;
		}
		CLOSE_DEBUG;
		pthread_mutex_unlock(&memory_list->lock);
	} else {
		fprintf(stderr, "Not available\n");
	}
}
void cp_mem_monitor_init(void)
{
	LIST_INIT(memory_list, cp_mem_monitor);
	pthread_mutex_init(&memory_list->lock, NULL);
	CLEAN_DEBUG;
}
void cp_mem_monitor_deinit(void)
{
	cp_show_mem_status(memory_list);
	OPEN_DEBUG;
	pthread_mutex_lock(&memory_list->lock);
	LIST_CLEAR(memory_list, cp_mem_item);
	pthread_mutex_unlock(&memory_list->lock);
	CLOSE_DEBUG;
	LIST_DEINIT(memory_list);
}
void* leak_malloc(ssize_t size, const char* func, int line, const char* file)
{
	pthread_mutex_lock(&memory_list->lock);
	OPEN_DEBUG;
	if (PRINT_ALLOC_INFO) {
		fprintf(leak_out, "LEAK_MALLOC: ");
		fflush(NULL);
	}
	void* ptr = malloc(size);
	cp_add_mem_entry (memory_list, ptr, size, func, line, file);
	CLOSE_DEBUG;
	pthread_mutex_unlock(&memory_list->lock);
	return ptr;
}
void* leak_calloc(ssize_t nmemb, ssize_t size, const char* func, int line, const char* file)
{
	pthread_mutex_lock(&memory_list->lock);
	OPEN_DEBUG;
	if (PRINT_ALLOC_INFO) {
		fprintf(leak_out, "LEAK_CALLOC: ");
		fflush(NULL);
	}
	void* ptr = calloc(nmemb, size);
	cp_add_mem_entry (memory_list, ptr, size, func, line, file);
	CLOSE_DEBUG;
	pthread_mutex_unlock(&memory_list->lock);
	return ptr;
}
void leak_free(void* ptr, const char* func, int line, const char* file)
{
	pthread_mutex_lock(&memory_list->lock);
	OPEN_DEBUG;
	if (ptr) {
		if (PRINT_ALLOC_INFO) {
			fprintf(leak_out, "LEAK_FREE: ");
			fflush(NULL);
		}
		cp_del_mem_entry(memory_list, ptr, func, line, file);
		free(ptr);
		ptr = NULL;
	}
	CLOSE_DEBUG;
	pthread_mutex_unlock(&memory_list->lock);
}
void* leak_realloc(void* ptr, ssize_t size, const char* func, int line, const char* file)
{
	pthread_mutex_lock(&memory_list->lock);
	OPEN_DEBUG;
	if (PRINT_ALLOC_INFO) {
		fprintf(leak_out, "LEAK_REALLOC: ");
		fflush(NULL);
	}
	cp_del_mem_entry(memory_list, ptr, func, line, file);
	ptr = realloc(ptr, size);
	if (PRINT_ALLOC_INFO) {
		fprintf(leak_out, "LEAK_REALLOC: ");
		fflush(NULL);
	}
	cp_add_mem_entry (memory_list, ptr, size, func, line, file);
	CLOSE_DEBUG;
	pthread_mutex_unlock(&memory_list->lock);
	return ptr;
}
char* leak_strdup(const char* ptr, const char* func, int line, const char* file)
{
	pthread_mutex_lock(&memory_list->lock);
	OPEN_DEBUG;
	if (PRINT_ALLOC_INFO) {
		fprintf(leak_out, "LEAK_STRDUP: ");
		fflush(NULL);
	}
	ssize_t size = strlen(ptr);
	char *ptr_ret = (char *)malloc(size);
	strncpy(ptr_ret, ptr, size);
	cp_add_mem_entry(memory_list, ptr_ret, size, func, line, file);
	CLOSE_DEBUG;
	pthread_mutex_unlock(&memory_list->lock);
	return ptr_ret;
}
void leak_free_only(void* ptr)
{
	pthread_mutex_lock(&memory_list->lock);
	OPEN_DEBUG;
	if (ptr) {
		if (PRINT_ALLOC_INFO) {
			fprintf(leak_out, "FREE_ONLY: ");
			fflush(NULL);
		}
		cp_del_mem_entry(memory_list, ptr, "ONLY_DELETE", -1, "O_D");
		free(ptr);
	}
	CLOSE_DEBUG;
	pthread_mutex_unlock(&memory_list->lock);
}
