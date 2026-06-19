#include "../include/string.h"

int kstrcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

int kstrlen(const char* s) {
    int i = 0;
    while (s[i]) i++;
    return i;
}

void kstrcpy(char* dst, const char* src) {
    while ((*dst++ = *src++));
}

void kmemset(void* p, uint8_t v, int n) {
    uint8_t* b = (uint8_t*)p;
    while (n--) *b++ = v;
}