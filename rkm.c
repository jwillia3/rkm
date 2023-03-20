#include <limits.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

int             fd;
Display         *display;
char            *prog = "rkm";
char            hostname[PATH_MAX + 1] = "localhost";
int             port = 0;
bool            server;
char            *hostaddr;
int             hostaddrlen;
bool            verbose;
double          sx = 1.0;
double          sy = 1.0;
KeyCode         breaker;
sigjmp_buf      resetenv;


void
handlehup(int sig)
{
    signal(SIGHUP, handlehup);
    siglongjmp(resetenv, 1);
}

void
print(char *type, char *msg, ...)
{
    if (!verbose && !strcmp(type, "debug"))
        return;

    va_list ap;
    va_start(ap, msg);
    printf("%s: %s: ", prog, type);
    vprintf(msg, ap);
    fputs("\n", stdout);
    fflush(stdout);
    va_end(ap);

    if (!strcmp(type, "fatal"))
        exit(1);
}


#define DEBUG(...) print("debug", __VA_ARGS__)
#define INFO(...) print("info", __VA_ARGS__)
#define WARN(...) print("warn", __VA_ARGS__)
#define ERROR(...) print("error", __VA_ARGS__)
#define FATAL(...) print("fatal", __VA_ARGS__)


void
receiver(void)
{
    int ignore;
    if (!XTestQueryExtension(display, &ignore, &ignore, &ignore, &ignore))
        FATAL("XTest extension not available");


    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port),
    };

    if (bind(fd, (struct sockaddr*) &addr, sizeof addr) < 0)
        FATAL("bind() failed");
    if (getsockname(fd, (struct sockaddr*) &addr, &(socklen_t){ sizeof addr }) < 0)
        FATAL("getsockname() failed");


    print("info", "listening on 0.0.0.0:%d\n", ntohs(addr.sin_port));


    char    cmd[1024];
    ssize_t sz = 0;
    while ((sz = recv(fd, cmd, sizeof cmd, 0)) > 0) {
        cmd[sz] = 0;

        DEBUG("cmd: %s", cmd);

        int code, button, x, y;
        bool ok = true;

        if (2 == sscanf(cmd, "scale %d %d\n", &x, &y)) {
            ok = true;
            int screen = DefaultScreen(display);
            sx = (double) DisplayWidth(display, screen) / x;
            sy = (double) DisplayHeight(display, screen) / y;
        }
        else if (1 == sscanf(cmd, "keydown %d\n", &code))
            ok = XTestFakeKeyEvent(display, code, true, 0);
        else if (1 == sscanf(cmd, "keyup %d\n", &code))
            ok = XTestFakeKeyEvent(display, code, false, 0);
        else if (1 == sscanf(cmd, "buttondown %d\n", &button))
            ok = XTestFakeButtonEvent(display, button, true, 0);
        else if (1 == sscanf(cmd, "buttonup %d\n", &button))
            ok = XTestFakeButtonEvent(display, button, false, 0);
        else if (2 == sscanf(cmd, "mousemove %d %d\n", &x, &y))
            ok = XTestFakeMotionEvent(display, -1, x * sx, y * sy, 0);
        else
            ERROR("message not understood: %s", cmd);

        XFlush(display);
        if (!ok)
            ERROR("command failed: %s", cmd);
    }
    close(fd);
}


void
command(char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);

    char    cmd[1024];
    size_t  sz = vsnprintf(cmd, sizeof cmd, msg, ap);

    struct sockaddr_in addr = {
                .sin_family = AF_INET,
                .sin_addr.s_addr = INADDR_ANY,
                .sin_port = htons(port),
            };

    if (hostaddr)
        memcpy(&addr.sin_addr, hostaddr, hostaddrlen);

    if (sendto(fd, cmd, sz, 0, (struct sockaddr*) &addr, sizeof addr) < 0)
        ERROR("could not contact server %s:%d", hostname, port);
}


void
sender(void)
{



    int             screen = DefaultScreen(display);
    unsigned long   black = BlackPixel(display, screen);
    unsigned long   white = WhitePixel(display, screen);
    Atom            WM_DELETE_MESSAGE = XInternAtom(display,
                                                    "WM_DELETE_WINDOW",
                                                    false);
    Window window = XCreateSimpleWindow(display,
                                        DefaultRootWindow(display),
                                        0,
                                        0,
                                        DisplayWidth(display, screen),
                                        DisplayHeight(display, screen),
                                        5,
                                        black,
                                        black);
    if (!window)
        FATAL("cannot open window");

    XSetWMProtocols(display, window, &WM_DELETE_MESSAGE, 1);

    char    title[256];
    snprintf(title,
             sizeof title,
             "Remote Keyboard & Mouse (Hit %s to Exit)",
             XKeysymToString(XKeycodeToKeysym(display, breaker, 0)));

    Xutf8SetWMProperties(display,
                         window,
                         title,
                         title,
                         NULL,
                         0,
                         NULL,
                         NULL,
                         NULL);

    XSelectInput(display,
                 window,
                 ButtonPressMask | ButtonReleaseMask |
                 KeyPressMask | KeyReleaseMask |
                 PointerMotionMask |
                 FocusChangeMask |
                 StructureNotifyMask);

    GC gc = XCreateGC(display, window, 0, 0);
    XSetBackground(display, gc, black);
    XSetForeground(display, gc, black);
    XClearWindow(display, window);
    XMapRaised(display, window);


    /*
        Identify our screen size to the server.
     */
    command("scale %d %d\n",
            DisplayWidth(display, screen),
            DisplayHeight(display, screen));


    while (true) {
        XEvent event;
        XNextEvent(display, &event);

        if (event.type == KeyPress && event.xkey.keycode == breaker)
            break;

        else if (event.type == FocusIn)
            XGrabPointer(display,
                         window,
                         true,
                         ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                         GrabModeAsync,
                         GrabModeAsync,
                         None,
                         None,
                         CurrentTime);

        else if (event.type == FocusOut)
            XUngrabPointer(display, CurrentTime);

        else if (event.type == KeyPress)
            command("keydown %d\n", event.xkey.keycode);

        else if (event.type == KeyRelease)
            command("keyup %d\n", event.xkey.keycode);

        else if (event.type == ButtonPress)
            command("buttondown %d\n", event.xbutton.button);

        else if (event.type == ButtonRelease)
            command("buttonup %d\n", event.xbutton.button);

        else if (event.type == MotionNotify)
            command("mousemove %d %d\n",
                    event.xmotion.x_root,
                    event.xmotion.y_root);

        // User closed the window.
        else if (event.type == ClientMessage &&
                 event.xclient.data.l[0] == WM_DELETE_MESSAGE)
                break;
    }
}


void
readconfig(FILE *file)
{
    char    line[PATH_MAX + 1];
    char    tmp[PATH_MAX + 1];

    while (fgets(line, sizeof line, file)) {
        int     n;
        if (1 == sscanf(line, " #%[^\n]", tmp))
            ;
        else if (1 == sscanf(line, " server = %[-a-zA-Z0-9.]\n", tmp))
            if (!strcmp(tmp, "true") || !strcmp(tmp, "yes") || !strcmp(tmp, "1"))
                server = true;
            else if (!strcmp(tmp, "false") || !strcmp(tmp, "no") || !strcmp(tmp, "0"))
                server = false;
            else
                ERROR("invalid value: server = %s", tmp);
        else if (1 == sscanf(line, " port = %d \n", &n))
            port = n;
        else if (1 == sscanf(line, " host = %[-a-zA-Z0-9.] \n", hostname))
            ;
        else if (1 == sscanf(line, " break = %[^ \t\n]", tmp)) {
            KeySym  keysym = XStringToKeysym(tmp);

            if (keysym == NoSymbol)
                ERROR("unrecognized key: %s", tmp);

            else if (!XKeysymToKeycode(display, keysym))
                ERROR("no keycode for: %s", tmp);

            else
                breaker = XKeysymToKeycode(display, keysym);
        }
        else
            ERROR("unrecognized conf line: %s", line);
    }
    fclose(file);
}


void
readallconfigs(void)
{
    char    *prefixes[] = {
        "/etc",
        "/usr/local/etc",
        getenv("XDG_CONFIG_DIRS"),
        getenv("HOME"),
        getenv("XDG_CONFIG_HOME"),
    };
    char    path[PATH_MAX + 1];
    char    tmp[sizeof path];
    FILE    *file;

    for (size_t i = 0; i < sizeof prefixes / sizeof *prefixes; i++) {

        // Environment variable did not exist.
        if (prefixes[i] == NULL)
            continue;

        // Expanded env is larger than buffer.
        if (strlen(prefixes[i]) >= sizeof path)
            continue;

        /*

            Split up environment variables that are compound.
            e.g. $XDG_CONFIG_DIRS

         */
        strcpy(tmp, prefixes[i]);
        char *p = strtok(tmp, ":");
        do {

            if (snprintf(path, sizeof path, "%s/rkm.conf", p) >= sizeof path)
                continue;

            if ((file = fopen(path, "r")))
                readconfig(file);

        } while ((p = strtok(NULL, ":")));
    }
}


int main(int argc, char **argv) {
    int     ch;

    prog = *argv? *argv: prog;

    /*

        Set up SIGHUP signal handler.
        When this is received, the configuration file is reloaded.

     */
    signal(SIGHUP, handlehup);
    if (sigsetjmp(resetenv, false)) {
        close(fd);
        XCloseDisplay(display);
    }


    /*

        Set up the socket and connection to the X server.

     */
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        FATAL("socket() failed");

    display = XOpenDisplay(0);
    if(display == NULL)
        FATAL("cannot connect to X server");

    breaker = XKeysymToKeycode(display, XK_Menu);


    /*

        Read configuration and command line options.

     */

    readallconfigs();

    while ((ch = getopt(argc, argv, "h:p:sv")) != -1)
        switch (ch) {
        case 's':
            server = true;
            break;
        case 'h':
            if (strlen(optarg) < sizeof hostname)
                strcpy(hostname, optarg);
            else
                FATAL("hostname too long: %td/%td",
                      strlen(optarg),
                      sizeof hostname);
            break;
        case 'p':
            port = strtol(optarg, 0, 0);
            break;
        case 'v':
            verbose = true;
            break;
        case '?':
        default:
            printf("%d#\n", ch);
            FATAL("usage: %s [sv] [-h host] [-p port]\n"
                  "  -h host\n"
                  "     host to connect to\n"
                  "  -p port\n"
                  "     port to serve from or connect to\n"
                  "     0 to select random port\n"
                  "  -v verbose information\n",
                  prog);
            break;
        }
    argc -= optind;
    argv -= optind;


    /*

        Find host.

     */
    struct hostent *hp = gethostbyname(hostname);
    if (!hp || !hp->h_addr_list[0] || !hp->h_length)
        FATAL("cannot find hostname: %s", hostname);
    else {
        hostaddrlen = hp->h_length;
        hostaddr = malloc(hostaddrlen);
        memcpy(hostaddr, hp->h_addr_list[0], hostaddrlen);
    }


    if (server)
        receiver();
    else
        sender();

    return 0;
}
