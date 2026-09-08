#include <stdio.h>
#include <string.h>

struct Contact {
    char name[50];
    int age;
    char phone[20];
};

void addContact(void) {
    struct Contact c;
    printf("Enter name: ");
    scanf(" %49[^\n]", c.name);
    printf("Enter age: ");
    scanf("%d", &c.age);
    printf("Enter phone: ");
    scanf(" %19[^\n]", c.phone);

    FILE *file = fopen("contacts.txt", "a");
    if (file == NULL) {
        printf("Error opening file.\n");
        return;
    }
    fprintf(file, "%s,%d,%s\n", c.name, c.age, c.phone);
    fclose(file);
    printf("Contact saved!\n");
}

void viewContacts(void) {
    FILE *file = fopen("contacts.txt", "r");
    if (file == NULL) {
        printf("No contacts found.\n");
        return;
    }
    struct Contact c;
    printf("\n--- Contact List ---\n");
    while (fscanf(file, " %49[^,],%d,%19[^\n]", c.name, &c.age, c.phone) == 3) {
        printf("Name: %s | Age: %d | Phone: %s\n", c.name, c.age, c.phone);
    }
    fclose(file);
    printf("--------------------\n");
}

int main(void) {
    int choice;
    do {
        printf("\n1. Add contact\n2. View contacts\n3. Exit\nChoice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1: addContact(); break;
            case 2: viewContacts(); break;
            case 3: printf("Goodbye!\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 3);
    return 0;
}