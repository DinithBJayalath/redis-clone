#include <cstdio>
#include <cstdlib>

struct sockaddr_in {
    uint16_t sin_family;
    uint16_t sin_port;
    struct in_addr sin_addr;
};

struct in_addr {
    uint32_t s_addr;
};

void die(const char *msg) {
    perror(msg);        // Prints "listen(): <description of errno>" to stderr
    exit(EXIT_FAILURE); // Halts the program immediately
}

void msg(const char *msg) {
    std::cout << msg << std::endl;
    return;
}