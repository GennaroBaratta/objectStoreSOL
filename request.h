#if !defined(REQ_H)
#define REQ_H

#include <unistd.h>

void initTable();
void freeTable();

int handle_register(char* name);

int handle_store(char* path, void* block, size_t len);

int handle_retrive(char* path,char** res);

int handle_delete(char* path);

int handle_disconnect();

void handle_print_stats();

#endif