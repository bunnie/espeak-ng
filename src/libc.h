#include <stdint.h>

char*  strcpy(char *, const char *);
char*  strchr(const char *, int);
int    strcmp(const char *, const char *);
char*  strcat(char *, const char *);
char*  strdup(const char *);
char*  strncpy(char *, const char *, size_t);
int atoi(const char *);

int    memcmp(const void *, const void *, size_t);
void * memcpy (void *dest, const void *src, size_t len);
void * memset (void *dest, int val, size_t len);
size_t strlen(const char *);

#define sprintf sprintf_
int sprintf_(char* buffer, const char* format, ...);

#define snprintf  snprintf_
int  snprintf_(char* buffer, size_t count, const char* format, ...);

#define printf printf_
int printf_(const char* format, ...);

char *strtok(char *s, const char *delim);
int	 sscanf(const char *, const char *, ...);
char *strtok_r(char *s, const char *delim, char **last);
intmax_t strtoimax(const char *nptr, char **endptr, int base);
