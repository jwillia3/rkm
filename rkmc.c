#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <GLFW/glfw3.h>

int                 exitkey = GLFW_KEY_MENU;
int                 fd;
struct addrinfo     *addr;


void
command(const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    char    buffer[1024];
    size_t  len = vsnprintf(buffer, sizeof buffer, msg, ap);
    if (sendto(fd, buffer, len, 0, addr->ai_addr, addr->ai_addrlen) < 0)
        puts("rkmc: cannot send message to server.");
}


void
onkey(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == exitkey) {
        glfwSetWindowShouldClose(window, true);
        return;
    }
    if (action == GLFW_PRESS)
        command("keydown %d\n", scancode);
    else if (action == GLFW_RELEASE)
        command("keyup %d\n", scancode);
}


void
onscroll(GLFWwindow *window, double x, double y)
{
    int button = y < 0? 5: 4;
    command("buttondown %d\n", button);
    command("buttonup %d\n", button);
}


void
onclick(GLFWwindow *window, int button, int action, int mods)
{
    button = button == GLFW_MOUSE_BUTTON_1? 1:
             button == GLFW_MOUSE_BUTTON_2? 3:
             button == GLFW_MOUSE_BUTTON_3? 2:
             button == GLFW_MOUSE_BUTTON_4? 8:
             button == GLFW_MOUSE_BUTTON_5? 9:
             1;
    if (action == GLFW_PRESS)
        command("buttondown %d\n", button);
    else if (action == GLFW_RELEASE)
        command("buttonup %d\n", button);
}


void
onmove(GLFWwindow *window, double x, double y) {
    int sx, sy;
    glfwGetFramebufferSize(window, &sx, &sy);
    command("mousemove %d %d\n",
            (int) fmin(fmax(0.0, x), sx),
            (int) fmin(fmax(0.0, y), sy));
}


void
onfocus(GLFWwindow *window, int focus)
{
    glfwSetInputMode(window,
                     GLFW_CURSOR,
                     focus? GLFW_CURSOR_HIDDEN: GLFW_CURSOR_NORMAL);
}


void
loop(void)
{
    if (!glfwInit())
        puts("GLFW not initialized"),
        exit(0);

    int             sx, sy;
    glfwWindowHint(GLFW_DECORATED, false);
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), 0, 0, &sx, &sy);
    const char      *title = "Remote Keyboard & Mouse";
    GLFWwindow      *window = glfwCreateWindow(sx, sy, title, 0, 0);
    glfwMakeContextCurrent(window);

    glfwGetFramebufferSize(window, &sx, &sy);
    command("scale %d %d\n", sx, sy);

    glfwSetKeyCallback(window, onkey);
    glfwSetScrollCallback(window, onscroll);
    glfwSetMouseButtonCallback(window, onclick);
    glfwSetCursorPosCallback(window, onmove);
    glfwSetWindowFocusCallback(window, onfocus);
    while (!glfwWindowShouldClose(window)) {
        glfwWaitEvents();
        glClearColor(.75, .75, .75, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
    }
}


void
findserver(const char *hostname, int port)
{
    char                portstr[32];
    struct addrinfo     hint = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = 0,
    };
    snprintf(portstr, sizeof portstr, "%d", port);

    if (getaddrinfo(hostname, portstr, &hint, &addr))
        printf("rkmc: cannot find server: %s:%d.\n", hostname, port),
        exit(1);
}


int
main(int argc, char **argv)
{
    if (!argv[1])
        help:
        puts("usage: rkmc HOSTNAME:PORT\n"
             "Remote keyboard and mouse client"),
        exit(1);

    char    *hostname = argv[1];
    char    *portstr = hostname + strcspn(hostname, ":");
    int     port;

    if (*portstr != ':')
        goto help;
    *portstr = 0;
    port = strtol(portstr + 1, &portstr, 0);
    if (*portstr)
        goto help;

    fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        puts("rkmc: cannot get socket."),
        exit(1);

    findserver(hostname, port);

    loop();
}
