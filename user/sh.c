#include "lib.h"
#include <args.h>

#define MAXARGS 16
#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()"

int debug_ = 0;

#define ENV_VAR_MAX 128
#define ENV_NAME_MAX 16
#define ENV_VALUE_MAX 64
#define E_ENV_VAR_READONLY 1
#define E_ENV_VAR_SET 2
struct Env_var{
    char name[ENV_VAR_MAX];
    char value[ENV_VALUE_MAX];
    u_int read_only;
    u_int global;
    u_int envid;
};


void list_env() 
{
    int num;
    int i;
    struct Env_var env_vars[ENV_VAR_MAX];
    fwritef(1, "\x1b[33m-*- ENVIRONMENT VARIABLES -*-\x1b[0m\n");
    u_int envid = syscall_getenvid();
    syscall_list_env(env_vars, envid, &num);
    for (i = 0; i < num; i++) {
        fwritef(1, "\x1b[36m%-16s\x1b[0m\x1b[35m%-10s \x1b[0m\x1b[35m%-10s\x1b[0m%s\n", env_vars[i].name, env_vars[i].read_only ? "READ-ONLY" : "", env_vars[i].global ? "GLOBAL" : "LOCAL", env_vars[i].value);
    }
}


void split(char *arg, char *name, char *value)
{
    int i, j;
    value[0] = 0;
    for (i = 0, j = 0; arg[i] != 0 && arg[i] != '='; i++, j++) {
        name[j] = arg[i];
    }
    name[j] = 0;
    if (arg[i] == '=') {
        for (i = i + 1, j=0; arg[i] != 0; i++, j++) {
            value[j] = arg[i];
        }
        value[j] = 0;
    }
}

void declare(char *name, char *value, u_int read_only, u_int global) 
{
    struct Env_var env_var;
    if (global) {
        env_var.envid = 0;
    } else {
        env_var.envid = syscall_getenvid();
    }
    env_var.global = global;
    strcpy(env_var.name, name);
    strcpy(env_var.value, value);
    env_var.read_only = read_only;

    int r;

    r = syscall_declare(&env_var);
    if (r == -E_ENV_VAR_READONLY) {
        fwritef(1, "error : attept to change READONLY var\n");
    } else if (r == -E_ENV_VAR_SET) {
        fwritef(1, "set env %s fail !\n", name); 
    }
}

int inner_cmd(int argc, char **argv)
{
    char name[ENV_NAME_MAX];
    char value[ENV_VALUE_MAX];
    if (!strcmp(argv[0], "clear")) {
        writef("\x1b[2J\x1b[H");
		return 1;
    }

    if (!strcmp(argv[0], "declare")) {
        if (argc == 1) {
            list_env();
        } else if (argc == 2) {
            split(argv[1], name, value);
            declare(name, value, 0, 0);
        } else if (argc == 3) {
            split(argv[2], name, value);
            if (!strcmp(argv[1], "-x")) {
                declare(name, value, 0, 1);
            } else if(!strcmp(argv[1], "-r")) {
                declare(name, value, 1, 0);
            } else if(!strcmp(argv[1], "-rx") || !strcmp(argv[1], "-xr")) {
                declare(name, value, 1, 1);
            }
        } else {
            fwritef(1, "usage : declare [-xr] [NAME [=VALUE]]\n");
        }
        return 1;
    }
    if (!strcmp(argv[0], "unset")) {
        if (argc == 2) {
            struct Env_var env_var;
            strcpy(env_var.name, argv[1]);
            env_var.envid = syscall_getenvid();
            syscall_unset(&env_var);
        } else {
            fwritef(1, "usage : unset NAME\n");
        }
        return 1;
    }

    return 0;
}



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

    if (*s == '"')
	{
		*p1 = s + 1;
		s++;
		while (*s && *s != '"')
			s++;
		if (*s == 0)
		{
			writef("\" does not match\n");
			return -1;
		}
		*s = 0;
		s++;
		while (*s && !strchr(WHITESPACE SYMBOLS, *s))
			s++;
		*p2 = s;
		return 'w';
	}
    if (*s == '$' && *(s + 1) && !strchr(WHITESPACE, *(s + 1))) 
    {
        *p1 = s + 1;
        s++;
        while (*s && !strchr(WHITESPACE, *s))
			s++;
		*s = 0;
		*p2 = s + 1;
		return '$';
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


void
runcmd(char *s)
{
	char *argv[MAXARGS], *t;
	int argc, c, i, r, p[2], fd, rightpipe, pid;
	int fdnum;
    int hang = 0;
	rightpipe = 0;
    char buffer[MAXARGS][65];
    int buf_len = 0;
    u_int read_only, global;
	gettoken(s, 0);
again:
	argc = 0;
	for(;;){
		c = gettoken(0, &t);
		switch(c){
		case 0:
			goto runit;
        case '&':
            hang = 1;
            break;
		case 'w':
			if(argc == MAXARGS){
				writef("too many arguments\n");
				exit();
			}
			argv[argc++] = t;
			break;
        case '$':
            if (argc == MAXARGS)
			{
				writef("too many arguments\n");
				exit();
			}
            u_int envid = syscall_getenvid();
            if (syscall_get_env((u_int)t, (u_int)buffer[buf_len], envid) < 0) 
            {
                argv[argc++] = t;
            } 
            else 
            {
                argv[argc++] = buffer[buf_len++];
            }
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
        case ';':
            if ((r = fork()) < 0) {
                    writef("; fork error");
                    exit();
                }
            if (r == 0) {
                goto again;
            } else {
                goto runit;
            }

		}
	}

runit:
	if(argc == 0) {
		if (debug_) user_debug("EMPTY COMMAND\n");
		return;
	}
	argv[argc] = 0;
	if (debug_) {
		writef("[%08x] SPAWN:", env->env_id);
		for (i=0; argv[i]; i++)
			writef(" %s", argv[i]);
		writef("\n");
	}

    if (inner_cmd(argc, argv)) {
        exit();
    }

	if ((r = spawn(argv[0], argv)) < 0) {
        writef("spawn error: %s: %e\n", argv[0], r);
        exit();
    }
	close_all();
	if (r > 0) {
		if (debug_) user_debug("[%08x] WAIT %s %08x\n", env->env_id, argv[0], r);
        if (!hang) {
            if (debug_) user_debug("!hang, wait exit");
            wait(r);
        } else {
            pid = fork();
            if (pid == 0) {
                if (debug_) user_debug("hang and child shell, wait exit");
                wait(r);
                exit();
            }
        }
		
	}
	if (rightpipe) {
		if (debug_) user_debug("[%08x] WAIT right-pipe %08x\n", env->env_id, rightpipe);
		wait(rightpipe);
	}

	exit();
}


int islatest_hist = 0;
int now_poi = 0;

int history(char *buf, int up) {
    static char history_info[4 * 1024 * 1024];
    static int size = 0;
    int i, j, end;
    if (!islatest_hist) {
        islatest_hist = 1;
        struct Stat state;
        if (stat(".history", &state) < 0) {
            return 0;
        }
        int f = open(".history", O_RDONLY);
        if (f < 0) {
            return 0;
        }
        read(f, history_info, state.st_size);
        history_info[state.st_size] = 0;
        close(f);
        now_poi = state.st_size - 1;
        size = state.st_size;
    }
    if ((now_poi == size - 1 && up == -1) || (now_poi <= 0 && up == 1)) {
        return 0;
    }
    if (up == 1) {  // up
        end = now_poi;
        for (now_poi = now_poi - 1; now_poi >= 0 && history_info[now_poi] != '\n'; now_poi--);

        for (j = 0, i = now_poi + 1; i < end; i++, j++) {
            buf[j] = history_info[i];
        }
        buf[j] = 0;
        fwritef(1, "%s", buf);
        return strlen(buf);
    }
    if (up == -1) { // down
        for (j = 0, now_poi = now_poi + 1; history_info[now_poi] != '\n'; now_poi++, j++) {
            buf[j] = history_info[now_poi];
        }
        buf[j] = 0;
        fwritef(1, "%s", buf);
        return strlen(buf);
    }
    return 0;
}

void savecmd(char *buf) {
    if (buf[0] == '\0') {
        return;
    }
    islatest_hist = 0;
    int f = open(".history", O_APPEND | O_WRONLY | O_CREAT);
    if (f < 0) {
        fwritef(1, "connot open history\n");
        return;
    }
    fwritef(f, "%s\n", buf);
    close(f);
}

void
readline(char *buf, u_int n)
{
	int i, r;
	r = 0;
    char tmp;
	for(i=0; i<n; i++){
		if((r = read(0, buf+i, 1)) != 1){
			if(r < 0)
				writef("read error: %e", r);
			exit();
		}
		if (buf[i] == 0x7f)
		{
			if (i > 0)
			{
				fwritef(1, "\x1b[1D\x1b[K");
				i -= 2;
			}
			else {
                i = -1;
            }
				
		} else if (buf[i] == '\x1b') {
            read(0, &tmp, 1);
            if (tmp != '[') {
                user_panic("\\x1b is not followed by '['");
            }
            read(0, &tmp, 1);
            if (tmp == 'A') { // up
                if (i) {
                    fwritef(1, "\x1b[1B\x1b[%dD\x1b[K", i);
                } else {
                    fwritef(1, "\x1b[1B");
                }
                i = history(buf, 1);
            } else if(tmp == 'B') {  // down
                if (i) {
                    fwritef(1, "\x1b[1A\x1b[%dD\x1b[K", i);
                } else {
                    fwritef(1, "\x1b[1A");
                }
                i = history(buf, -1);
            } 
            else if(tmp == 'C') 
            {
                fwritef(1, "\x1b[1D");
            } 
            else if(tmp == 'D') 
            {
                fwritef(1, "\x1b[1C");
            }
            i--;
        } else if(buf[i] == '\r' || buf[i] == '\n'){
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

    // writef("argc = %d ; argv: ", argc);
    // for(i = 0;i < argc; i++) {
    //     writef("%s ",argv[i]);
    // }
    // writef("\n");
    
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
		if (interactive) { fwritef(1, "\n$ "); }

		readline(buf, sizeof buf);
        if (buf[0] == 0) {
            continue;
        }
        savecmd(buf);
		if (buf[0] == '#') { continue; }
		if (echocmds) { fwritef(1, "# %s\n", buf); }
        if (!strcmp(buf, "exit")) {
            exit();
        }
		if ((r = fork()) < 0) {
            user_panic("fork: %e", r);
        }
		if (r == 0) {
			runcmd(buf);
			exit();
			return;
		} else
			wait(r);
	}
}

