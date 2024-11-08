// server1.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void send_directory_listing(int client_socket) {
    DIR *d;
    struct dirent *dir;
    char buffer[BUFFER_SIZE];

    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            snprintf(buffer, BUFFER_SIZE, "%s\n", dir->d_name);
            write(client_socket, buffer, strlen(buffer));
        }
        closedir(d);
    } else {
        write(client_socket, "Failed to open directory\n", 25);
    }
}

void send_file(int client_socket, const char *filename) {
    FILE *file = fopen(filename, "rb");
    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    if (file == NULL) {
        write(client_socket, "File not found\n", 15);
        return;
    }

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        write(client_socket, buffer, bytes_read);
    }

    fclose(file);
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    int bytes_read;

    // Handle client requests
    while ((bytes_read = read(client_socket, buffer, BUFFER_SIZE)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Received: %s\n", buffer);

        // Respond to DIR and GET commands
        if (strncmp(buffer, "DIR", 3) == 0) {
            send_directory_listing(client_socket);
        } else if (strncmp(buffer, "GET", 3) == 0) {
            char *filename = buffer + 4; // Skip "GET " to get the filename
            send_file(client_socket, filename);
        } else {
            write(client_socket, "Invalid command\n", 16);
        }
    }

    close(client_socket);
    return NULL;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 3) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len)) >= 0) {
        pthread_t thread_id;
        int *client_sock = malloc(sizeof(int));
        *client_sock = client_socket;

        if (pthread_create(&thread_id, NULL, handle_client, client_sock) != 0) {
            perror("Thread creation failed");
            close(client_socket);
            free(client_sock);
        }
    }

    close(server_socket);
    return 0;
}