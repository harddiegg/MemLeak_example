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
    char mem_place[128];
    char time_stamp[32];
    struct cp_mem_item *next;
    struct cp_mem_item *prev;
}cp_mem_item;

typedef struct cp_mem_monitor{
    ssize_t IDCounter;
    ssize_t total_alloc;
    int itemCnt;
    pthread_mutex_t lock;
    cp_mem_item *first;
    cp_mem_item *last;
}cp_mem_monitor;
cp_mem_monitor *memory_list;

void cp_del_mem_entry(cp_mem_monitor *memory_list, void *ptr, const char *func, const int line, const char* file);
void cp_add_mem_entry(cp_mem_monitor *memory_list, void *ptr, ssize_t size, const char *func, const int line, const char* file);
void cp_show_mem_status(cp_mem_monitor *memory_list);
void cp_mem_monitor_init(void);
void cp_mem_monitor_deinit(void);
void* leak_malloc( ssize_t size, const char* func, int line, const char* file );
void* leak_calloc( ssize_t nmemb, ssize_t size, const char* func, int line, const char* file );
void* leak_realloc( void* ptr, ssize_t size, const char* func, int line, const char* file );
void* leak_strdup( void* ptr, const char* func, int line, const char* file );
void leak_free( void* ptr, const char* func, int line, const char* file );
void leak_free_only( void* ptr );

#define LEAK_MALLOC( size ) leak_malloc( size, __func__, __LINE__, __FILE__ )
#define LEAK_CALLOC( nmemb, size ) leak_calloc( nmemb, size, __func__, __LINE__, __FILE__ )
#define LEAK_REALLOC( ptr, size ) leak_realloc( ptr, size, __func__, __LINE__, __FILE__ )
#define LEAK_FREE( ptr ) leak_free( ptr, __func__, __LINE__, __FILE__ )
#define LEAK_STRDUP( ptr ) leak_strdup( ptr, __func__, __LINE__, __FILE__ )
#define LEAK_ONLY_FREE leak_free_only
#define LEAK_STATUS cp_show_mem_status( memory_list );

#define xfree(ptr) if (ptr){free(ptr); ptr=NULL;}

#define LIST_INIT(list, type)                               \
        {                                                   \
            list = (type *)LEAK_CALLOC(1, sizeof(type));         \
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
#define LIST_FIND_ITEM(list, type, finded, findID)          \
        if (list)                                           \
        {                                                   \
            type *__tmp = list->first;                      \
            while(__tmp)                                    \
            {                                               \
                if (__tmp->ID == findID)                    \
                {                                           \
                    finded = __tmp;                         \
                    break;                                  \
                }                                           \
                __tmp = __tmp->next;                        \
            }                                               \
        }
#define LIST_SHOW(list, type)                               \
        if (list)                                           \
        {                                                   \
			printf("List itemCnt: %d IDCounter:%d\n", list->itemCnt, list->IDCounter);\
            type *__tmp = list->first;                      \
            while(__tmp)                                    \
            {                                               \
                printf("ID: %d\n", __tmp->ID);              \
                __tmp = __tmp->next;                        \
            }                                               \
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

#endif
