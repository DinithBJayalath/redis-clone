#include <iostream>
#include <sys/socket.h>
#include <helper.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {die("Socket()");}
    struct sockaddr_in client_addr = {};
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = ntohs(1234);
    client_addr.sin_addr.s_addr = ntohl(0);
    int rv = connect(client_fd, (struct sockaddr *)&client_addr, sizeof(client_addr));
    if (rv) {die("connect()");}
    char msg[] = "Hallo!";
    write(client_fd, msg, strlen(msg));
    char rbuf[64] = {};
    ssize_t n = read(client_fd, rbuf, sizeof(rbuf)-1);
    if (n < 0) {die("read()");}
    printf("Server: %s\n",rbuf);
    close(client_fd);
}