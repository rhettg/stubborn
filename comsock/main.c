#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#include "evt.h"
#include "com.h"
#include "ci.h"

#include "command.h"

EVT_t evt = {0};
COM_t com = {0};
CI_t  ci  = {0};

void fatal(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

void debugEvent(EVT_Event_t *e)
{
    printf("Event: %d\n", e->type);
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
                int rp = parse_ci_command(&ci, buf);
                if (0 != rp) {
                    fprintf(stderr, "failed parsing: %d\n", rp);
                    if (0 > send(c_fd, "ERR\n", 4, 0)) {
                        perror("failed to report error");
                    }
                } else {
                    // It isn't OK yet actually.
                    if (0 > send(c_fd, "OK\n", 4, 0)) {
                        perror("failed to report error");
                    }
                }
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
    com.evt = &evt;
    ci.evt = &evt;

    EVT_subscribe(&evt, &debugEvent);

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
