#include <stdio.h>
#include <string.h>

struct Person {
    char name[50];
    int age;
};

int main(void) {
    int arr[3] = {1, 2, 3};
    int *ptr = arr;
    for (int i = 0; i < 3; i++) {
        printf("%d\n", *(ptr + i));
    }

    struct Person p;
    strcpy(p.name, "Alice");
    p.age = 25;
    printf("%s is %d years old\n", p.name, p.age);

    FILE *file = fopen("output.txt", "w");
    if (file != NULL) {
        fprintf(file, "Text written by advanced-c.c\n");
        fclose(file);
        printf("Wrote to output.txt\n");
    }
    return 0;
}