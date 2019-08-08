#if !defined(REQ_H)
#define REQ_H

#include <unistd.h>

void initTable();

int handle_register(char* name);

int handle_store(char* path, void* block, size_t len);

int handle_retrive(char* name);

int handle_delete(char* name);

int handle_disconnect();

#endif