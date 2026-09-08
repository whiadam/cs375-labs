#include <stdio.h>

long factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int main(void) {
    int n;
    printf("Enter a number: ");
    scanf("%d", &n);
    printf("%d! = %ld\n", n, factorial(n));
    return 0;
}