#ifndef STRING_H_
#define STRING_H_

#include "types.h"

int  kstrcmp(const char* a, const char* b);
int  kstrlen(const char* s);
void kstrcpy(char* dst, const char* src);
void kmemset(void* p, uint8_t v, int n);

#endif