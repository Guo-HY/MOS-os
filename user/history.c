#include "lib.h"

void umain(int argc, char **argv) 
{
    char history_info[4 * 1024 * 1024];
    int fd = open(".history", O_RDONLY | O_CREAT);
    fwritef(1, "--------HISTORY--------\n");
    struct Stat state;
    if (stat(".history", &state) < 0) {
            return 0;
    }
    read(fd, history_info, state.st_size);
    history_info[state.st_size] = 0;
    fwritef(1, "%s", history_info);
    close(fd);
}