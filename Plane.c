#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <string.h>

#define MAX_PASS_SEATS 10
#define MAX_CARGO_ITEMS 100
#define MAX_BODY_WEIGHT 100
#define MAX_LUGGAGE_WEIGHT 25
#define MAX_AIRPORT_NUM 10
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
int read_input(const char *prompt) {
    int input;
    printf("%s", prompt);
    if (scanf("%d", &input) != 1) {
        fprintf(stderr, "Invalid input. Exiting...\n");
        exit(EXIT_FAILURE);
    }
    return input;
}

// Function to create a pipe and handle errors
void create_pipe(int *pipe_fd) {
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
}

// Function to prompt the user for input and handle errors
void prompt_user(const char *prompt, int *i) {
    printf("%s", prompt);
    if (scanf("%d", i) != 1) {
        fprintf(stderr, "Failed to read input. Exiting...\n");
        exit(EXIT_FAILURE);
    }
}


// Main function
int main() {
    int plane_id, plane_type, num_seats = 0, num_cargo_items = 0;
    float total_weight = 0.0, avg_cargo_weight = 0.0;
    int departure_airport, arrival_airport;

    int luggage_weights[MAX_PASS_SEATS]; // Store luggage weights for all passengers
    int body_weights[MAX_PASS_SEATS]; // Store body weights for all passengers

    // Create a message queue
    key_t key = ftok(".", 'A'); // Generate a key for the message queue
    int msgid = msgget(key, 0666); // Create or access the existing message queue
    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    // Prompt user for plane ID
    plane_id = read_input("Enter Plane ID: ");
    
    // Prompt user for type of plane (1 for passenger, 0 for cargo)
    plane_type = read_input("Enter Type of Plane (1 for passenger, 0 for cargo): ");

    if (plane_type == 1) { // Passenger plane
        // Prompt user for number of occupied seats
        num_seats = read_input("Enter Number of Occupied Seats: ");

        // Create a pipe for each passenger process
        int passenger_pipes[num_seats][2];
        pid_t pid;

        for (int i = 0; i < num_seats; i++) {
            create_pipe(passenger_pipes[i]);

            pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (pid == 0) { // Child process (passenger)
                close(passenger_pipes[i][0]); // Close unused read end

                int luggage_weight;
                prompt_user("Enter Weight of Your Luggage: ", &luggage_weight);
                write(passenger_pipes[i][1], &luggage_weight, sizeof(luggage_weight));

                int body_weight;
                prompt_user("Enter Your Body Weight: ", &body_weight);
                write(passenger_pipes[i][1], &body_weight, sizeof(body_weight));

                close(passenger_pipes[i][1]); // Close write end
                exit(EXIT_SUCCESS);
            } else { // Parent process (plane)
                close(passenger_pipes[i][1]); // Close unused write end

                // Read luggage and body weight from pipe
                if (read(passenger_pipes[i][0], &luggage_weights[i], sizeof(luggage_weights[i])) == -1) {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
                

                if (read(passenger_pipes[i][0], &body_weights[i], sizeof(body_weights[i])) == -1) {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
                wait(NULL);
                printf("Passenger %d luggage weight: %d\n", i + 1, luggage_weights[i]);
                printf("Passenger %d body weight: %d\n", i + 1, body_weights[i]);
                close(passenger_pipes[i][0]); // Close read end
                
            }
        }

        // Calculate total weight for passenger plane
        // Adding sum of all luggage weights
        for (int i = 0; i < num_seats; i++) {
            total_weight += luggage_weights[i];
        }
        // Adding sum of all body weights
        for (int i = 0; i < num_seats; i++) {
            total_weight += body_weights[i];
        }
        // Adding weights of all crew members
        total_weight += 7 * 75; // Crew members' weight (2 pilots + 5 flight attendants)
        
    } else { // Cargo plane
        // Prompt user for number of cargo items and average weight
        num_cargo_items = read_input("Enter Number of Cargo Items: ");
        avg_cargo_weight = read_input("Enter Average Weight of Cargo Items: ");
        
        // Calculate total weight for cargo plane
        total_weight += num_cargo_items * avg_cargo_weight;
        total_weight += 2 * 75; // Crew members' weight (2 pilots)
    }
    
    // Prompt user for departure and arrival airports
    departure_airport = read_input("Enter Airport Number for Departure: ");
    arrival_airport = read_input("Enter Airport Number for Arrival: ");
    
    // Prepare and send message to the air traffic controller
    struct message msg;
    msg.mtype = plane_id; // Message type for departure request
    msg.plane_id = plane_id;
    msg.plane_type = plane_type;
    msg.num_seats = num_seats;
    msg.num_cargo_items = num_cargo_items;
    msg.avg_cargo_weight = avg_cargo_weight;
    msg.departure_airport = departure_airport;
    msg.arrival_airport = arrival_airport;
    msg.total_weight = total_weight;
    msg.airport_type = 0;
    if (msgsnd(msgid, &msg, sizeof(msg), 0) == -1) {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }
    
    // Receive confirmation from air traffic controller for boarding/loading and takeoff
    if (msgrcv(msgid, &msg, sizeof(msg), 1000+msg.mtype, 0) == -1) {
        perror("msgrcv");
        exit(EXIT_FAILURE);
    }
    // Simulate journey duration
    printf("Plane is departing from Airport %d to Airport %d...\n", departure_airport, arrival_airport);
    sleep(30); // Simulate journey duration

    msg.mtype = plane_id; // Message type for departure request
    msg.plane_id = plane_id;
    msg.plane_type = plane_type;
    msg.num_seats = num_seats;
    msg.num_cargo_items = num_cargo_items;
    msg.avg_cargo_weight = avg_cargo_weight;
    msg.departure_airport = departure_airport;
    msg.arrival_airport = arrival_airport;
    msg.total_weight = total_weight;
    msg.airport_type = 1;
    if (msgsnd(msgid, &msg, sizeof(msg), 0) == -1) {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }
    
    // Receive confirmation from air traffic controller for deboarding/unloading process
    if (msgrcv(msgid, &msg, sizeof(msg), 1000+msg.mtype, 0) == -1) {
        perror("msgrcv");
        exit(EXIT_FAILURE);
    }
    
    // Simulate deboarding/unloading process
    printf("Plane has arrived at Airport %d.\n", arrival_airport);
    printf("Plane is deboarding/unloading...\n");
    
    // Display successful travel message
    printf("Plane %d has successfully traveled from Airport %d to Airport %d!\n", plane_id, departure_airport, arrival_airport);

    return 0;
}