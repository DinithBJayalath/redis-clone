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

static int32_t write_all(int fd, const uint8_t *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t read_full(int fd, uint8_t *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t send_req(int fd, const uint8_t *text, size_t len) {
    if (len > K_MAX_MSG) {
        msg("Too long");
        return -1;
    }
    std::vector<uint8_t> wbuf = {};
    buf_append(wbuf, (const uint8_t *)&len, 4);
    buf_append(wbuf, text, len);
    return write_all(fd, wbuf.data(), wbuf.size());
}

static int32_t read_res(int fd) {
    std::vector<uint8_t> rbuf = {};
    rbuf.resize(4);
    errno = 0;
    int32_t err = read_full(fd, &rbuf[0], rbuf.size()); 
    if (err) {
        if (errno == 0) {
            msg("EOF");
        }
        else {
            msg("read() error");
        }
        return err;
    }
    uint32_t len = 0;
    memcpy(&len, rbuf.data(), 4);
    if (len > K_MAX_MSG) {
        msg("Too long");
        return -1;
    }
    rbuf.resize(4+len);
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }
    printf("len:%u data:%.*s\n", len, len < 100 ? len : 100, &rbuf[4]);
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
    const size_t k_max_msg = 32 << 20;
    std::vector<std::string> query_list = {
        "Hello1", "Hello2", "Hello3",
        std::string(k_max_msg, 'z'),
        "Hello5"
    };
    for (const std::string &s : query_list) {
        int32_t err = send_req(client_fd, (uint8_t *)s.data(), s.size());
        if (err) {
            goto L_DONE;
        }   
    }
    for (int i = 0; i < query_list.size(); ++i) {
        int32_t err = read_res(client_fd);
        if (err) {
            goto L_DONE;
        }
    }
    L_DONE:
        close(client_fd);
        return 0;
}