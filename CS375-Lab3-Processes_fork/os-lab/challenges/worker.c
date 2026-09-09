// worker.c
// This is a small helper program that ch4 runs with exec.
// It prints its arguments and the MYVAR environment variable if it is set.
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    printf("worker: args =");
    for (int i = 1; i < argc; ++i)
        printf(" %s", argv[i]);
    printf("\n");

    const char *myvar = getenv("MYVAR");
    if (myvar)
        printf("worker: MYVAR=%s\n", myvar);
    else
        printf("worker: MYVAR not set\n");
    return 0;
}
