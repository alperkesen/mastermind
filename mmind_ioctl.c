#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "mastermind_ioctl.h"
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int fd;
    int res;
    int cmd;

    fd = open("/dev/mastermind", O_RDWR);
    cmd = atoi(argv[1]);

    if (cmd == 0) {
        res = ioctl(fd, MMIND_REMAINING, 0);
	printf("Remaining guess: %d\n", res);
    } else if (cmd == 1) {
        res = ioctl(fd, MMIND_NEWGAME, atoi(argv[2]));
        printf("Started new game! (MN: %s)\n", argv[2]);
    } else if (cmd == 2) {
        res = ioctl(fd, MMIND_ENDGAME, 0);
        printf("End game!\n");
    } else {
        printf("Wrong command!\n");
    }

    return 0;
}
