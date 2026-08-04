#include <iostream>
#include <cassert>
#include "helper.h"
#include <sys/socket.h>
#include <netinet/ip.h>
#include <vector>
#include <poll.h>
#include <fcntl.h>

struct Conn {
    int fd = -1;
    bool want_read = false;
    bool want_write = false;
    bool want_close = false;
    std::vector<uint8_t> incoming;
    std::vector<uint8_t> outgoing;
};

static void fd_set_nb(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

static bool try_one_request(Conn *conn) {
    if (conn->incoming.size() < 4) {
        return false;
    }
    uint32_t len = 0;
    memcpy(&len, conn->incoming.data(), 4);
    if (len > K_MAX_MSG) {
        conn->want_close = true;
        return false;
    }
    if (4 + len > conn->incoming.size()) {
        return false;
    }
    const uint8_t *request = &conn->incoming[4];
    buf_append(conn->outgoing, (const uint8_t *)&len, 4);
    buf_append(conn->outgoing, request, len);
    buf_consume(conn->incoming, 4 + len);
    return true;
}

static Conn *handle_accept(int fd) {
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int client_fd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
    if (client_fd < 0) {return NULL;}
    fd_set_nb(client_fd);
    Conn *conn = new Conn();
    conn->fd = client_fd;
    conn->want_read = true;
    return conn;
}

static void handle_read(Conn *conn) {
    uint8_t buf[64*1024] = {};
    int rv = read(conn->fd, buf, (size_t)buf);
    if (rv <= 0) {
        conn->want_close = true;
        return;
    }
    buf_append(conn->incoming, buf, (size_t)buf);
    while (try_one_request(conn)) {}
    if (conn->outgoing.size() > 0) {
        conn->want_read = false;
        conn->want_write = true;
        return handle_write(conn);
    }
}

static void handle_write(Conn *conn) {
    assert(conn->outgoing.size() > 0);
    int rv = write(conn->fd, conn->outgoing.data(), conn->outgoing.size());
    if (rv < 0 && errno == EAGAIN) {
        return;
    }
    if (rv < 0) {
        conn->want_close = true;
        return;
    }
    buf_consume(conn->outgoing, (size_t)rv);
    if (conn->outgoing.size() == 0) {
        conn->want_write = false;
        conn->want_read = true;
    }
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
    std::vector<Conn *> fd2conn;
    std::vector<struct pollfd> poll_args;
    while (true) {
        poll_args.clear();
        struct pollfd pfd = {server_fd, POLLIN, 0};
        poll_args.push_back(pfd);
        for (Conn *conn : fd2conn) {
            if (!conn) {
                continue;
            }
            struct pollfd pfd = {conn->fd, POLLERR, 0};
            if (conn->want_read) {
                pfd.events |= POLLIN;
            }
            if (conn->want_write) {
                pfd.events |= POLLOUT;
            }
            poll_args.push_back(pfd);
        }
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), -1);
        if (rv < 0 && errno == EINTR) {continue;}
        if (rv < 0) {die("poll()");}
        if (poll_args[0].revents) {
            if (Conn *conn = handle_accept(server_fd)) {
                if (fd2conn.size() <= conn->fd) {
                    fd2conn.resize(conn->fd+1);
                }
                fd2conn[conn->fd] = conn;
            }
        }
        for (size_t i = 1; i < poll_args.size(); ++i) {
            uint32_t ready = poll_args[i].revents;
            Conn *conn = fd2conn[poll_args[i].fd];
            if (ready & POLLIN) {
                handle_read(conn);
            }
            if (ready & POLLOUT) {
                handle_write(conn);
            }
            if ((ready & POLLERR) || conn->want_close) {
                (void)close(conn->fd);
                fd2conn[conn->fd] = NULL;
                delete(conn);
            }
        }
    }

}