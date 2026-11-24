#include <stdlib.h>

int atoi(const char* s) {
    int sign = 1, n = 0;
    while (*s == ' ') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') n = n * 10 + (*s++ - '0');
    return sign * n;
}

int abs(int x) {
    return x < 0 ? -x : x;
}