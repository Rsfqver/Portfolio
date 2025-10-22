#include <stdio.h>
#include <stdlib.h>

int main() {
    int choice;

    while (1) {
        printf("1. Start Chat\n");
        printf("2. View Chat Logs\n");
        printf("3. Help\n");
        printf("4. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar(); // Consume newline character left by scanf

        if (choice == 1) {
            system("./client");
        } else if (choice == 2) {
            system("cat /var/log/chat_server.log");
        } else if (choice == 3) {
            system("less help.txt"); // Assuming help.txt is the help file
        } else if (choice == 4) {
            printf("Exiting...\n");
            exit(0);
        } else {
            printf("Invalid choice, please try again.\n");
        }
    }

    return 0;
}

