#include <stdio.h>

int main(int argc, char **argv)
{
    (void) argv;
    printf("Hello from guest-compiled C, argc=%d\n", argc);
    return 7;
}
