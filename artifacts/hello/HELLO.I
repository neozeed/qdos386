# 1 "HELLO.C"
# 1 "./stdio.h" 1
 





# 1 "./stddef.h" 1
 


typedef unsigned int size_t;
typedef int ptrdiff_t;





# 7 "./stdio.h" 2

# 1 "./stdarg.h" 1
 





typedef char *va_list;






# 8 "./stdio.h" 2











 




typedef struct qd_file FILE;
typedef long fpos_t;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

 
int putchar(int c);
int puts(const char *s);
int putc(int c, FILE *fp);
int fputc(int c, FILE *fp);
int fputs(const char *s, FILE *fp);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp);
int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);
int fprintf(FILE *fp, const char *fmt, ...);
int vfprintf(FILE *fp, const char *fmt, va_list ap);
int sprintf(char *buf, const char *fmt, ...);
 
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
int vsprintf(char *buf, const char *fmt, va_list ap);
int fflush(FILE *fp);
int setvbuf(FILE *fp, char *buf, int mode, size_t size);
void setbuf(FILE *fp, char *buf);

 
int getchar(void);
int fgetc(FILE *fp);
int getc(FILE *fp);
int ungetc(int c, FILE *fp);
char *fgets(char *buf, int n, FILE *fp);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *fp);

 
FILE *fopen(const char *name, const char *mode);
int fclose(FILE *fp);
int fileno(FILE *fp);                        
FILE *fdopen(int fd, const char *mode);     
int fseek(FILE *fp, long off, int whence);
long ftell(FILE *fp);
int feof(FILE *fp);
int ferror(FILE *fp);
void clearerr(FILE *fp);
void rewind(FILE *fp);

 
int fgetpos(FILE *fp, fpos_t *pos);
int fsetpos(FILE *fp, const fpos_t *pos);

 

int scanf(const char *fmt, ...);
int fscanf(FILE *fp, const char *fmt, ...);
int sscanf(const char *s, const char *fmt, ...);
int vsscanf(const char *s, const char *fmt, va_list ap);
char *gets(char *buf);

 

void perror(const char *s);
int remove(const char *name);
int rename(const char *from, const char *to);
FILE *tmpfile(void);
char *tmpnam(char *s);
FILE *freopen(const char *name, const char *mode, FILE *fp);


# 1 "HELLO.C" 2


int main(int argc, char **argv)
{
    (void) argv;
    printf("Hello from guest-compiled C, argc=%d\n", argc);
    return 7;
}
