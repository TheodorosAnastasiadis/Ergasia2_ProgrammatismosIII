//Αυτο ειναι το προγραμμα με τα sockets Οχι το διορθωμενο προηγουμενο
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define PORT 8080
#define MAX_CLIENTS 5
#define PRODUCTS 20
#define ORDERS_PER_CLIENT 10

typedef struct {
    char description[50];
    float price;
    int item_count;
    int request_count;
    int unserved_customers;
} Product;

Product catalog[PRODUCTS];
int total_orders = 0;
int successful_orders = 0;
int failed_orders = 0;
float total_revenue = 0.0;

void initialize_catalog() {
    for (int i = 0; i < PRODUCTS; i++) {
        sprintf(catalog[i].description, "Product %d", i);
        catalog[i].price = (float)(10 + i * 5);
        catalog[i].item_count = 2;
        catalog[i].request_count = 0;
        catalog[i].unserved_customers = 0;
    }
}

void client_process(int client_id, const struct sockaddr_in *server_addr) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL) + client_id);
    for (int j = 0; j < ORDERS_PER_CLIENT; j++) {
        int product_id = rand() % PRODUCTS;
        send(sock, &product_id, sizeof(product_id), 0);

        char response[100];
        recv(sock, response, sizeof(response), 0);
        printf("Client %d: %s\n", client_id, response);

        sleep(1);
    }
    close(sock);
    exit(0);
}

void handle_order(int client_socket) {
    int product_id;
    recv(client_socket, &product_id, sizeof(product_id), 0);

    total_orders++;
    catalog[product_id].request_count++;

    char buffer[100];
    if (catalog[product_id].item_count > 0) {
        catalog[product_id].item_count--;
        snprintf(buffer, sizeof(buffer), "Order for Product %d (Price: %.2f) successful!", product_id, catalog[product_id].price);
        successful_orders++;
        total_revenue += catalog[product_id].price;
    } else {
        snprintf(buffer, sizeof(buffer), "Product %d out of stock.", product_id);
        failed_orders++;
        catalog[product_id].unserved_customers++;
    }

    send(client_socket, buffer, strlen(buffer) + 1, 0);
}

int main() {
    initialize_catalog();

    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    pid_t pids[MAX_CLIENTS];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        pids[i] = fork();

        if (pids[i] == -1) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }

        if (pids[i] == 0) {
            close(server_fd);
            client_process(i, &address);
        }
    }

    for (int i = 0; i < MAX_CLIENTS * ORDERS_PER_CLIENT; i++) {
        int client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        handle_order(client_socket);
        close(client_socket);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < MAX_CLIENTS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    // Summary Report
    printf("\nSummary Report:\n");
    for (int i = 0; i < PRODUCTS; i++) {
        printf("%s - Remaining: %d, Requests: %d, Unserved Customers: %d\n",
               catalog[i].description, catalog[i].item_count, catalog[i].request_count, catalog[i].unserved_customers);
    }

    // Overall Report
    printf("\nOverall Report:\n");
    printf("Total Orders: %d\n", total_orders);
    printf("Successful Orders: %d\n", successful_orders);
    printf("Failed Orders: %d\n", failed_orders);
    printf("Total Revenue: %.2f\n", total_revenue);

    close(server_fd);
    return 0;
}

