#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

void fatal(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

int main(int argc, char const *argv[])
{
    char buf[1024];
    struct sockaddr_un addr;
    int fd, cl;
    int rl;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (0 != unlink("socket")) {
        if (ENOENT != errno) {
            fatal("failed to remove");
        }
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "socket", sizeof(addr.sun_path)-1);

    if (0 != bind(fd, (struct sockaddr*)&addr, sizeof(addr))) {
        fatal("failed to open socket");
    }

    if (0 != listen(fd, 1)) {
        fatal("failed to listen");
    }

    while (1) {
        cl = accept(fd, 0, 0);
        if (cl < 0) {
            printf("failed to accept: %d\n", errno);
            continue;
        }

        while(1) {
            rl = read(cl, buf, sizeof(buf));
            if (rl < 0 && EAGAIN != errno) {
                printf("failed reading\n");
                rl = 0;
            }

            if (rl == 0) {
                printf("closing client\n");
                close(cl);
                break;
            }

            buf[rl] = 0;
            printf("RX: %s\n", buf);
        }
    }

    return 0;
}
