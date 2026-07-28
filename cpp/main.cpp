#include <iostream>
#include <helper.h>
#include <sys/socket.h>
#include <unistd.h>

struct sockaddr_in {
    uint16_t sin_family;
    uint16_t sin_port;
    struct in_addr sin_addr;
};

struct in_addr {
    uint32_t s_addr;
};


int main(int argc, char* argv[]) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1234);
    server_addr.sin_addr.s_addr = htonl(0);
    int rv = bind(server_fd, (const sockaddr *)&server_addr, sizeof(server_addr));
    if (rv) {die("bind()");}
    rv = listen(server_fd, SOMAXCONN);
    if (rv) {die("listen()");}
    return 0;
    while (true) {
        struct sockaddr_in client_addr = {};
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {continue;}
        test_action(client_fd);
        close(client_fd);
    }
}   

static void test_action(int client_fd) {
    char rbuf[64] = {};
    ssize_t n = read(client_fd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        msg("read() error");
        return;
    }
    printf("Client: %s\n", rbuf);
    char wbuf[] = "Hello!";
    write(client_fd, wbuf, strlen(wbuf));
}