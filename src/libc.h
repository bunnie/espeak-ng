char*  strcpy(char *, const char *);
char*  strchr(const char *, int);
int    strcmp(const char *, const char *);
char*  strcat(char *, const char *);

void * memcpy (void *dest, const void *src, size_t len);
void * memset (void *dest, int val, size_t len);
size_t strlen(const char *);

#define sprintf sprintf_
int sprintf_(char* buffer, const char* format, ...);

#define snprintf  snprintf_
int  snprintf_(char* buffer, size_t count, const char* format, ...);