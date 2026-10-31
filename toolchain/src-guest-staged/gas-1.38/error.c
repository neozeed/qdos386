#include <stdio.h>

void error(char *p)
{
fprintf(stderr,"%s",p);
exit (-1);
}
