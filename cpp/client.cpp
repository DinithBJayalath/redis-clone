#include <iostream>
#include <sys/socket.h>
#include "helper.h"
#include <unistd.h>

static int32_t query(int fd, const char *text) {
    uint32_t len = (uint32_t)strlen(text);
    if (len > K_MAX_MSG) {
        msg("Too long");
        return -1;
    }
    char wbuf[4 + K_MAX_MSG] = {};
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], text, len);
    if (uint32_t err = write_full(fd, wbuf, 4 + len)) {
        return err;
    }
    char rbuf[4 +K_MAX_MSG] = {};
    errno = 0;
    uint32_t err = read_all(fd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }
    memcpy(&len, rbuf, 4);
    if (len > K_MAX_MSG) {
        msg("Too long to read");
        return -1;
    }
    err = read_all(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }
    printf("Server: %.*s\n", len, &rbuf[4]);
    return 0;
}

int main(int argc, char* argv[]) {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {die("Socket()");}
    struct sockaddr_in client_addr = {};
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = ntohs(1234);
    client_addr.sin_addr.s_addr = ntohl(0);
    int rv = connect(client_fd, (struct sockaddr *)&client_addr, sizeof(client_addr));
    if (rv) {die("connect()");}
    uint32_t err = query(client_fd, "hello1");
    if (err) {
        goto L_DONE;
    }
    err = query(client_fd, "hello2");
    if (err) {
        goto L_DONE;
    }
    L_DONE:
        close(client_fd);
        return 0;
}