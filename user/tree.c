#include "lib.h"
char path[MAXPATHLEN];

void tree(char *curdir, int indent)
{
	int fd, n;
	char buf[MAXNAMELEN];
	struct File f;

	fd = open(curdir, O_RDONLY);
	if (fd < 0)
	{
		user_panic("open %s: %e", curdir, fd);
	}

	int i;

	while ((n = readn(fd, &f, sizeof(struct File))) == sizeof(struct File))
	{
		if (f.f_name[0])
		{
			for (i = 0; i < indent; ++i)
			{
				fwritef(1, "|   ");
			}
			if (f.f_type == FTYPE_DIR)
			{
				fwritef(1, "|-- %s\n", f.f_name);
				strcpy(buf, curdir);
				strcat(buf, f.f_name);
				strcat(buf, "/");
				tree(buf, indent + 1);
			}
			else
			{
				fwritef(1, "|-- %s\n", f.f_name);
			}
		}
	}

	close(fd);
	if (n)
		user_panic("error in directory %s: %e", curdir, n);
}

void usage()
{
    fwritef(1, "usage: tree [path]\n");
}


void umain(int argc, char **argv) 
{
    int len;
	if (argc > 2)
	{
		usage();
		return;
	}
	struct Stat st;
	if (argc == 1)
		tree("/", 0);
	else
	{
		strcpy(path, argv[1]);
		if (stat(path, &st) < 0)
		{
			fwritef(1, "cannot open directory\n");
			return;
		}
		if (st.st_isdir != FTYPE_DIR)
		{
			fwritef(1, "specified path should be directory\n");
			return;
		}
		if (path[strlen(path) - 1] != '/') 
        {
            len = strlen(path);
            path[len] = '/';
            path[len + 1] = '\0';
        }
		tree(path, 0);
	}
}