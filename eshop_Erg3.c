#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 5

typedef struct {
    char description[50];
    float price;
    int item_count;
    int request_count;
    int unserved_customers;
} Product;

int total_orders = 0;
int successful_orders = 0;
int failed_orders = 0;
float total_revenue = 0.0;

void initialize_catalog(Product catalog[]) {
    for (int i = 0; i < 20; i++) {
        sprintf(catalog[i].description, "Product %d", i);
        catalog[i].price = (float)(10 + i * 5);
        catalog[i].item_count = 2;
        catalog[i].request_count = 0;
        catalog[i].unserved_customers = 0;
    }
}

void handle_client(int client_socket, Product catalog[]) {
    char buffer[100];
    for (int j = 0; j < 10; j++) {
        int product_id;
        recv(client_socket, &product_id, sizeof(product_id), 0);
        
        total_orders++;
        catalog[product_id].request_count++;
        
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
        printf("Client: %s\n", buffer);
        sleep(1);
    }
    close(client_socket);
}

int main() {
    Product catalog[20];
    initialize_catalog(catalog);

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    pid_t pids[MAX_CLIENTS];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        pids[i] = fork();

        if (pids[i] == -1) {
            perror("fork failed");
            exit(1);
        }

        if (pids[i] == 0) {  // Child process (client)
            close(server_fd);
            
            int sock = 0;
            struct sockaddr_in serv_addr;
            
            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                printf("\n Socket creation error \n");
                return -1;
            }

            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT);

            if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
                printf("\nInvalid address/ Address not supported \n");
                return -1;
            }

            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                printf("\nConnection Failed \n");
                return -1;
            }

            srand(time(NULL) + i);

            for (int j = 0; j < 10; j++) {
                int product_id = rand() % 20;
                send(sock, &product_id, sizeof(product_id), 0);

                char response[100];
                recv(sock, response, sizeof(response), 0);
                printf("Client %d: %s\n", i, response);

                sleep(1);
            }

            close(sock);
            exit(0);
        }
    }

    // Parent process (server)
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        handle_client(new_socket, catalog);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < MAX_CLIENTS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    // Print summary report
    printf("\nSummary Report:\n");
    for (int i = 0; i < 20; i++) {
        printf("%s - Remaining: %d , Requests: %d, Unserved Customers: %d \n",
               catalog[i].description, catalog[i].item_count, catalog[i].request_count, catalog[i].unserved_customers);
    }

    // Print overall report
    printf("\nOverall Report:\n");
    printf("Total Orders: %d\n", total_orders);
    printf("Successful Orders: %d\n", successful_orders);
    printf("Failed Orders: %d\n", failed_orders);
    printf("Total Revenue: %.2f\n", total_revenue);

    close(server_fd);
    return 0;
}

