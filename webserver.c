#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>

#define DEFAULT_PORT 80
#define BUFFER_SIZE 1024
#define STATIC_DIR "./static"

void serve_file(int client_fd, const char *file_path);

int main(int argc, char *argv[]){
    char buffer[BUFFER_SIZE];

    // Create socket
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket failed...");
        exit(EXIT_FAILURE);
    }
    printf("Socket Created\n");

    // Set up server address
    struct sockaddr_in server_adder;
    int server_addrlen = sizeof(server_adder);
    server_adder.sin_family = AF_INET;
    server_adder.sin_port = htons(DEFAULT_PORT);
    server_adder.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the socket
    if (bind(sock_fd, (struct sockaddr *)&server_adder, server_addrlen) != 0) {
        perror("Binding failed...");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(sock_fd, SOMAXCONN) != 0) {
        perror("Listen failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d...\n\n", DEFAULT_PORT);

    while (1) {
        // Accept incoming connections
        int newsockfd = accept(sock_fd, (struct sockaddr *)&server_adder, 
                               (socklen_t *)&server_addrlen);
        if (newsockfd < 0) {
            perror("webserver (accept)");
            continue;
        }
        printf("Connection accepted\n");

        // Read the HTTP request
        int valread = read(newsockfd, buffer, BUFFER_SIZE - 1);
        if (valread < 0) {
            perror("webserver (read)");
            close(newsockfd);
            continue;
        }
        buffer[valread] = '\0';

        // Check if request is for /static
        if (strncmp(buffer, "GET /static/", 12) == 0) {
            char file_path[BUFFER_SIZE];
            sscanf(buffer, "GET /static/%s", file_path);

            // Build the full path to the static directory
            char full_path[BUFFER_SIZE];
            snprintf(full_path, sizeof(full_path), "%s/%s", STATIC_DIR, file_path);

            // Serve the requested file
            serve_file(newsockfd, full_path);
        } else {
            // If not a valid /static request, respond with a 404
            char *not_found_resp = "HTTP/1.0 404 Not Found\r\n"
                                   "Content-Type: text/plain\r\n\r\n"
                                   "404 - File Not Found";
            write(newsockfd, not_found_resp, strlen(not_found_resp));
        }

        close(newsockfd);
    }

    close(sock_fd);
    return 0;
}

void serve_file(int client_fd, const char *file_path) {
    // Check if file exists and can be opened
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        perror("File not found or cannot open");
        char *not_found_resp = "HTTP/1.0 404 Not Found\r\n"
                               "Content-Type: text/plain\r\n\r\n"
                               "404 - File Not Found";
        write(client_fd, not_found_resp, strlen(not_found_resp));
        return;
    }

    // Determine the file type for Content-Type header
    const char *content_type = "application/octet-stream";
    if (strstr(file_path, ".png")) {
        content_type = "image/png";
    } else if (strstr(file_path, ".jpg") || strstr(file_path, ".jpeg")) {
        content_type = "image/jpeg";
    } else if (strstr(file_path, ".gif")) {
        content_type = "image/gif";
    } else if (strstr(file_path, ".html")) {
        content_type = "text/html";
    }

    // Send HTTP headers
    char header[BUFFER_SIZE];
    snprintf(header, sizeof(header), "HTTP/1.0 200 OK\r\n"
                                     "Content-Type: %s\r\n"
                                     "Content-Disposition: inline; filename=\"%s\"\r\n"
                                     "Connection: close\r\n\r\n", 
                                     content_type, file_path);
    write(client_fd, header, strlen(header));

    // Send file contents in chunks
    char file_buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), file)) > 0) {
        write(client_fd, file_buffer, bytes_read);
    }

    fclose(file);
}
