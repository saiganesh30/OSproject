#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

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

// Global variables
int numAirports;
int plains_in_air;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to write plane journey to file
void write_to_file(int plane_id, int departure_airport, int arrival_airport) {
    FILE *fp = fopen("AirTrafficController.txt", "a");
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fprintf(fp, "Plane %d has departed from Airport %d and will land at Airport %d.\n", plane_id, departure_airport, arrival_airport);
    fclose(fp);
}

// Function to handle plane departures and arrivals
void handle_plane(int msgid, struct message *msge) {
	struct message msg = *msge;
    // Parse message to extract plane details
    int plane_id, plane_type, departure_airport, arrival_airport, num_seats, num_cargo_items,airport_type;
    float avg_cargo_weight, total_weight;
    plane_id = msg.plane_id;
    plane_type = msg.plane_type;
    num_seats = msg.num_seats;
    num_cargo_items = msg.num_cargo_items;
    avg_cargo_weight = msg.avg_cargo_weight;
    departure_airport = msg.departure_airport;
    arrival_airport = msg.arrival_airport;
    total_weight = msg.total_weight;
    if(msg.airport_type == 0)
    {
        msg.mtype = departure_airport+2000;
        if (msgsnd(msgid, &msg, sizeof(msg), 0) == -1) {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
       msg.mtype = arrival_airport+2000;
        if (msgsnd(msgid, &msg, sizeof(msg), 0) == -1) {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        }
    }
}

// Main function for the air traffic controller process
int main()
{
    plains_in_air = 0;
    // Prompt the user for the number of airports and validate input
    printf("Enter the number of airports to be handled/managed (2-10): ");
    scanf("%d", &numAirports);
    if (numAirports < 2 || numAirports > 10) {
        printf("Invalid input. Number of airports must be between 2 and 10.\n");
        exit(EXIT_FAILURE);
    }

    // Set up the message queue
    key_t key = ftok(".", 'A'); // Generate a key
    int msgid = msgget(key, IPC_CREAT | 0666); // Create or access the existing message queue

    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    int cleanup_flag = 0;
    // Main loop to handle plane messages and listen for termination requests
    while (1)
    {
        if(cleanup_flag)
        {    
            if(plains_in_air == 0)
            {
	    struct message msg;
	    for (int i = 1; i <= numAirports; i++)
	    {
            msg.mtype = 2000+i;
            msg.cleanup_flag = 1;
            if (msgsnd(msgid, &msg, sizeof(msg), 0) == -1) {
                perror("msgsnd");
                exit(EXIT_FAILURE);
            }
	    }
	    // Wait for confirmation messages from airports
	    for (int i = 1; i <= numAirports; i++) {
		    if (msgrcv(msgid, &msg, sizeof(msg), 200+i, 0) == -1) {
		        perror("msgrcv");
		        exit(EXIT_FAILURE);
		    }
	    }

    // Perform cleanup activities
    remove("AirTrafficController.txt");

    // Terminate the air traffic controller process
    printf("Terminating the air traffic controller process.\n");
    exit(EXIT_SUCCESS);
                break;
            }
        }
        struct message msg;
        if (msgrcv(msgid, &msg, sizeof(msg), -501, 0) == -1)
        {
            if (errno != EINTR) {
                perror("msgrcv");
                exit(EXIT_FAILURE);
            }
        }
      
        if (msg.mtype == 500)
        {
            cleanup_flag = 1;
        }
        else if(msg.mtype <= 10)
        {
            handle_plane(msgid,&msg);
        }
        else if(msg.mtype > 200 && msg.mtype < 300)
        {
            if(msg.airport_type == 0)
            {
                plains_in_air++;
                msg.mtype = msg.plane_id + 1000;
                if (msgsnd(msgid, &msg, sizeof(msg), 0) == -1)
                {
                    perror("msgsnd");
                    exit(EXIT_FAILURE);
                }
                write_to_file(msg.plane_id, msg.departure_airport, msg.arrival_airport);
            }
            if(msg.airport_type == 1)
            {
                plains_in_air--;
                msg.mtype = msg.plane_id + 1000;
                if (msgsnd(msgid, &msg, sizeof(msg), 0) == -1)
                {
                    perror("msgsnd");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    
	if (msgctl(msgid, IPC_RMID, NULL) == -1)
	{
		perror("msgctl");
		exit(EXIT_FAILURE);
    	}
    return 0;
}