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

#define DEFAULT_PORT 80
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]){
    char buffer[BUFFER_SIZE];
    char resp[] = "HTTP/1.0 200 OK\r\n"
                  "Server: webserver-c\r\n"
                  "Content-type: text/html\r\n\r\n"
                  "<html>hello, world</html>\r\n";

    //socket creation
    int sock_fd = socket(AF_INET,SOCK_STREAM,0);
    if (sock_fd == -1){
        perror("socket failed...");
        exit(EXIT_FAILURE);
    }
    printf("Socket Created\n");
    //creating the address to bind
    struct sockaddr_in server_adder;
    int server_addrlen = sizeof(server_adder);

    server_adder.sin_family = AF_INET;
    server_adder.sin_port = htons(DEFAULT_PORT);
    server_adder.sin_addr.s_addr = htonl(INADDR_ANY);
    //bind()
    if(bind(sock_fd,(struct serveraddr *)&server_adder,server_addrlen) != 0){
        perror("Binding failed...");
        exit(EXIT_FAILURE);
    }

    if(listen(sock_fd, SOMAXCONN) != 0){
        perror("webserver");
        exit(EXIT_FAILURE);
    }

    printf("Server listening...\n\n");


    while(1) {
        // Accept incoming connections
        int newsockfd = accept(sock_fd, (struct sockaddr *)&server_adder,
                               (socklen_t *)&server_addrlen);
        if (newsockfd < 0) {
            perror("webserver (accept)");
            continue;
        }
        printf("connection accepted\n");

        // Read from the socket
        int valread = read(newsockfd, buffer, BUFFER_SIZE);
        if (valread < 0) {
            perror("webserver (read)");
            continue;
        }

        // Write to the socket
        int valwrite = write(newsockfd, resp, strlen(resp));
        if (valwrite < 0) {
            perror("webserver (write)");
            continue;
        }

        close(newsockfd);
    }

    return 0;
}