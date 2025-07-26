#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <iostream>

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }
    
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // 127.0.0.1
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("connect");
    }

    printf("Connected to server on port 1234. Type message or quit to exit\n");

    char input[1024] = {};
    char response[1024] = {};

    while(true){
        printf("> ");
        fflush(stdout);

        // Read input from stdin
        if (!fgets(input, sizeof(input), stdin)) {
            break;// eof error
        }

        size_t len = strlen(input);
        if(len > 0 && input[len-1] == '\n') {
            input[len-1] = '\0';
            len--;
        }
        if(strcmp(input, "quit") == 0) {
            std::cout<<"Exiting client."<<std::endl;
            break; 
        }
        if(len == 0) {
            continue; // skip empty input
        }
        // sending message to server
        ssize_t bytes_written = write(fd, input, len);
        if (bytes_written < 0) {
            die("write");
        } else if ((size_t)bytes_written != len) {
            fprintf(stderr, "Warning: only wrote %zd of %zu bytes\n", bytes_written, len);
        }
        ssize_t n = read(fd, response, sizeof(response) - 1);
        if (n < 0) {
            die("read");
        } else if(n == 0) {
            std::cout << "Server closed the connection." << std::endl;
            break; // server closed connection
        }
        response[n] = '\0'; // null-terminate the string
        printf("Server response: %s\n", response);
    }
    close(fd);
    return 0;
}