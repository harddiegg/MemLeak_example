#include "mem_leak.h"

extern cp_mem_monitor *memory_list;
#define DEBUG_FILE "logfile"
#define OPEN_DEBUG FILE* leak_out = fopen( DEBUG_FILE, "a" )
#define CLOSE_DEBUG fclose( leak_out )
#define CLEAN_DEBUG FILE* leak_file = fopen( DEBUG_FILE, "w" ); fprintf( leak_file, "Start:\n" )

void cp_del_mem_entry(cp_mem_monitor *memory_list, void *ptr, const char *func, const int line, const char* file)
{
  if (!memory_list || !ptr)
    return;
  // fprintf(stderr, "lock: %s:%d\n", func, line);
  pthread_mutex_lock(&memory_list->lock);
  OPEN_DEBUG;
  cp_mem_item *tmp = memory_list->first;
  while(tmp)
  {
    // printf("Try:%p:%p:%d\n", tmp->ptr, ptr, tmp->ptr == ptr);
    if (tmp->ptr == ptr)
    {
      break;
    }
    tmp = tmp->next;
  }
  if (tmp)
  {
    fprintf(stderr  , "@%d %s:[%p] - [%s]\n", getpid(), file, ptr, tmp->mem_place);
    fprintf(leak_out, "@%d %s:[%p] - [%s]\n", getpid(), file, ptr, tmp->mem_place);
    fflush( NULL );
    memory_list->total_alloc-=tmp->size;
    LIST_DEL_ITEM(memory_list, tmp, cp_mem_item);
  }
  else
  {
    fprintf(stderr  , "Can't find:%s:[%p] at %s:%d\n", file, ptr, func, line);
    fprintf(leak_out, "Can't find:%s:[%p] at %s:%d\n", file, ptr, func, line);
    fflush( NULL );
  }
  // fprintf(stderr, "unlock: %s:%d\n", func, line);
  CLOSE_DEBUG;
  pthread_mutex_unlock(&memory_list->lock);
}
void cp_add_mem_entry(cp_mem_monitor *memory_list, void *ptr, ssize_t size, const char *func, const int line, const char* file)
{
  if (!memory_list || !ptr)
    return;
  // fprintf(stderr, "lock: %s:%d\n", func, line);
  pthread_mutex_lock(&memory_list->lock);
  OPEN_DEBUG;
  cp_mem_item *new_entry = NULL;
  LIST_CREATE_ITEM(new_entry, cp_mem_item);
  if (new_entry)
  {
    struct timeval tv;
    time_t nowtime;
    struct tm *nowtm;
    char tmbuf[32];

    gettimeofday(&tv, NULL);
    nowtime = tv.tv_sec;
    nowtm = localtime(&nowtime);
    strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
    strcpy(new_entry->time_stamp, tmbuf);
    sprintf(new_entry->mem_place, "%s:%d", func, line);
    //printf("%s:%d NEW ptr:%p   %d [%s]\n", __FUNCTION__, __LINE__, ptr, size, new_entry->mem_place);
    fprintf(stderr  , "@%d %s:[%p] + [%d] [%s]\n", getpid(), file, ptr, (int)size, new_entry->mem_place);
    fprintf(leak_out, "@%d %s:[%p] + [%d] [%s]\n", getpid(), file, ptr, (int)size, new_entry->mem_place);
    fflush( NULL );
    new_entry->ptr = ptr;
    new_entry->size = size;
    LIST_ADD_ITEM(memory_list, new_entry);
    memory_list->total_alloc +=size;
  }
  CLOSE_DEBUG;
  pthread_mutex_unlock(&memory_list->lock);
  // fprintf(stderr, "unlock: %s:%d\n", func, line);
}
void cp_show_mem_status(cp_mem_monitor *memory_list)
{
        if (memory_list)
        {
          // fprintf(stderr, "lock: %s:%d\n", __func__, __LINE__);

            pthread_mutex_lock(&memory_list->lock);
            OPEN_DEBUG;
            fprintf(stderr, "List itemCnt: %d IDCounter:%d TotalMem %d\n", memory_list->itemCnt, (int)memory_list->IDCounter, (int)memory_list->total_alloc);
            fprintf(leak_out, "List itemCnt: %d IDCounter:%d TotalMem %d\n", memory_list->itemCnt, (int)memory_list->IDCounter, (int)memory_list->total_alloc);
            fflush( NULL );
 //printf("List itemCnt: %d IDCounter:%d TotalMem %d\n", memory_list->itemCnt, memory_list->IDCounter, memory_list->total_alloc);
            cp_mem_item *__tmp = memory_list->first;
            while(__tmp)
            {
	            fprintf(stderr, "ID: %d ptr %p, size %d,alloced at %s [%s]\n", __tmp->ID, __tmp->ptr, (int)__tmp->size, __tmp->mem_place, __tmp->time_stamp);
	            fprintf(leak_out, "ID: %d ptr %p, size %d,alloced at %s [%s]\n", __tmp->ID, __tmp->ptr, (int)__tmp->size, __tmp->mem_place, __tmp->time_stamp);
                //printf("ID: %d ptr %p, size %d,alloced at %s\n", __tmp->ID, __tmp->ptr, __tmp->size, __tmp->mem_place);
	            __tmp = __tmp->next;
            }
            CLOSE_DEBUG;
            pthread_mutex_unlock(&memory_list->lock);
            // fprintf(stderr, "unlock %s:%d\n", __func__, __LINE__);
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
  // sleep(20);
  // fprintf(stderr, "lock: %s:%d\n", __func__, __LINE__);
  pthread_mutex_lock(&memory_list->lock);
  LIST_CLEAR(memory_list, cp_mem_item);
  pthread_mutex_unlock(&memory_list->lock);
  // fprintf(stderr, "unlock %s:%d\n", __func__, __LINE__);
  LIST_DEINIT(memory_list);
}
void* leak_malloc( ssize_t size, const char* func, int line, const char* file )
{
    pthread_mutex_lock(&memory_list->lock);
    OPEN_DEBUG;
    fprintf( leak_out, "LEAK_MALLOC: " );
    fflush( NULL );
    void* ptr = malloc(size);
    cp_add_mem_entry (memory_list, ptr, size, func, line, file);
    CLOSE_DEBUG;
    pthread_mutex_unlock(&memory_list->lock);
    return ptr;
}
void* leak_calloc( ssize_t nmemb, ssize_t size, const char* func, int line, const char* file )
{
    pthread_mutex_lock(&memory_list->lock);
    OPEN_DEBUG;
    fprintf( leak_out, "LEAK_CALLOC" );
    fflush( NULL );
    void* ptr = calloc(nmemb, size);
    cp_add_mem_entry (memory_list, ptr, size, func, line, file);
    CLOSE_DEBUG;
    pthread_mutex_unlock(&memory_list->lock);
    return ptr;
}
void leak_free( void* ptr, const char* func, int line, const char* file )
{
    pthread_mutex_lock(&memory_list->lock);
    OPEN_DEBUG;
    if ( ptr ) {
        fprintf( leak_out, "LEAK_FREE: " );
        fflush( NULL );
        cp_del_mem_entry(memory_list, ptr, func, line, file);
        free(ptr);
        ptr = NULL;
    }
    CLOSE_DEBUG;
    pthread_mutex_unlock(&memory_list->lock);
}
void* leak_realloc( void* ptr, ssize_t size, const char* func, int line, const char* file )
{
    pthread_mutex_lock(&memory_list->lock);
    OPEN_DEBUG;
    fprintf( leak_out, "LEAK_REALLOC: " );
    fprintf(stderr  , "@%d %s:[%p] < [%d] [%s:%d]\n", getpid(), file, ptr, (int)size, func, line);
    fprintf(leak_out, "@%d %s:[%p] < [%d] [%s:%d]\n", getpid(), file, ptr, (int)size, func, line);
    fflush( NULL );
    cp_del_mem_entry(memory_list, ptr, func, line, file);
    ptr = realloc(ptr, size);
    cp_add_mem_entry (memory_list, ptr, size, func, line, file);
    fprintf(stderr  , "@%d %s:[%p] > [%d] [%s:%d]\n", getpid(), file, ptr, (int)size, func, line);
    fprintf(leak_out, "@%d %s:[%p] > [%d] [%s:%d]\n", getpid(), file, ptr, (int)size, func, line);
    fflush( NULL );
    CLOSE_DEBUG;
    pthread_mutex_unlock(&memory_list->lock);
    return ptr;
}
void* leak_strdup( void* ptr, const char* func, int line, const char* file )
{
    pthread_mutex_lock(&memory_list->lock);
    OPEN_DEBUG;
    fprintf( leak_out, "LEAK_STRDUP: " );
    fflush( NULL );
    ssize_t size = strlen(ptr);
    void *ptr_ret = malloc(size);
    strncpy(ptr_ret, ptr, size);
    cp_add_mem_entry (memory_list, ptr_ret, size, func, line, file);
    CLOSE_DEBUG;
    pthread_mutex_unlock(&memory_list->lock);
    return ptr_ret;
}
void leak_free_only( void* ptr )
{
    pthread_mutex_lock(&memory_list->lock);
    OPEN_DEBUG;
    if ( ptr ) {
        fprintf( leak_out, "FREE_ONLY: " );
        fflush( NULL );
        cp_del_mem_entry(memory_list, ptr, "ONLY_DELETE", -1, "O_D");
        free(ptr);
    }
    CLOSE_DEBUG;
    pthread_mutex_unlock(&memory_list->lock);
}
