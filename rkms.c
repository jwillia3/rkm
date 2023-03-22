#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

int             fd;
double          sx = 1.0;
double          sy = 1.0;

void
loop(void)
{
    Display     *display = XOpenDisplay(0);
    int         ignore;

    if(!display)
        puts("cannot connect to X server"),
        exit(1);
    if (!XTestQueryExtension(display, &ignore, &ignore, &ignore, &ignore))
        puts("XTest extension not available"),
        exit(1);

    char        cmd[1024];
    ssize_t     sz;

    while ((sz = recv(fd, cmd, sizeof cmd, MSG_WAITFORONE)) >= 0) {
        int     code, button, x, y;

        if (sz == sizeof cmd)
            continue;

        cmd[sz] = 0;

        if (2 == sscanf(cmd, "scale %d %d\n", &x, &y)) {
            int     screen = DefaultScreen(display);
            sx = (double) DisplayWidth(display, screen) / x;
            sy = (double) DisplayHeight(display, screen) / y;
        }
        else if (1 == sscanf(cmd, "keydown %d\n", &code))
            XTestFakeKeyEvent(display, code, true, 0);
        else if (1 == sscanf(cmd, "keyup %d\n", &code))
            XTestFakeKeyEvent(display, code, false, 0);
        else if (1 == sscanf(cmd, "buttondown %d\n", &button))
            XTestFakeButtonEvent(display, button, true, 0);
        else if (1 == sscanf(cmd, "buttonup %d\n", &button))
            XTestFakeButtonEvent(display, button, false, 0);
        else if (2 == sscanf(cmd, "mousemove %d %d\n", &x, &y))
            XTestFakeMotionEvent(display, -1, x * sx, y * sy, 0);
        else
            printf("rkms: message not understood: %s", cmd);

        XFlush(display);
    }

    close(fd);
}

void
bindserver(int port)
{
    struct sockaddr_in  addr = {
        .sin_family = PF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port),
    };

    if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0 ||
        bind(fd, (struct sockaddr*) &addr, sizeof addr))
    {
        puts("rkms: cannot bind server."),
        exit(1);
    }
}

int main(int argc, char **argv)
{
    if (!argv[1])
        help:
        puts("usage: rkms PORT\n"
             "Remote keyboard and mouse server"),
        exit(1);

    char    *portstr = argv[1];
    int     port = strtol(portstr, &portstr, 0);
    if (*portstr || !port)
        goto help;

    bindserver(port);
    loop();
}
