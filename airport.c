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

// Define a runway structure
struct runway {
    int load_capacity;
    pthread_mutex_t mutex;
    int engaged; // Flag to track runway engagement
};
int planes_in_air;
struct runway *runways;
// Structure to represent airport data
struct airport_data {
    int airport_num;
    int num_runways;
    struct runway *runways;
    int msgid; // Message queue ID
    int plane_id;
    int plane_type;
    int num_seats;
    int num_cargo_items;
    float avg_cargo_weight;
    int departure_airport;
    int arrival_airport;
    int total_weight;
    int airport_type;
};

// Function to read user input safely
int read_input(const char *prompt) {
    int input;
    printf("%s", prompt);
    scanf("%d", &input);
    return input;
}

int max(int a, int b){
	if(a>b){
		return a;
	}
	else{
		return b;
	}
	
}

// Best-fit runway selection
int find_best_fit_runway(struct airport_data *airport, float total_weight, int *backup_runway) {
    int assigned_runway = -1;
    float min_diff = 15000; // Start with max possible difference
    *backup_runway = 0;
    int max_capacity = 0;
    // Search through runways
    
    for (int i = 0; i < airport->num_runways; i++) {
        pthread_mutex_lock(&runways[i].mutex);
        max_capacity = max(max_capacity,runways[i].load_capacity);
        if (!runways[i].engaged){
            float diff = runways[i].load_capacity - total_weight;
            if (diff >= 0 && diff < min_diff) {
                assigned_runway = i;
                min_diff = diff;
            }
        }
        pthread_mutex_unlock(&runways[i].mutex);
    }

    // Check backup runway if necessary
    if (assigned_runway == -1 && total_weight > max_capacity) {
        *backup_runway = 1;
    }
    
    return assigned_runway;
}



// Handle plane departure
void *handle_departure(void* arg)
{
    struct airport_data airport = *((struct airport_data *)arg); 
    int plane_id, num_seats, plane_type, num_cargo_items, departure_airport, arrival_airport, airport_type;
    float avg_cargo_weight,total_weight;

    plane_id = airport.plane_id;
    plane_type = airport.plane_type;
    num_seats = airport.num_seats;
    num_cargo_items = airport.num_cargo_items;
    avg_cargo_weight = airport.avg_cargo_weight;
    departure_airport = airport.departure_airport;
    arrival_airport = airport.arrival_airport;
    total_weight = airport.total_weight;
    airport_type = airport.airport_type;
    while(1)
    {
        int backup_runway = 0;
        int assigned_runway = find_best_fit_runway(&airport, total_weight, &backup_runway);

        if (assigned_runway == -1 && backup_runway) {
            assigned_runway = airport.num_runways; // Use backup runway
        }

        if (assigned_runway != -1) {
            pthread_mutex_lock(&runways[assigned_runway].mutex);
            runways[assigned_runway].engaged = 1;
            pthread_mutex_unlock(&runways[assigned_runway].mutex);
            // Handle departure
            sleep(3); // Simulate boarding/loading process
            sleep(2); //simulate takeoff
            // Send confirmation message to the air traffic controller
            struct message msg;
            msg.mtype = departure_airport + 200 ; // Message type for departure confirmation
            msg.plane_id = plane_id;
            msg.plane_type = plane_type;
            msg.num_seats = num_seats;
            msg.num_cargo_items = num_cargo_items;
            msg.avg_cargo_weight = avg_cargo_weight;
            msg.departure_airport = departure_airport;
            msg.arrival_airport = arrival_airport;
            msg.total_weight = total_weight;
            if (msgsnd(airport.msgid, &msg, sizeof(msg), 0) == -1) {
                perror("msgsnd");
                exit(EXIT_FAILURE);
            }
            printf("Plane %d has completed boarding/loading and taken off from Runway No. %d of Airport No. %d.\n",
                plane_id, assigned_runway + 1, airport.airport_num);
            // Adjust load capacity
            pthread_mutex_lock(&runways[assigned_runway].mutex);
            runways[assigned_runway].engaged = 0;
            pthread_mutex_unlock(&runways[assigned_runway].mutex);
            break;
        }
        else
        {
            continue;
        }
    }
    planes_in_air++;
    return NULL;
}

// Handle plane arrival
void *handle_arrival(void *arg)
{
    int plane_id, num_seats, num_cargo_items, plane_type, departure_airport, arrival_airport, airport_type;
    float total_weight, avg_cargo_weight;
    struct airport_data airport = *((struct airport_data *)arg); 
    plane_id = airport.plane_id;
    plane_type = airport.plane_type;
    num_seats = airport.num_seats;
    num_cargo_items = airport.num_cargo_items;
    avg_cargo_weight = airport.avg_cargo_weight;
    departure_airport = airport.departure_airport;
    arrival_airport = airport.arrival_airport;
    total_weight = airport.total_weight;
    airport_type = airport.airport_type;
    while(1)
    {
        int backup_runway = 0;
        int assigned_runway = find_best_fit_runway(&airport, total_weight, &backup_runway);

        if (assigned_runway == -1 && backup_runway)
        {
            assigned_runway = airport.num_runways; // Use backup runway
        }

        if (assigned_runway != -1) {
            pthread_mutex_lock(&runways[assigned_runway].mutex);
            runways[assigned_runway].engaged = 1;
            pthread_mutex_unlock(&runways[assigned_runway].mutex);
            // Handle arrival
            sleep(2); // Simulate landing process
            sleep(3); // Simulate deboarding/unloading process
            // Send confirmation message to the air traffic controller
            struct message msg;
            msg.mtype = arrival_airport + 200 ; // Message type for departure confirmation
            msg.plane_id = plane_id;
            msg.plane_type = plane_type;
            msg.num_seats = num_seats;
            msg.num_cargo_items = num_cargo_items;
            msg.avg_cargo_weight = avg_cargo_weight;
            msg.departure_airport = departure_airport;
            msg.arrival_airport = arrival_airport;
            msg.total_weight = total_weight;
            msg.airport_type = 1;
            if (msgsnd(airport.msgid, &msg, sizeof(msg), 0) == -1) {
                perror("msgsnd");
                exit(EXIT_FAILURE);
            }
            printf("Plane %d has landed on Runway No. %d of Airport No. %d and has completed deboarding/unloading.\n",
                plane_id, assigned_runway + 1, airport.airport_num);
            pthread_mutex_lock(&runways[assigned_runway].mutex);
            runways[assigned_runway].engaged = 0;
            pthread_mutex_unlock(&runways[assigned_runway].mutex);
            break;
        }
        else
        {
            continue;
        }
    }
    planes_in_air--;
    return NULL;
}

// Main function for the airport process
int main()
{
	planes_in_air = 0;
    // Create the message queue
    key_t key = ftok(".", 'A');
    int msgid = msgget(key, 0666);
    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    // Prompt user for the airport number
    int airport_num = read_input("Enter Airport Number: ");
    
    // Prompt user for the number of runways
    int num_runways = read_input("Enter Number of Runways (even number): ");
    if (num_runways < 1 || num_runways > 10 || num_runways % 2 != 0) {
        printf("Invalid number of runways. It should be an even number between 1 and 10.\n");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for runways
    runways = malloc((num_runways + 1) * sizeof(struct runway)); // +1 for backup runway
    if (!runways) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Prompt user for load capacities of each runway
    printf("Enter loadCapacity of Runways (give as a space separated list in a single line): ");
    for (int i = 0; i < num_runways; i++) {
        while (1) {
            int load_capacity;
            if (scanf("%d", &load_capacity) != 1) {
                printf("Invalid input. Please enter an integer.\n");
                while (getchar() != '\n'); // Clear input buffer
                continue;
            }
            if (load_capacity < 1000 || load_capacity > 12000) {
                printf("Invalid input. Load capacity should be between 1000 and 12000 kgs.\n");
                continue;
            }
            runways[i].load_capacity = load_capacity;
            break;
        }
        pthread_mutex_init(&runways[i].mutex, NULL);
        runways[i].engaged = 0;
    }

    // Add backup runway with a load capacity of 15,000 kg
    runways[num_runways].load_capacity = 15000;
    pthread_mutex_init(&runways[num_runways].mutex, NULL);
    runways[num_runways].engaged = 0;
    int cleanup_flag = 0;
    pthread_t thread_id[128];
    int i = 0;
    while(1)
    {
        if(cleanup_flag == 1)
        {
            break;
        }
        struct message msg;
        if (msgrcv(msgid, &msg, sizeof(msg), 2000+airport_num, 0) == -1) {
            if (errno != EINTR) {
                perror("msgrcv");
                exit(EXIT_FAILURE);
            }
        }
        if(msg.cleanup_flag == 1)
        {
            cleanup_flag = 1;
            continue;
        }
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
        airport_type = msg.airport_type;
        // Initialize airport data
        struct airport_data airport = {
            .airport_num = airport_num,
            .num_runways = num_runways + 1, // Including the backup runway
            .msgid = msgid,
            .plane_id = plane_id,
            .plane_type = plane_type,
            .num_seats = num_seats,
            .num_cargo_items = num_cargo_items,
            .avg_cargo_weight = avg_cargo_weight,
            .departure_airport = departure_airport,
            .arrival_airport = arrival_airport,
            .total_weight = total_weight,
            .airport_type = airport_type
        };
        if (msg.airport_type == 0)
        {
            pthread_create(&thread_id[i++], NULL, handle_departure, &airport);
        }
        else
        {
            pthread_create(&thread_id[i++], NULL, handle_arrival, &airport);
        }
        
    }
    

    // Clean up
    for (int i = 0; i < num_runways + 1; i++) {
        pthread_mutex_destroy(&runways[i].mutex);
    }
    

    struct message msg;
    msg.mtype = airport_num+200; 
    if (msgsnd(msgid, &msg, sizeof(msg), 0) == -1)
    {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }
    free(runways);
    return 0;
}