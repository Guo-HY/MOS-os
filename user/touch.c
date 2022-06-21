#include "lib.h"

void umain(int argc, char **argv)
{
	if (argc != 2)
	{
		fwritef(1, "usage: touch file_name\n");
		return;
	}
	int fd = open(argv[1], O_CREAT);
	if (fd < 0)
	{
		fwritef(1, "touch error!\n");
		return;
	}
    close(fd);
}
