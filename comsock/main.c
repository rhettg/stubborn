#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#include "evt.h"

#include "command.h"

EVT_t evt = {0};

void fatal(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

int open_client_sock()
{
    struct sockaddr_un addr;
    int fd;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (0 != unlink("socket")) {
        if (ENOENT != errno) {
            perror("failed to remove");
            return -1;
        }
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "socket", sizeof(addr.sun_path)-1);

    if (0 != bind(fd, (struct sockaddr*)&addr, sizeof(addr))) {
        perror("failed to open socket");
        return -1;
    }

    if (0 != listen(fd, 1)) {
        perror("failed to listen");
        return -1;
    }

    return fd;
}

int run_server(int server)
{
    char buf[1024];
    int c_fd = 0;
    size_t rlen;
    fd_set set;

    char command_buf[1024];
    size_t command_len = 0;

    printf("starting server\n");

    while (1) {
        FD_ZERO(&set);
        if (0 == c_fd) {
            FD_SET(server, &set);
        } else {
            FD_SET(c_fd, &set);
        }

        printf("waiting for io\n");
        int ret = select(FD_SETSIZE, &set, NULL, NULL, NULL);
        if (0 > ret) {
            perror("failed to select");
            continue;
        }
        printf("io ready\n");

        if (0 != FD_ISSET(server, &set)) {
            printf("accepting to connection\n");
            c_fd = accept(server, 0, 0);
            if (c_fd < 0) {
                fprintf(stderr, "failed to accept: %d\n", errno);
                continue;
            }
        }

        if (0 != FD_ISSET(c_fd, &set)) {
            printf("reading request from client\n");
            rlen = read(c_fd, buf, sizeof(buf));
            if (rlen < 0 && EAGAIN != errno) {
                fprintf(stderr, "failed reading: %d\n", errno);
                rlen = 0;
            }

            if (0 < rlen) {
                buf[rlen] = 0;
                printf("RX: %s\n", buf);
                int rfcd = feed_command_data(command_buf, &command_len, buf, rlen);
                
            } else {
                printf("closing client\n");
                close(c_fd);
                c_fd = 0;
            }
        }
    }

    return 0;
}

int main(int argc, char const *argv[])
{
    int fd;

    EVT_init(&evt);

    fd = open_client_sock();
    if (0 > fd) {
        exit(1);
    }

    if (0 != run_server(fd)) {
        fprintf(stderr, "server failed\n");
        exit(1);
    }

    return 0;
}