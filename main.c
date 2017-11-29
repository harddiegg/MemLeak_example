#include <stdio.h>
#include "mem_leak.h"

extern cp_mem_monitor *memory_list;

int main() {
	cp_mem_monitor_init();

	char *mas = LEAK_STRDUP("Hello world");
	LEAK_FREE(mas);

	mas = LEAK_MALLOC(100);
	LEAK_FREE(mas);

	mas = LEAK_CALLOC(1, 10);
	mas = LEAK_REALLOC(mas, 55);
	LEAK_FREE(mas);
	
	cp_mem_monitor_deinit();
	return 0;
}
