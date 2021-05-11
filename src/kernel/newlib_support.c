#include <errno.h>
#include <sys/types.h>

void _exit(void) {
    while (1) {
        __asm__("hlt");
    }
}

caddr_t sbrk(int incr) {
    return NULL;
}

int getpid(void) {
    return 1;
}

int kill(int pid, int sig) {
    errno = EINVAL;
    return -1;
}
