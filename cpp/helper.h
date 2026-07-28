#include <cstdio>
#include <cstdlib>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void die(const char *msg) {
    perror(msg);        // Prints "listen(): <description of errno>" to stderr
    exit(EXIT_FAILURE); // Halts the program immediately
}

void msg(const char *msg) {
    std::cout << msg << std::endl;
}