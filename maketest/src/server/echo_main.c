#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 30

int main(){
    int server_fd, new_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    fd_set read_fds;
    fd_set master_fds;
    int fdmax;      // maximum file descriptor number
    
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;               // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;       // Accept connections from any IP address
    server_addr.sin_port = htons(PORT);             // Port number

    if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    FD_ZERO(&master_fds);
    FD_ZERO(&read_fds);

    FD_SET(server_fd, &master_fds);
    fdmax = server_fd;  // Initialize fdmax to the server socket

    printf("Server is listening on port %d\n", PORT);

    while(1){
        read_fds = master_fds;  // Copy master_fds to read_fds for select()

        if(select(fdmax + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Select failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        for(int i = 0; i <= fdmax; i++) {
            if(FD_ISSET(i, &read_fds)) {        // hay una actividad en el socket i

                if(i == server_fd) {            // manejar nuevas conexiones en el socket de listen
                    // New connection
                    if((new_socket = accept(server_fd, NULL, NULL)) < 0) {
                        perror("Accept failed");
                        continue;
                    }
                    FD_SET(new_socket, &master_fds);
                    if(new_socket > fdmax) {
                        fdmax = new_socket;  // Update fdmax
                    }
                    printf("New connection on socket %d\n", new_socket);
                } else {
                    // Data from an existing client
                    ssize_t bytes_read = read(i, buffer, sizeof(buffer));
                    if(bytes_read <= 0) {
                        // Connection closed or error
                        close(i);
                        FD_CLR(i, &master_fds);
                        printf("Socket %d closed\n", i);
                    } else {
                        buffer[bytes_read] = '\0';  // Null-terminate the string
                        printf("Received from socket %d: %s\n", i, buffer);
                        if(write(i, buffer, bytes_read) < 0) {
                            perror("Write failed");
                        } else {
                            printf("Echoed back to socket %d: %s\n", i, buffer);
                        }
                    }
                }
            }
        }
    }

}