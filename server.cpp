#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

void die(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}
int main(){
    int fd = socket(AF_INET, SOCK_STREAM,0);
    //step2 = socket options
    int val = 1;
    setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val));
    
    struct sockaddr_in addr ={};
    addr.sin_family = AF_INET;// internet address family (ipv4 or ipv6 or unix)
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(0);  // wildcard address(host to network long);

    int rv = bind(fd,(const struct sockaddr*)&addr, sizeof(addr));
    // rv means return value
    if( rv) {
        die("bind failed");
    }

    //step 3 listen

    rv = listen(fd, SOMAXCONN);
    if(rv) {
        die("listen failed");
    }
    // read and write to the socket
    printf("Listening on port 1234...\n");
    while(true){
        //accept;
        struct sockaddr_in client_addr = {};
        socklen_t client_addr_len = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if(connfd < 0) {
            die("accept failed");
        }
        // do something
        //task1 : read from the socket
        while(true){
            
            char buffer[1024] = {};
            ssize_t n = read(connfd, buffer, sizeof(buffer) - 1);
            if(n > 0){
                buffer[n] = '\0'; // null-terminate the string
                printf("Received: %s\n", buffer);
                //echo back to the client
                ssize_t bytes_written = write(connfd, buffer, n);
                if(bytes_written < 0) {
                    perror("write failed");
                    break; // exit the loop on write error
                } else if(bytes_written != n) {
                    fprintf(stderr, "Warning: only wrote %zd of %zd bytes\n", bytes_written, n);
                }

            } else if(n < 0) {
                perror("read failed");
                break; // exit the loop on read error
            } else {
                printf("Client disconnected.\n");
                break; // exit the loop on client disconnect
            }
        }
        // char buffer[1024];
        // ssize_t n = read(connfd, buffer, sizeof(buffer));
        // if(n > 0) {
            // process the request
        //     ssize_t bytes_written = write(connfd, buffer, n);
        //     if(bytes_written < 0) {
        //         perror("write failed");
        //     } else if(bytes_written != n) {
        //         fprintf(stderr, "Warning: only wrote %zd of %zd bytes\n", bytes_written, n);
        //     }
        // }
        // else if(n < 0) {
        //     die("read failed");
        // }
        // task2 : write to the socket
        // For simplicity, we just echo back what we read
        // close the connection
        close(connfd);

    }

    return 0;
}
