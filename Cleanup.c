#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#define BUF_SIZE 256

// Define message structure
struct message {
    long mtype; // Message type
    int plane_id;
    int plane_type;
    int num_seats;
    int num_cargo_items;
    float avg_cargo_weight;
    int departure_airport;
    int arrival_airport;
    int total_weight;
    int airport_type;
    int cleanup_flag;
};

// Function to read user input safely
char read_input() {
    char input;
    printf("Do you want the Air Traffic Control System to terminate? (Y for Yes, N for No): ");
    scanf(" %c", &input);
    return input;
}

// Main function for cleanup process
int main() {
    // Create message queue
    key_t key = ftok(".", 'A'); // Generate key for the message queue
    int msgid = msgget(key, 0666); // Create or access the existing message queue
    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    
    // Prompt the user to check whether the system should terminate
    while (1) {
        char user_input = read_input();
        if (user_input == 'Y' || user_input == 'y') {
            // Send termination request to the air traffic controller
            struct message msg;
            msg.mtype = 500; // Message type for termination request
            
            if (msgsnd(msgid, &msg, sizeof(msg), 0) == -1) {
                perror("msgsnd");
                exit(EXIT_FAILURE);
            }
            printf("Termination request sent to the air traffic controller.\n");
            
            // The cleanup process has sent the termination request; it can now terminate itself
            break;
        } else if (user_input == 'N' || user_input == 'n') {
            // Continue checking for termination request
            continue;
        } else {
            // Invalid input; prompt the user again
            printf("Invalid input. Please enter 'Y' for Yes or 'N' for No.\n");
        }
    }
    
    // Cleanup process can now exit
    return 0;
}