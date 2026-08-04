#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

void die(const char *msg) {
    perror(msg);        // Prints "listen(): <description of errno>" to stderr
    exit(EXIT_FAILURE); // Halts the program immediately
}

void msg(const char *msg) {
    std::cout << msg << std::endl;
}

static void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len) {
    buf.insert(buf.end(), data, data+len);
}

static void buf_consume(std::vector<uint8_t> &buf, size_t len) {
    buf.erase(buf.begin(), buf.begin() + len);
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
    err = read_all(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return -1;
    }
    printf("Client: %.*s\n", len, &rbuf[4]);
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_full(fd, wbuf, 4+len);
}