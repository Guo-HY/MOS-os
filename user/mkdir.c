#include "lib.h"

void umain(int argc, char **argv)
{
	if (argc != 2)
	{
		fwritef(1, "usage: mkdir directory_name\n");
		return;
	}
	int fd;
	fd = open(argv[1], O_MKDIR);
	if (fd < 0)
	{
		fwritef(1, "mkdir error!\n");
		return;
	}
	close(fd);
}
