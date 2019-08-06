#if !defined(_CLIENTLIB_H_)
#define _CLIENTLIB_H_

#include <communication.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int os_connect(char* name);

int os_store(char* name, void* block, size_t len);

void* os_retrive(char* name);

int os_delete(char* name);

int os_disconnect();

#endif