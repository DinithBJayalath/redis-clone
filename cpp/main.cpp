#include <iostream>
#include <cassert>
#include "helper.h"
#include <sys/socket.h>
#include <unistd.h>

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

static int32_t read_all(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {return -1;}
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_full(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {return -1;}
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

const size_t K_MAX_MSG = 4096;

static int32_t one_responce(int fd) {
    char rbuf[4+K_MAX_MSG];
    errno = 0;
    int32_t err = read_all(fd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }
    uint32_t len = 0;
    memcpy(&len, rbuf, 4);
    if (len > K_MAX_MSG) {
        msg("Too long");
        return -1;
    }
    int32_t err = read_all(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return -1;
    }
    printf("Client: %.*s\n", len, rbuf);
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_full(fd, wbuf, 4+len);
}

int main(int argc, char* argv[]) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1234);
    server_addr.sin_addr.s_addr = htonl(0);
    int rv = bind(server_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    if (rv) {die("bind()");}
    rv = listen(server_fd, SOMAXCONN);
    if (rv) {die("listen()");}
    while (true) {
        struct sockaddr_in client_addr = {};
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {continue;}
        while (true) {
            int32_t err = one_responce(client_fd);
            if (err) {
                break;
            }
        }
        close(client_fd);
    }
}