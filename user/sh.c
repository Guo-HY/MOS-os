#include "lib.h"
#include <args.h>

int debug_ = 0;

//
// get the next token from string s
// set *p1 to the beginning of the token and
// *p2 just past the token.
// return:
//	0 for end-of-string
//	> for >
//	| for |
//	w for a word
//
// eventually (once we parse the space where the nul will go),
// words get nul-terminated.
#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()"

int
_gettoken(char *s, char **p1, char **p2)
{
	int t;

	if (s == 0) {
		if (debug_) user_debug("GETTOKEN NULL\n");
		return 0;
	}

	if (debug_) user_debug("GETTOKEN: %s\n", s);

	*p1 = 0;
	*p2 = 0;

	while(strchr(WHITESPACE, *s))
		*s++ = 0;
	if(*s == 0) {
		if (debug_) user_debug("EOL\n");
		return 0;
	}
	if(strchr(SYMBOLS, *s)){
		t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		if (debug_) user_debug("TOK %c\n", t);
		return t;
	}
	*p1 = s;
	while(*s && !strchr(WHITESPACE SYMBOLS, *s))
		s++;
	*p2 = s;
	if (debug_) {
		t = **p2;
		**p2 = 0;
		user_debug("WORD: %s\n", *p1);
		**p2 = t;
	}
	return 'w';
}

int
gettoken(char *s, char **p1)
{
	static int c, nc;
	static char *np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

#define MAXARGS 16
void
runcmd(char *s)
{
	char *argv[MAXARGS], *t;
	int argc, c, i, r, p[2], fd, rightpipe;
	int fdnum;
	rightpipe = 0;
	gettoken(s, 0);
again:
	argc = 0;
	for(;;){
		c = gettoken(0, &t);
		switch(c){
		case 0:
			goto runit;
		case 'w':
			if(argc == MAXARGS){
				writef("too many arguments\n");
				exit();
			}
			argv[argc++] = t;
			break;
		case '<':
			if(gettoken(0, &t) != 'w'){
				writef("syntax error: < not followed by word\n");
				exit();
			}
			// Your code here -- open t for reading,
			// dup it onto fd 0, and then close the fd you got.
            if ((fdnum = open(t, O_RDONLY)) < 0) {
                writef(" < open error");
                exit();
            }
            if ((r = dup(fdnum, 0)) < 0) {
                writef(" < dup error");
                exit();
            }
            close(fdnum);
			break;
		case '>':
            if(gettoken(0, &t) != 'w'){
				writef("syntax error: < not followed by word\n");
				exit();
			}
			// Your code here -- open t for writing,
			// dup it onto fd 1, and then close the fd you got.
            if ((fdnum = open (t, O_WRONLY)) < 0) {
                writef(" > open error");
                exit();
            }
            if ((r = dup(fdnum, 1)) < 0) {
                writef(" > dup error");
                exit();
            }
            close(fdnum);
			break;
		case '|':
			// Your code here.
			// 	First, allocate a pipe.
			//	Then fork.
			//	the child runs the right side of the pipe:
			//		dup the read end of the pipe onto 0
			//		close the read end of the pipe
			//		close the write end of the pipe
			//		goto again, to parse the rest of the command line
			//	the parent runs the left side of the pipe:
			//		dup the write end of the pipe onto 1
			//		close the write end of the pipe
			//		close the read end of the pipe
			//		set "rightpipe" to the child envid
			//		goto runit, to execute this piece of the pipeline
			//			and then wait for the right side to finish
            if ((r = pipe(p)) < 0) {
                writef("| pipe error");
                exit();
            }
            if ((r = fork()) < 0) {
                writef("| fork error");
                exit();
            }
            if (r == 0) {
                if ((r = dup(p[0], 0)) < 0) {
                    writef("| child dup error");
                    exit();
                }
                close(p[0]);
                close(p[1]);
                goto again;
            }else {
                rightpipe = r;
                if ((r = dup(p[1], 1)) < 0) {
                    writef("| father dup error");
                    exit();
                }
                close(p[0]);
                close(p[1]);
                goto runit;
            }
			break;
		}
	}

runit:
	if(argc == 0) {
		if (debug_) user_debug("EMPTY COMMAND\n");
		return;
	}
	argv[argc] = 0;
	if (1) {
		writef("[%08x] SPAWN:", env->env_id);
		for (i=0; argv[i]; i++)
			writef(" %s", argv[i]);
		writef("\n");
	}

	if ((r = spawn(argv[0], argv)) < 0)
		writef("spawn error: %s: %e\n", argv[0], r);
	close_all();
	if (r >= 0) {
		if (debug_) user_debug("[%08x] WAIT %s %08x\n", env->env_id, argv[0], r);
		wait(r);
	}
	if (rightpipe) {
		if (debug_) user_debug("[%08x] WAIT right-pipe %08x\n", env->env_id, rightpipe);
		wait(rightpipe);
	}

	exit();
}

void
readline(char *buf, u_int n)
{
	int i, r;

	r = 0;
	for(i=0; i<n; i++){
		if((r = read(0, buf+i, 1)) != 1){
			if(r < 0)
				writef("read error: %e", r);
			exit();
		}
		if(buf[i] == '\b'){
			if(i > 0)
				i -= 2;
			else
				i = 0;
		}
		if(buf[i] == '\r' || buf[i] == '\n'){
			buf[i] = 0;
			return;
		}
	}
	writef("line too long\n");
	while((r = read(0, buf, 1)) == 1 && buf[0] != '\n')
		;
	buf[0] = 0;
}	

char buf[1024];

void
usage(void)
{
    writef("#############################################\n");
	writef("#  usage: sh [-dix] [command-file]          #\n");
    writef("#  d : debug                                #\n");
    writef("#  i : set stdin to console                 #\n");
    writef("#  x : echo command                         #\n");
    writef("#  if set [command-file], then read command #\n");
    writef("#  from [command file]                      #\n");
    writef("#############################################\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int r, interactive, echocmds;
	interactive = '?';
	echocmds = 0;
    int i;
	writef("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	writef("::                                                         ::\n");
	writef("::              Super Shell  V0.0.0_1                      ::\n");
	writef("::                                                         ::\n");
	writef(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");

    writef("argc = %d ; argv: ", argc);
    for(i = 0;i < argc; i++) {
        writef("%s ",argv[i]);
    }
    writef("\n");
    
	ARGBEGIN{
	case 'd':
        if (debug_) user_debug("shell debug : in case d\n");
		debug_++;
		break;
	case 'i':
        if (debug_) user_debug("shell debug : in case i\n");
		interactive = 1;
		break;
	case 'x':
        if (debug_) user_debug("shell debug : in case x\n");
		echocmds = 1;
		break;
	default:
        if (debug_) user_debug("shell debug : in case default\n");
		usage();
	}ARGEND

	if(argc > 1) {
        if (debug_) user_debug("shell debug : in argc > 1\n");
        usage();
    }
		
	if(argc == 1){
        if (debug_) user_debug("shell debug : in argc == 1\n");
		close(0);
		if ((r = open(argv[1], O_RDONLY)) < 0)
			user_panic("open %s: %e", r);
		user_assert(r==0);
	}
	if(interactive == '?') {
        if (debug_) user_debug("shell debug : in interactive == ?\n");
        interactive = iscons(0);
    }
		
	for(;;){
		if (interactive)
			fwritef(1, "\n$ ");
		readline(buf, sizeof buf);
		
		if (buf[0] == '#')
			continue;
		if (echocmds)
			fwritef(1, "# %s\n", buf);
		if ((r = fork()) < 0)
			user_panic("fork: %e", r);
		if (r == 0) {
			runcmd(buf);
			exit();
			return;
		} else
			wait(r);
	}
}

