/* $Header: /home/foxen/CR/FB/src/RCS/interface.c,v 1.1 1996/06/12 02:40:50 foxen Exp $ */

/*
 * $Log: interface.c,v $
 * Revision 1.1  1996/06/12 02:40:50  foxen
 * Initial revision
 *
 * Revision 5.25  1994/03/21  11:00:42  foxen
 * Autoconfiguration mods.
 *
 * Revision 5.24  1994/03/14  12:20:58  foxen
 * Fb5.20 release checkpoint.
 *
 * Revision 5.23  1994/03/14  12:08:46  foxen
 * Initial portability mods and bugfixes.
 *
 * Revision 5.22  1994/02/13  13:54:42  foxen
 * Added the DESCR_SETUSER primitive.
 *
 * Revision 5.21  1994/02/13  09:46:49  foxen
 * added DESCR_SETUSER primitive.
 *
 * Revision 5.20  1994/02/11  05:52:41  foxen
 * Memory cleanup and monitoring code mods.
 *
 * Revision 5.19  1994/02/08  11:08:32  foxen
 * Changes to make general propqueues run immediately, instead of queueing up.
 *
 * Revision 5.18  1994/01/29  03:20:18  foxen
 * make WHO_doing not end pad with spaces.
 *
 * Revision 5.17  1994/01/18  20:52:20  foxen
 * Version 5.15 release.
 *
 * Revision 5.16  1994/01/15  02:04:18  foxen
 * Minor code cleanup for @doing mods.
 *
 * Revision 5.15  1994/01/15  00:31:07  foxen
 * @doing mods.
 *
 * Revision 5.14  1994/01/10  10:29:31  foxen
 * wall_and_flush() no longer will broadcast null strings.
 *
 * Revision 5.12  1994/01/06  03:15:30  foxen
 * version 5.12
 *
 * Revision 5.11  1993/12/20  06:22:51  foxen
 * Fixed QUIT bug from 5.10.
 *
 * Revision 5.1  1993/12/17  00:07:33  foxen
 * initial revision.
 *
 * Revision 1.1  91/01/24  00:44:18  cks
 * Initial revision
 * 
 * Revision 1.1  91/01/24  00:44:18  cks
 * changes for QUELL.
 *
 * Revision 1.0  91/01/22  21:56:00  cks
 * Initial revision
 *
 * Revision 1.13  90/09/28  12:23:15  rearl
 * Fixed @boot bug!  Finally!
 *
 * Revision 1.12  90/09/16  04:42:16  rearl
 * Preparation code added for disk-based MUCK.
 *
 * Revision 1.11  90/09/15  22:23:23  rearl
 * Fixed non-HOSTNAMES problem.  Well, problems.
 *
 * Revision 1.10  90/09/13  06:25:47  rearl
 * Changed boot_off() a bit in hopes of finding the bug...
 *
 * Revision 1.9  90/09/10  02:19:00  rearl
 * Changed NL line termination to CR/LF pairs.
 *
 * Revision 1.8  90/09/04  18:43:55  rearl
 * Fixed bug with connecting players.
 *
 * Revision 1.7  90/08/27  03:27:03  rearl
 * Fixed dump_status logging.
 *
 * Revision 1.6  90/08/15  03:02:23  rearl
 * Took out #ifdef CONNECT_MESSAGES.
 *
 * Revision 1.5  90/08/02  18:46:38  rearl
 * Fixed calls to log functions.
 *
 * Revision 1.4  90/07/29  17:35:28  rearl
 * Some minor fix-ups, most notably, gives number of players connected
 * in the WHO list (dump_users()).
 *
 * Revision 1.3  90/07/23  14:52:33  casie
 * *** empty log message ***
 *
 * Revision 1.2  90/07/21  01:40:25  casie
 * corrected a bug with the log_status function call.
 *
 * Revision 1.1  90/07/19  23:03:42  casie
 * Initial revision
 *
 *
 */

#include "copyright.h"
#include "config.h"
#include "match.h"
#include "mpi.h"

#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include<openssl/ssl.h>

#include "db.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "props.h"
#include "externs.h"

extern int errno;
int     shutdown_flag = 0;
int     restart_flag = 0;

static const char *connect_fail = "Either that player does not exist, or has a different password.\r\n";

static const char *create_fail = "Either there is already a player with that name, or that name is illegal.\r\n";

static const char *flushed_message = "<Output Flushed>\r\n";
static const char *shutdown_message = "\r\nGoing down - Bye\r\n";

int resolver_sock[2];

struct text_block {
    int     nchars;
    struct text_block *nxt;
    char   *start;
    char   *buf;
};

struct text_queue {
    int     lines;
    struct text_block *head;
    struct text_block **tail;
};

struct descriptor_data {
    int     descriptor;
    SSL    *ssl;
    int     ssl_accepted;
    int     connected;
    int     booted;
    dbref   player;
    char   *output_prefix;
    char   *output_suffix;
    int     output_size;
    struct text_queue output;
    struct text_queue input;
    char   *raw_input;
    char   *raw_input_at;
    long    last_time;
    long    connected_at;
    const char *hostname;
    const char *username;
    int     quota;
    struct descriptor_data *next;
    struct descriptor_data **prev;
} *descriptor_list = 0;

struct socks {
       int     s;
       int is_ssl;
} sock[MAX_LISTEN_PORTS];

SSL_CTX *ctx;

static int numsocks = 0;
/* static int sock[MAX_LISTEN_PORTS]; */
static int ndescriptors = 0;
extern void fork_and_dump();

extern int rwhocli_setup(const char *server, const char *serverpw, const char *myname, const char *comment);
extern int rwhocli_shutdown();
extern int rwhocli_pingalive();
extern int rwhocli_userlogin(const char *uid, const char *name,time_t tim);
extern int rwhocli_userlogout(const char *uid);

void    process_commands(void);
void    shovechars(int portc, int* portv);
static void    shutdownsock(struct descriptor_data * d);
static struct descriptor_data *initializesock(int s, const char *hostname, SSL *ssl);
void    make_nonblocking(int s);
static void    freeqs(struct descriptor_data * d);
static void    welcome_user(struct descriptor_data * d);
static void    check_connect(struct descriptor_data * d, const char *msg);
void    close_sockets(const char *msg);
int     boot_off(dbref player);
void    boot_player_off(dbref player);
const char *addrout(long, unsigned short, unsigned short);
static void    dump_users(struct descriptor_data * d, char *user);
static struct descriptor_data *new_connection(int sock, int port, int is_ssl);
void    parse_connect(const char *msg, char *command, char *user, char *pass);
void    set_userstring(char **userstring, const char *command);
static int     do_command(struct descriptor_data * d, char *command);
char   *strsave(const char *s);
int     make_socket(int);
static int     queue_string(struct descriptor_data *, const char *);
static int     queue_write(struct descriptor_data *, const char *, int);
static int     process_output(struct descriptor_data * d);
static int     process_input(struct descriptor_data * d);
void    announce_connect(dbref);
static void    announce_disconnect(struct descriptor_data *);
char   *time_format_1(long);
char   *time_format_2(long);
void    init_descriptor_lookup();
void    init_descr_count_lookup();
static void    remember_descriptor(struct descriptor_data *);
void    remember_player_descr(dbref player, int);
void    update_desc_count_table();
void    forget_player_descr(dbref player, int);
static void    forget_descriptor(struct descriptor_data *);
static struct descriptor_data* descrdata_by_descr(int i);
static struct descriptor_data* lookup_descriptor(int);
int     online(dbref player);
int     online_init();
dbref   online_next(int *ptr);
long 	max_open_files(void);



#ifdef SPAWN_HOST_RESOLVER
void kill_resolver();
#endif

void spawn_resolver();
void resolve_hostnames();

#define MALLOC(result, type, number) do {   \
                                       if (!((result) = (type *) malloc ((number) * sizeof (type)))) \
                                       panic("Out of memory");                             \
                                     } while (0)

#define FREE(x) (free((void *) x))

#ifndef BOOLEXP_DEBUGGING

#ifdef DISKBASE
extern FILE *input_file;
extern FILE *delta_infile;
extern FILE *delta_outfile;

#endif

short db_conversion_flag = 0;
short db_decompression_flag = 0;
short wizonly_mode = 0;

time_t sel_prof_start_time;
long sel_prof_idle_sec;
long sel_prof_idle_usec;
unsigned long sel_prof_idle_use;

void
show_program_usage(char *prog)
{
    fprintf(stderr, "Usage: %s [<options>] infile dumpfile [portnum [portnum ...]]\n", prog);
    fprintf(stderr, "            -convert       load db, save in current format, and quit.\n");
    fprintf(stderr, "            -decompress    when saving db, save in uncompressed format.\n");
    fprintf(stderr, "            -nosanity      don't do db sanity checks at startup time.\n");
    fprintf(stderr, "            -insanity      load db, then enter interactive sanity editor.\n");
    fprintf(stderr, "            -sanfix        attempt to auto-fix a corrupt db after loading.\n");
    fprintf(stderr, "            -wizonly       only allow wizards to login.\n");
    fprintf(stderr, "            -help          display this message.\n");
    exit(1);
}

extern int sanity_violated;

int 
main(int argc, char **argv)
{
    FILE *ffd;
    char *infile_name = NULL;
    char *outfile_name = NULL;
    int i, plain_argnum, nomore_options;
    int sanity_skip;
    int sanity_interactive;
    int sanity_autofix;
    int portcount = 0;
       int ssl_init = 0;
    int whatport[MAX_LISTEN_PORTS];

#ifdef DETACH
    int   fd;
#endif

    if (argc < 3) {
	show_program_usage(*argv);
    }

    time(&current_systime);
    init_descriptor_lookup();
    init_descr_count_lookup();

    plain_argnum = 0;
    nomore_options = 0;
    sanity_skip = 0;
    sanity_interactive = 0;
    sanity_autofix = 0;
    for (i = 1; i < argc; i++) {
	if (!nomore_options && argv[i][0] == '-') {
	    if (!strcmp(argv[i], "-convert")) {
		db_conversion_flag = 1;
	    } else if (!strcmp(argv[i], "-decompress")) {
		db_decompression_flag = 1;
	    } else if (!strcmp(argv[i], "-nosanity")) {
		sanity_skip = 1;
	    } else if (!strcmp(argv[i], "-insanity")) {
		sanity_interactive = 1;
	    } else if (!strcmp(argv[i], "-wizonly")) {
		wizonly_mode = 1;
	    } else if (!strcmp(argv[i], "-sanfix")) {
		sanity_autofix = 1;
	    } else if (!strcmp(argv[i], "--")) {
	        nomore_options = 1;
	    } else {
	        show_program_usage(*argv);
	    }
	} else {
	    plain_argnum++;
	    switch (plain_argnum) {
	      case 1:
	        infile_name = argv[i];
	        break;
	      case 2:
	        outfile_name = argv[i];
	        break;
	      default:
                if (argv[i][0] == 's') {
                        ssl_init = 1;
                        sock[portcount].is_ssl = 1;
                        whatport[portcount] = atoi(argv[i]+1);
                } else {
                        sock[portcount].is_ssl = 0;
                        whatport[portcount] = atoi(argv[i]);
                }
                if (portcount>= MAX_LISTEN_PORTS) {
                        show_program_usage(*argv);
                }
                if (whatport[portcount]<  1 || whatport[portcount]>  65535) {
                        show_program_usage(*argv);
                }
                portcount++;
                break;
	    }
	}
    }
    if (plain_argnum < 2) {
	show_program_usage(*argv);
    } else if (plain_argnum == 2) {
	whatport[portcount++] = TINYPORT;
    } else

#ifdef DISKBASE
    if (!strcmp(infile_name, outfile_name)) {
	fprintf(stderr, "Output file must be different from the input file.");
	exit(3);
    }
#endif

    if (!sanity_interactive) {

#ifdef DETACH
	/* Go into the background */
	fclose(stdin); fclose(stdout); fclose(stderr);
	if (fork() != 0) exit(0);
#endif

	/* save the PID for future use */
	if ((ffd = fopen(PID_FILE,"w")) != NULL)
	{
	    fprintf(ffd, "%d\n", getpid());
	    fclose(ffd);
	}

	log_status("INIT: TinyMUCK %s starting.\n", "version");

#ifdef DETACH
	/* Detach from the TTY, log whatever output we have... */
	freopen(LOG_ERR_FILE,"a",stderr);
	setbuf(stderr, NULL);
	freopen(LOG_FILE,"a",stdout);
	setbuf(stdout, NULL);

	/* Disassociate from Process Group */
# ifdef SYS_POSIX
	setsid();
# else
#  ifdef  SYSV
	setpgrp(); /* System V's way */
#  else
	setpgrp(0, getpid()); /* BSDism */
#  endif  /* SYSV */

#  ifdef  TIOCNOTTY  /* we can force this, POSIX / BSD */
	if ( (fd = open("/dev/tty", O_RDWR)) >= 0)
	{
	    ioctl(fd, TIOCNOTTY, (char *)0); /* lose controll TTY */
	    close(fd);
	}
#  endif /* TIOCNOTTY */
# endif /* !SYS_POSIX */
#endif /* DETACH */

#ifdef SPAWN_HOST_RESOLVER
	if (!db_conversion_flag) {
	    spawn_resolver();
	}
#endif

    }

    sel_prof_start_time = time(NULL); /* Set useful starting time */
    sel_prof_idle_sec = 0;
    sel_prof_idle_usec = 0;
    sel_prof_idle_use = 0;
    if (init_game(infile_name, outfile_name) < 0) {
	fprintf(stderr, "Couldn't load %s!\n", infile_name);
	exit(2);
    }

    if (!sanity_interactive && !db_conversion_flag) {
	set_signals();

#ifdef MUD_ID
	do_setuid(MUD_ID);
#endif				/* MUD_ID */

	if (!sanity_skip) {
	    sanity(AMBIGUOUS);
	    if (sanity_violated) {
	        wizonly_mode = 1;
	        if (sanity_autofix) {
		    sanfix(AMBIGUOUS);
	        }
	    }
	}

       if(ssl_init) {
               SSL_library_init();
               ctx = SSL_CTX_new(SSLv23_method());
               if(!SSL_CTX_use_certificate_chain_file(ctx, SSL_KEY_FILE)) {
                       fprintf(stderr, "Couldn't load %s!\n", SSL_KEY_FILE);
                       exit(2);
               }
               if(!SSL_CTX_use_PrivateKey_file(ctx, SSL_KEY_FILE, SSL_FILETYPE_PEM)) {
                       fprintf(stderr, "Couldn't load %s!\n", SSL_KEY_FILE);
                       exit(2);
               }
               if(!SSL_CTX_load_verify_locations(ctx, NULL, SSL_CA_DIR)) {
                       fprintf(stderr, "Couldn't load CA from %s!\n", SSL_CA_DIR);
                       exit(2);
               }
       }

	/* go do it */
	shovechars(portcount, whatport);

	if (restart_flag) {
	    close_sockets(
		"\r\nServer restarting.  Try logging back on in a few minutes.");
	} else {
	    close_sockets("\r\nServer shutting down normally.");
	}

	do_dequeue((dbref) 1, "all");

	if (tp_rwho) {
	    rwhocli_shutdown();
	}
    }

    if (sanity_interactive) {
        san_main();
    } else {
	dump_database();
	tune_save_parmsfile();

#ifdef SPAWN_HOST_RESOLVER
	kill_resolver();
#endif

#ifdef MALLOC_PROFILING
	db_free();
	free_old_macros();
	purge_all_free_frames();
	purge_mfns();
#endif

#ifdef DISKBASE
	fclose(input_file);
#endif
#ifdef DELTADUMPS
	fclose(delta_infile);
	fclose(delta_outfile);
	(void) unlink(DELTAFILE_NAME);
#endif

#ifdef MALLOC_PROFILING
	CrT_summarize_to_file("malloc_log", "Shutdown");
#endif

       SSL_CTX_free(ctx);

	if (restart_flag) {
	    char* argbuf[MAX_LISTEN_PORTS + 3];
	    int socknum;
	    int argnum = 0;

	    argbuf[argnum++] = "restart";
	    for (socknum = 0; socknum < numsocks; socknum++, argnum++) {
		argbuf[argnum] = (char *)malloc(16);
		sprintf(argbuf[argnum], "%d", whatport[socknum]);
	    }
	    argbuf[argnum] = NULL;
	    execv("restart", argbuf);
	}
    }

    exit(0);
    return 0;
}

#endif				/* BOOLEXP_DEBUGGING */


int 
notify_nolisten(dbref player, const char *msg, int isprivate)
{
    /* struct descriptor_data *d; */
    int     retval = 0;
    char    buf[BUFFER_LEN + 2];
    char    buf2[BUFFER_LEN + 2];
    int firstpass = 1;
    char *ptr1;
    const char *ptr2;
    dbref ref;
    int di;
    int* darr;
    int dcount;

#ifdef COMPRESS
    extern const char *uncompress(const char *);

    msg = uncompress(msg);
#endif				/* COMPRESS */

#if defined(ANONYMITY)
    msg = unmangle(player, msg);
#endif

    ptr2 = msg;
    while (ptr2 && *ptr2) {
	ptr1 = buf;
	while (ptr2 && *ptr2 && *ptr2 != '\r')
	    *(ptr1++) = *(ptr2++);
	*(ptr1++) = '\r';
	*(ptr1++) = '\n';
	*(ptr1++) = '\0';
	if (*ptr2 == '\r')
	    ptr2++;

	if (Typeof(player) == TYPE_PLAYER) {
	    darr = DBFETCH(player)->sp.player.descrs;
	    dcount = DBFETCH(player)->sp.player.descr_count;
	    if (!darr) {
		dcount = 0;
	    }
	} else {
	    dcount = 0;
	    darr = NULL;
	}

        for (di = 0; di < dcount; di++) {
            queue_string(descrdata_by_descr(darr[di]), buf);
            if (firstpass) retval++;
        }

	if (tp_zombies) {
	    if ((Typeof(player) == TYPE_THING) && (FLAGS(player) & ZOMBIE) &&
		    !(FLAGS(OWNER(player)) & ZOMBIE) &&
		    (!(FLAGS(player) & DARK) || Wizard(OWNER(player)))) {
		ref = getloc(player);
		if (Wizard(OWNER(player)) || ref == NOTHING ||
			Typeof(ref) != TYPE_ROOM || !(FLAGS(ref) & ZOMBIE)) {
		    if (isprivate || getloc(player) != getloc(OWNER(player))) {
			char pbuf[BUFFER_LEN];
			const char *prefix;

			prefix = GETPECHO(player);
			if (prefix && *prefix) {
			    char ch = *match_args;

			    *match_args = '\0';
			    prefix = do_parse_mesg(player, player, prefix,
			    		"(@Pecho)", pbuf, MPI_ISPRIVATE
			    	     );
			    *match_args = ch;
			}
			if (!prefix || !*prefix) {
			    prefix = NAME(player);
			    sprintf(buf2, "%s> %.*s", prefix,
				(int)(BUFFER_LEN - (strlen(prefix) + 3)), buf);
			} else {
			    sprintf(buf2, "%s %.*s", prefix,
				(int)(BUFFER_LEN - (strlen(prefix) + 2)), buf);
			}

                        if (Typeof(OWNER(player)) == TYPE_PLAYER) {
                            darr   = DBFETCH(OWNER(player))->sp.player.descrs;
                            dcount = DBFETCH(OWNER(player))->sp.player.descr_count;
                            if (!darr) {
                                dcount = 0;
                            }
                        } else {
                            dcount = 0;
                            darr = NULL;
                        }

                        for (di = 0; di < dcount; di++) {
                            queue_string(descrdata_by_descr(darr[di]), buf2);
                            if (firstpass) retval++;
                        }
		    }
		}
	    }
	}
	firstpass = 0;
    }
    return retval;
}

int 
notify_from_echo(dbref from, dbref player, const char *msg, int isprivate)
{
    const char *ptr;

#ifdef COMPRESS
    extern const char *uncompress(const char *);
    ptr = uncompress(msg);
#else
    ptr = msg;
#endif				/* COMPRESS */

#ifdef ANONYMITY
    ptr = unmangle(player, ptr);
#endif

    if (tp_listeners) {
	if (tp_listeners_obj || Typeof(player) == TYPE_ROOM) {
	    listenqueue(from, getloc(from), player, player, NOTHING,
			"_listen", ptr, tp_listen_mlev, 1, 0);
	    listenqueue(from, getloc(from), player, player, NOTHING,
			"~listen", ptr, tp_listen_mlev, 1, 1);
	    listenqueue(from, getloc(from), player, player, NOTHING,
			"~olisten", ptr, tp_listen_mlev, 0, 1);
	}
    }

    if (Typeof(player) == TYPE_THING && (FLAGS(player) & VEHICLE) &&
	    (!(FLAGS(player) & DARK) || Wizard(OWNER(player)))
    ) {
	dbref ref;
	ref = getloc(player);
	if (Wizard(OWNER(player)) || ref == NOTHING ||
		Typeof(ref) != TYPE_ROOM || !(FLAGS(ref) & VEHICLE)
	) {
	    if (!isprivate && getloc(from) == getloc(player)) {
		char buf[BUFFER_LEN];
		char pbuf[BUFFER_LEN];
		const char *prefix;

		prefix = GETOECHO(player);
		if (prefix && *prefix) {
		    char ch = *match_args;

		    *match_args = '\0';
		    prefix = do_parse_mesg(from, player, prefix,
				    "(@Oecho)", pbuf, MPI_ISPRIVATE
			    );
		    *match_args = ch;
		}
		if (!prefix || !*prefix)
		    prefix = "Outside>";
		sprintf(buf, "%s %.*s",
			prefix,
			(int)(BUFFER_LEN - (strlen(prefix) + 2)),
			msg
		);
		ref = DBFETCH(player)->contents;
		while (ref != NOTHING) {
		    notify_nolisten(ref, buf, isprivate);
		    ref = DBFETCH(ref)->next;
		}
	    }
	}
    }

    return notify_nolisten(player, msg, isprivate);
}

int 
notify_from(dbref from, dbref player, const char *msg)
{
    return notify_from_echo(from, player, msg, 1);
}

int 
notify(dbref player, const char *msg)
{
    return notify_from_echo(player, player, msg, 1);
}


struct timeval 
timeval_sub(struct timeval now, struct timeval then)
{
    now.tv_sec -= then.tv_sec;
    now.tv_usec -= then.tv_usec;
    if (now.tv_usec < 0) {
	now.tv_usec += 1000000;
	now.tv_sec--;
    }
    return now;
}

int 
msec_diff(struct timeval now, struct timeval then)
{
    return ((now.tv_sec - then.tv_sec) * 1000
	    + (now.tv_usec - then.tv_usec) / 1000);
}

struct timeval 
msec_add(struct timeval t, int x)
{
    t.tv_sec += x / 1000;
    t.tv_usec += (x % 1000) * 1000;
    if (t.tv_usec >= 1000000) {
	t.tv_sec += t.tv_usec / 1000000;
	t.tv_usec = t.tv_usec % 1000000;
    }
    return t;
}

struct timeval 
update_quotas(struct timeval last, struct timeval current)
{
    int     nslices;
    int     cmds_per_time;
    struct descriptor_data *d;

    nslices = msec_diff(current, last) / tp_command_time_msec;

    if (nslices > 0) {
	for (d = descriptor_list; d; d = d->next) {
	    if (d->connected) {
		cmds_per_time = ((FLAGS(d->player) & INTERACTIVE)
			      ? (tp_commands_per_time * 8) : tp_commands_per_time);
	    } else {
		cmds_per_time = tp_commands_per_time;
	    }
	    d->quota += cmds_per_time * nslices;
	    if (d->quota > tp_command_burst_size)
		d->quota = tp_command_burst_size;
	}
    }
    return msec_add(last, nslices * tp_command_time_msec);
}

/*
 * long max_open_files()
 *
 * This returns the max number of files you may have open
 * as a long, and if it can use setrlimit() to increase it,
 * it will do so.
 *
 * Becuse there is no way to just "know" if get/setrlimit is
 * around, since its defs are in <sys/resource.h>, you need to
 * define USE_RLIMIT in config.h to attempt it.
 *
 * Otherwise it trys to use sysconf() (POSIX.1) or getdtablesize()
 * to get what is avalible to you.
 */
#ifdef HAVE_RESOURCE_H
# include <sys/resource.h>
#endif

#if defined(RLIMIT_NOFILE) || defined(RLIMIT_OFILE)
# define USE_RLIMIT
#endif

long 	max_open_files(void)
{
#if defined(_SC_OPEN_MAX) && !defined(USE_RLIMIT) /* Use POSIX.1 method, sysconf() */
/*
 * POSIX.1 code.
 */
    return sysconf(_SC_OPEN_MAX);
#else /* !POSIX */
# if defined(USE_RLIMIT) && (defined(RLIMIT_NOFILE) || defined(RLIMIT_OFILE))
#  ifndef RLIMIT_NOFILE
#   define RLIMIT_NOFILE RLIMIT_OFILE /* We Be BSD! */
#  endif /* !RLIMIT_NOFILE */
/*
 * get/setrlimit() code.
 */
    struct rlimit file_limit;
    
    getrlimit(RLIMIT_NOFILE, &file_limit);		/* Whats the limit? */

    if (file_limit.rlim_cur < file_limit.rlim_max)	/* if not at max... */
    {
	file_limit.rlim_cur = file_limit.rlim_max;	/* ...set to max. */
	setrlimit(RLIMIT_NOFILE, &file_limit);
	
	getrlimit(RLIMIT_NOFILE, &file_limit);		/* See what we got. */
    }	

    return (long) file_limit.rlim_cur;

# else /* !RLIMIT */
/*
 * Don't know what else to do, try getdtablesize().
 * email other bright ideas to me. :) (whitefire)
 */
    return (long) getdtablesize();
# endif /* !RLIMIT */
#endif /* !POSIX */
}

static void 
goodbye_user(struct descriptor_data * d)
{
    if(d->ssl) {
        SSL_write(d->ssl, "\r\n", 2);
        SSL_write(d->ssl, tp_leave_mesg, strlen(tp_leave_mesg));
        SSL_write(d->ssl, "\r\n\r\n", 4);
    } else {
        write(d->descriptor, "\r\n", 2);
        write(d->descriptor, tp_leave_mesg, strlen(tp_leave_mesg));
        write(d->descriptor, "\r\n\r\n", 4);
    }

}

static void
idleboot_user(struct descriptor_data * d)
{
    if(d->ssl) {
            SSL_write(d->ssl, "\r\n", 2);
            SSL_write(d->ssl, tp_idle_mesg, strlen(tp_idle_mesg));
            SSL_write(d->ssl, "\r\n\r\n", 4);
    } else {
            write(d->descriptor, "\r\n", 2);
            write(d->descriptor, tp_idle_mesg, strlen(tp_idle_mesg));
            write(d->descriptor, "\r\n\r\n", 4);
    }

    d->booted=1;
}

static int con_players_max = 0;  /* one of Cynbe's good ideas. */
static int con_players_curr = 0;  /* for playermax checks. */
extern void purge_free_frames(void);

time_t current_systime = 0;

void 
shovechars(int portc, int* portv)
{
    fd_set  input_set, output_set;
    long    now, tmptq;
    struct timeval last_slice, current_time;
    struct timeval next_slice;
    struct timeval timeout, slice_timeout;
    int     cnt;  int maxd = 0;
    struct descriptor_data *d, *dnext;
    struct descriptor_data *newd;
    int     avail_descriptors;
    struct timeval sel_in, sel_out;
    int socknum;
    int openfiles_max;

    for (socknum = 0; socknum < portc; socknum++) {
	sock[socknum].s = make_socket(portv[socknum]);
	maxd = sock[socknum].s + 1;
	numsocks++;
    }
    gettimeofday(&last_slice, (struct timezone *) 0);

    openfiles_max =  max_open_files();
    printf("Max FDs = %d\n", openfiles_max);

    avail_descriptors = openfiles_max - 5;

    while (shutdown_flag == 0) {
	gettimeofday(&current_time, (struct timezone *) 0);
	last_slice = update_quotas(last_slice, current_time);

	next_muckevent();
	process_commands();

	for (d = descriptor_list; d; d = dnext) {
	    dnext = d->next;
	    if (d->booted) {
		process_output(d);
		if (d->booted == 2) {
		    goodbye_user(d);
		}
		d->booted = 0;
		process_output(d);
		shutdownsock(d);
	    }
	}
	purge_free_frames();
	untouchprops_incremental(1);

	if (shutdown_flag)
	    break;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	next_slice = msec_add(last_slice, tp_command_time_msec);
	slice_timeout = timeval_sub(next_slice, current_time);

	FD_ZERO(&input_set);
	FD_ZERO(&output_set);
	if (ndescriptors < avail_descriptors) {
	    for (socknum = 0; socknum < numsocks; socknum++) {
		FD_SET(sock[socknum].s, &input_set);
	    }
	}
	for (d = descriptor_list; d; d = d->next) {
	    if (d->input.lines > 100)
		timeout = slice_timeout;
	    else
		FD_SET(d->descriptor, &input_set);
	    if (d->output.head)
		FD_SET(d->descriptor, &output_set);
	}
#ifdef SPAWN_HOST_RESOLVER
        FD_SET(resolver_sock[1], &input_set);
#endif

	tmptq = next_muckevent_time();
	if ((tmptq >= 0L) && (timeout.tv_sec > tmptq)) {
            timeout.tv_sec = tmptq + (tp_pause_min / 1000);
            timeout.tv_usec = (tp_pause_min % 1000) * 1000L;
	}
	gettimeofday(&sel_in,NULL);
	if (select(maxd, &input_set, &output_set,
		   (fd_set *) 0, &timeout) < 0) {
	    if (errno != EINTR) {
		perror("select");
		return;
	    }
	} else {
	    time(&current_systime);
	    gettimeofday(&sel_out,NULL);
	    sel_out.tv_usec -= sel_in.tv_usec;
	    sel_out.tv_sec -= sel_in.tv_sec;
	    if (sel_out.tv_usec < 0) {
	      sel_out.tv_usec += 1000000;
	      sel_out.tv_sec -= 1;
	    }
	    sel_prof_idle_sec += sel_out.tv_sec;
	    sel_prof_idle_usec += sel_out.tv_usec;
	    if (sel_prof_idle_usec >= 1000000) {
	      sel_prof_idle_usec -= 1000000;
	      sel_prof_idle_sec += 1;
	    }
	    sel_prof_idle_use++;
	    (void) time(&now);
	    for (socknum = 0; socknum < numsocks; socknum++) {
		if (FD_ISSET(sock[socknum].s, &input_set)) {
		    if (!(newd = new_connection(sock[socknum].s, portv[socknum],
				sock[socknum].is_ssl))) {
			if (errno
				&& errno != EINTR
				&& errno != EMFILE
				&& errno != ENFILE
			    /*
			     *  && errno != ETIMEDOUT
			     *  && errno != ECONNRESET
			     *  && errno != ENOTCONN
			     *  && errno != EPIPE
			     *  && errno != ECONNREFUSED
			     *#ifdef EPROTO
			     *  && errno != EPROTO
			     *#endif
			     */
				) {
			    perror("new_connection");
			    /* return; */
			}
		    } else {
			if (newd->descriptor >= maxd)
			    maxd = newd->descriptor + 1;
		    }
		}
	    }
#ifdef SPAWN_HOST_RESOLVER
            if (FD_ISSET(resolver_sock[1], &input_set)) {
                resolve_hostnames();
            }
#endif
	    for (cnt = 0, d = descriptor_list; d; d = dnext) {
		dnext = d->next;
		if (!d->ssl_accepted)
		{
		    if (FD_ISSET(d->descriptor, &input_set) 
			|| FD_ISSET(d->descriptor, &output_set)) {
			int ret = SSL_accept(d->ssl);
			if (ret == 1)
			{
			    // Success!
			    d->ssl_accepted = TRUE;
			}
			else if (ret == 0)
			{
			    // Negotiations failed
			    d->booted = 1;
			}
			else
			{
			    switch(SSL_get_error(d->ssl, ret))
			    {
			    case SSL_ERROR_WANT_READ:
			    case SSL_ERROR_WANT_WRITE:
		    		break; // same as EWOULDBLOCK
		
			    default:
		    		// FUUUUUuuuu!
		    		// Need to close the connection...
		    		d->booted = 1;
			    }
			}
		    }
		}
		else
		{
    		    if (FD_ISSET(d->descriptor, &input_set)) {
		        d->last_time = now;
		        if (!process_input(d)) {
			    d->booted = 1;
		        }
		    }
		    if (FD_ISSET(d->descriptor, &output_set)) {
		        if (!process_output(d)) {
			    d->booted = 1;
		        }
		    }
		}
		if (d->connected) {
		    cnt++;
		    if (tp_idleboot&&((now - d->last_time) > tp_maxidle)&&
		            !Wizard(d->player)
		    ) {
		        idleboot_user(d);
		    }
		} else {
		    if ((now - d->connected_at) > 300) {
			d->booted = 1;
		    }
		}
	    }
	    if (cnt > con_players_max) {
		add_property((dbref)0, "_sys/max_connects", NULL, cnt);
		con_players_max = cnt;
	    }
	    con_players_curr = cnt;
	}
    }
    (void) time(&now);
    add_property((dbref)0, "_sys/lastdumptime", NULL, (int)now);
    add_property((dbref)0, "_sys/shutdowntime", NULL, (int)now);
}


void 
wall_and_flush(const char *msg)
{
    struct descriptor_data *d, *dnext;
    char    buf[BUFFER_LEN + 2];

#ifdef COMPRESS
    extern const char *uncompress(const char *);

    msg = uncompress(msg);
#endif				/* COMPRESS */

    if (!msg || !*msg) return;
    strcpy(buf, msg);
    strcat(buf, "\r\n");

    for (d = descriptor_list; d; d = dnext) {
	dnext = d->next;
	queue_string(d, buf);
	/* queue_write(d, "\r\n", 2); */
	if (!process_output(d)) {
	    d->booted = 1;
	}
    }
}


void
flush_user_output(dbref player)
{
    int di;
    int* darr;
    int dcount;
    struct descriptor_data *d;
 
    if (Typeof(player) == TYPE_PLAYER) {
        darr = DBFETCH(player)->sp.player.descrs;
        dcount = DBFETCH(player)->sp.player.descr_count;
        if (!darr) {
            dcount = 0;
        }
    } else {
        dcount = 0;
        darr = NULL;
    }

    for (di = 0; di < dcount; di++) {
        d = descrdata_by_descr(darr[di]);
        if (d && !process_output(d)) {
            d->booted = 1;
        }
    }
}   


void 
wall_wizards(const char *msg)
{
    struct descriptor_data *d, *dnext;
    char    buf[BUFFER_LEN + 2];

#ifdef COMPRESS
    extern const char *uncompress(const char *);

    msg = uncompress(msg);
#endif				/* COMPRESS */

    strcpy(buf, msg);
    strcat(buf, "\r\n");

    for (d = descriptor_list; d; d = dnext) {
	dnext = d->next;
	if (d->connected && Wizard(d->player)) {
	    queue_string(d, buf);
	    if (!process_output(d)) {
		d->booted = 1;
	    }
	}
    }
}


static struct descriptor_data *
new_connection(int sock, int port, int is_ssl)
{
    int     newsock;
    struct sockaddr_in addr;
    size_t     addr_len;
    char    hostname[128];
    int	    optval;

    SSL             *ssl = NULL;
    BIO             *sbio;

    addr_len = sizeof(addr);
    newsock = accept(sock, (struct sockaddr *) & addr, &addr_len);
    if (newsock < 0) {
	return 0;
    } else {
	optval = 1; /* TCP KEEPALIVE ON */
	if(setsockopt(newsock, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0)
	{
            perror("setsockopt()->SO_KEEPALIVE");
	}
	optval = 60 * 5; /* TCP KEEPALIVE IDLE TIME (5 minutes) */
	if(setsockopt(newsock, SOL_TCP, TCP_KEEPIDLE, &optval, sizeof(optval)) < 0)
	{
            perror("setsockopt()->TCP_KEEPIDLE");
	}

        if (is_ssl) {
                sbio = BIO_new_socket(newsock, BIO_NOCLOSE);
		// Try and tell BIO that this is a nonblocking socket.
		BIO_set_nbio(sbio, 1);
                ssl = SSL_new(ctx);
		// Prep socket for nonblocking
		SSL_set_mode(ssl, 
			SSL_MODE_ENABLE_PARTIAL_WRITE|
			SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

                SSL_set_bio(ssl, sbio, sbio);

		/* This is filled with fail. This will leak BIOs and SSLs.
                if (SSL_accept(ssl)<=0) {
                        close(newsock);
                        return 0;
                }
		*/
        }

	strcpy(hostname, addrout(addr.sin_addr.s_addr, addr.sin_port, port));
	log_status("ACCEPT: %s(%d) on descriptor %d\n", hostname,
		    ntohs(addr.sin_port), newsock);
	log_status("CONCOUNT: There are now %d open connections.\n",
		    ndescriptors);
	return initializesock(newsock, hostname, ssl);
    }
}


#ifdef SPAWN_HOST_RESOLVER

void
kill_resolver()
{
    int i;
    pid_t p;

    write(resolver_sock[1], "QUIT\n", 5);
    p = wait(&i);
}



void
spawn_resolver()
{
    socketpair(AF_UNIX, SOCK_STREAM, 0, resolver_sock);
    make_nonblocking(resolver_sock[1]);
    if (!fork()) {
        close(0);
        close(1);
        dup(resolver_sock[0]);
        dup(resolver_sock[0]);
	execl("./resolver", "resolver", NULL);
	execl("./bin/resolver", "resolver", NULL);
	execl("../src/resolver", "resolver", NULL);
	perror("resolver execlp");
	_exit(1);
    }
}


void
resolve_hostnames()
{
    char buf[BUFFER_LEN];
    char *ptr, *ptr2, *ptr3, *hostip, *port, *hostname, *username, *tempptr;
    struct descriptor_data *d;
    int got,dc;

    got = read(resolver_sock[1], buf, sizeof(buf));
    if (got < 0) return;
    if (got == sizeof(buf)) {
	got--;
        while(got>0 && buf[got] != '\n') buf[got--] = '\0';
    }
    ptr = buf;
    dc = 0;
    do {
        for(ptr2 = ptr; *ptr && *ptr != '\n' && dc < got; ptr++,dc++);
        if (*ptr) { *ptr++ = '\0'; dc++; }
        if (*ptr2) {
            ptr3 = index(ptr2, ':');
            if (!ptr3) return;
           hostip = ptr2;
           port = index(ptr2, '(');
           if (!port) return;
           tempptr = index(port, ')');
           if (!tempptr) return;
           *tempptr = '\0';
           hostname = ptr3;
           username = index(ptr3, '(');
           if (!username) return;
           tempptr = index(username, ')');
           if (!tempptr) return;
           *tempptr = '\0';
            if (*port&&*hostname&&*username) {
                *port++ = '\0';
                *hostname++ = '\0';
                *username++ = '\0';
                for (d = descriptor_list; d; d = d->next) {
                    if(!strcmp(d->hostname, hostip)&&
                       !strcmp(d->username, port)) {
                        FREE(d->hostname);
                        FREE(d->username);
                        d->hostname = strsave(hostname);
                        d->username = strsave(username);
                    }
                }
            }
        }
    } while (dc < got && *ptr);
}

#endif


/*  addrout -- Translate address 'a' from int to text.		*/

const char *
addrout(long a, unsigned short prt, unsigned short servport)
{
    static char buf[128];
    struct in_addr addr;
    
    addr.s_addr = a;
    prt = ntohs(prt);
        
#ifndef SPAWN_HOST_RESOLVER
    if (tp_hostnames) {
	/* One day the nameserver Qwest uses decided to start */
	/* doing halfminute lags, locking up the entire muck  */
	/* that long on every connect.  This is intended to   */
	/* prevent that, reduces average lag due to nameserver*/
	/* to 1 sec/call, simply by not calling nameserver if */
	/* it's in a slow mood *grin*. If the nameserver lags */
	/* consistently, a hostname cache ala OJ's tinymuck2.3*/
	/* would make more sense:                             */
	static secs_lost = 0;
	if (secs_lost) {
	    secs_lost--;
	} else {
	    time_t gethost_start = current_systime;
	    struct hostent *he   = gethostbyaddr(((char *)&addr), 
						 sizeof(addr), AF_INET);
	    time_t gethost_stop  = time(&current_systime);
	    time_t lag           = gethost_stop - gethost_start;
	    if (lag > 10) {
		secs_lost = lag;

#if MIN_SECS_TO_LOG
		if (lag >= CFG_MIN_SECS_TO_LOG) {
		    log_status( "GETHOSTBYNAME-RAN: secs %3d\n", lag );
		}
#endif

	    }
	    if (he) {
		sprintf(buf, "%s(%u)", he->h_name, prt);
		return buf;
	    }
	}
    }
#endif /* SPAWN_HOST_RESOLVER */

    a = ntohl(a);

#ifdef SPAWN_HOST_RESOLVER
    sprintf(buf, "%ld.%ld.%ld.%ld(%u)%u\n",
        (a >> 24) & 0xff,
        (a >> 16) & 0xff,
        (a >> 8)  & 0xff,
         a        & 0xff,
	prt, servport
    );
    if (tp_hostnames) {
        write(resolver_sock[1], buf, strlen(buf));
    }
#endif

    sprintf(buf, "%ld.%ld.%ld.%ld(%u)",
        (a >> 24) & 0xff,
        (a >> 16) & 0xff,
        (a >> 8)  & 0xff,
         a        & 0xff, prt
    );
    return buf;
}


static void 
clearstrings(struct descriptor_data * d)
{
    if (d->output_prefix) {
	FREE(d->output_prefix);
	d->output_prefix = 0;
    }
    if (d->output_suffix) {
	FREE(d->output_suffix);
	d->output_suffix = 0;
    }
}

static void
closesock(struct descriptor_data *d)
{
    if (d->ssl != NULL)
    {
	SSL_shutdown(d->ssl);
	SSL_free(d->ssl);
        close(d->descriptor);
    } else {
        shutdown(d->descriptor, 2);
        close(d->descriptor);
    }
}

static void 
shutdownsock(struct descriptor_data * d)
{
    if (d->connected) {
       log_status("DISCONNECT: descriptor %d player %s(%d) from %s(%s)\n",
                  d->descriptor, NAME(d->player), d->player,
                  d->hostname, d->username);
	announce_disconnect(d);
    } else if (d->ssl && !d->ssl_accepted) {
       log_status("DISCONNECT: descriptor %d from %s(%s) SSL negotiations failed.\n",
                  d->descriptor, d->hostname, d->username);
    } else {
       log_status("DISCONNECT: descriptor %d from %s(%s) never connected.\n",
                  d->descriptor, d->hostname, d->username);
    }
    clearstrings(d);
    closesock(d);
    forget_descriptor(d);
    freeqs(d);
    *d->prev = d->next;
    if (d->next)
	d->next->prev = d->prev;
    if (d->hostname)
	free((void *)d->hostname);
    if (d->username)
        free((void *)d->username);
    FREE(d);
    ndescriptors--;
    log_status("CONCOUNT: There are now %d open connections.\n", ndescriptors);
}

static struct descriptor_data *
initializesock(int s, const char *hostname, SSL *ssl)
{
    struct descriptor_data *d;
    char buf[128], *ptr;

    ndescriptors++;
    MALLOC(d, struct descriptor_data, 1);
    d->descriptor = s;
    d->ssl = ssl;
    // For non-ssl sockets, set it to true. Otherwise, false.
    d->ssl_accepted = (ssl == NULL);
    d->connected = 0;
    d->booted = 0;
    d->connected_at = current_systime;
    make_nonblocking(s);
    d->output_prefix = 0;
    d->output_suffix = 0;
    d->output_size = 0;
    d->output.lines = 0;
    d->output.head = 0;
    d->output.tail = &d->output.head;
    d->input.lines = 0;
    d->input.head = 0;
    d->input.tail = &d->input.head;
    d->raw_input = 0;
    d->raw_input_at = 0;
    d->quota = tp_command_burst_size;
    d->last_time = 0;
    strcpy(buf,hostname);
    ptr=index(buf,')');
    if (ptr) *ptr='\0';
    ptr=index(buf,'(');
    *ptr++='\0';
    d->hostname = alloc_string(buf);
    d->username = alloc_string(ptr);
    if (descriptor_list)
	descriptor_list->prev = &d->next;
    d->next = descriptor_list;
    d->prev = &descriptor_list;
    descriptor_list = d;
    remember_descriptor(d);

    welcome_user(d);
    return d;
}

int 
make_socket(int port)
{
    int     s;
    struct sockaddr_in server;
    int     opt;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
	perror("creating stream socket");
	exit(3);
    }
    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
		   (char *) &opt, sizeof(opt)) < 0) {
	perror("setsockopt");
	exit(1);
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    if (bind(s, (struct sockaddr *) & server, sizeof(server))) {
	perror("binding stream socket");
	close(s);
	exit(4);
    }
    listen(s, 5);
    return s;
}

struct text_block *
make_text_block(const char *s, int n)
{
    struct text_block *p;

    MALLOC(p, struct text_block, 1);
    MALLOC(p->buf, char, n);
    bcopy(s, p->buf, n);
    p->nchars = n;
    p->start = p->buf;
    p->nxt = 0;
    return p;
}

void 
free_text_block(struct text_block * t)
{
    FREE(t->buf);
    FREE((char *) t);
}

static void 
add_to_queue(struct text_queue * q, const char *b, int n)
{
    struct text_block *p;

    if (n == 0)
	return;

    p = make_text_block(b, n);
    p->nxt = 0;
    *q->tail = p;
    q->tail = &p->nxt;
    q->lines++;
}

static int 
flush_queue(struct text_queue * q, int n)
{
    struct text_block *p;
    int     really_flushed = 0;

    n += strlen(flushed_message);

    while (n > 0 && (p = q->head)) {
	n -= p->nchars;
	really_flushed += p->nchars;
	q->head = p->nxt;
	q->lines--;
	free_text_block(p);
    }
    p = make_text_block(flushed_message, strlen(flushed_message));
    p->nxt = q->head;
    q->head = p;
    q->lines++;
    if (!p->nxt)
	q->tail = &p->nxt;
    really_flushed -= p->nchars;
    return really_flushed;
}

static int 
queue_write(struct descriptor_data * d, const char *b, int n)
{
    int     space;

    space = MAX_OUTPUT - d->output_size - n;
    if (space < 0)
	d->output_size -= flush_queue(&d->output, -space);
    add_to_queue(&d->output, b, n);
    d->output_size += n;
    return n;
}

static int 
queue_string(struct descriptor_data * d, const char *s)
{
    return queue_write(d, s, strlen(s));
}

static int 
process_output(struct descriptor_data * d)
{
    struct text_block **qp, *cur;
    int     cnt;

    /* drastic, but this may give us crash test data */
    if (!d || !d->descriptor) {
        fprintf(stderr, "process_output: bad descriptor or connect struct!\n");
        abort();
    }

    if (d->output.lines == 0) {
        return 1;
    }

    for (qp = &d->output.head; (cur = *qp);)
    {
        if (d->ssl) 
	{
            cnt = SSL_write(d->ssl, cur->start, cur->nchars);
            if (cnt <= 0)
	    {
		switch(SSL_get_error(d->ssl, cnt))
		{
		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
		    return 1; // same as EWOULDBLOCK
		
		default:
		    // FUUUUUuuuu!
		    // Need to close the connection...
		    return 0;
	        }
	    }
	    
        }
        else
	{
	    cnt = write(d->descriptor, cur->start, cur->nchars);

    	    if (cnt < 0) {
	        if (errno == EWOULDBLOCK)
		    return 1;
	        return 0;
	    }
	}

	d->output_size -= cnt;
	if (cnt == cur->nchars) {
	    d->output.lines--;
	    if (!cur->nxt) {
		d->output.tail = qp;
		d->output.lines = 0;
	    }
	    *qp = cur->nxt;
	    free_text_block(cur);
	    continue;		/* do not adv ptr */
	}
	cur->nchars -= cnt;
	cur->start += cnt;
	break;
    }
    return 1;
}

void 
make_nonblocking(int s)
{
#if !defined(O_NONBLOCK) || defined(ULTRIX)	/* POSIX ME HARDER */
# ifdef FNDELAY 	/* SUN OS */
#  define O_NONBLOCK FNDELAY 
# else
#  ifdef O_NDELAY 	/* SyseVil */
#   define O_NONBLOCK O_NDELAY
#  endif /* O_NDELAY */
# endif /* FNDELAY */
#endif

    if (fcntl(s, F_SETFL, O_NONBLOCK) == -1) {
	perror("make_nonblocking: fcntl");
	panic("O_NONBLOCK fcntl failed");
    }
}

static void 
freeqs(struct descriptor_data * d)
{
    struct text_block *cur, *next;

    cur = d->output.head;
    while (cur) {
	next = cur->nxt;
	free_text_block(cur);
	cur = next;
    }
    d->output.lines = 0;
    d->output.head = 0;
    d->output.tail = &d->output.head;

    cur = d->input.head;
    while (cur) {
	next = cur->nxt;
	free_text_block(cur);
	cur = next;
    }
    d->input.lines = 0;
    d->input.head = 0;
    d->input.tail = &d->input.head;

    if (d->raw_input)
	FREE(d->raw_input);
    d->raw_input = 0;
    d->raw_input_at = 0;
}

char   *
strsave(const char *s)
{
    char   *p;

    MALLOC(p, char, strlen(s) + 1);

    if (p)
	strcpy(p, s);
    return p;
}

static void 
save_command(struct descriptor_data * d, const char *command)
{
    if (d->connected && !string_compare((char *) command, BREAK_COMMAND)) {
	if (dequeue_prog(d->player, 2))
	    notify(d->player, "Foreground program aborted.");
	DBFETCH(d->player)->sp.player.block = 0;
	if (!(FLAGS(d->player) & INTERACTIVE))
	    return;
    }
    add_to_queue(&d->input, command, strlen(command) + 1);
}

static int 
process_input(struct descriptor_data * d)
{
    char    buf[MAX_COMMAND_LEN * 2];
    int     got;
    char    *p, *pend, *q, *qend;

    if(d->ssl) {
        got = SSL_read(d->ssl, buf, sizeof buf);
        if (got <= 0)
        {
	    switch(SSL_get_error(d->ssl, got))
	    {
	    case SSL_ERROR_WANT_READ:
	    case SSL_ERROR_WANT_WRITE:
	        return 1; // same as EWOULDBLOCK
		
	    default:
	        // FUUUUUuuuu!
	        // Need to close the connection...
	        return 0;
	    }
        }
    } else {
        got = read(d->descriptor, buf, sizeof buf);
        if (got <= 0)
            return 0; // OI error, kick them off!
    }
    if (!d->raw_input) {
	MALLOC(d->raw_input, char, MAX_COMMAND_LEN);
	d->raw_input_at = d->raw_input;
    }
    p = d->raw_input_at;
    pend = d->raw_input + MAX_COMMAND_LEN - 1;
    for (q = buf, qend = buf + got; q < qend; q++) {
	if (*q == '\n') {
	    *p = '\0';
	    if (p > d->raw_input)
		save_command(d, d->raw_input);
	    p = d->raw_input;
	} else if (p < pend && isascii(*q)) {
	    if (isprint(*q)) {
		*p++ = *q;
	    } else if (*q == '\t') {
		*p++ = ' ';
	    } else if (*q == 8 || *q == 127) {
		/* if BS or DEL, delete last character */
		if (p > d->raw_input) p--;
	    }
	}
    }
    if (p > d->raw_input) {
	d->raw_input_at = p;
    } else {
	FREE(d->raw_input);
	d->raw_input = 0;
	d->raw_input_at = 0;
    }
    return 1;
}

void 
set_userstring(char **userstring, const char *command)
{
    if (*userstring) {
	FREE(*userstring);
	*userstring = 0;
    }
    while (*command && isascii(*command) && isspace(*command))
	command++;
    if (*command)
	*userstring = strsave(command);
}

void 
process_commands(void)
{
    int     nprocessed;
    struct descriptor_data *d, *dnext;
    struct text_block *t;

    do {
	nprocessed = 0;
	for (d = descriptor_list; d; d = dnext) {
	    dnext = d->next;
	    if (d->quota > 0 && (t = d->input.head)
	       && (!(d->connected && DBFETCH(d->player)->sp.player.block))) {
		d->quota--;
		nprocessed++;
		if (!do_command(d, t->start)) {
		    d->booted = 2;
		    /* process_output(d); */
		    /* shutdownsock(d);  */
		}
		/* start former else block */
		d->input.head = t->nxt;
		d->input.lines--;
		if (!d->input.head) {
		    d->input.tail = &d->input.head;
		    d->input.lines = 0;
		}
		free_text_block(t);
		/* end former else block */
	    }
	}
    } while (nprocessed > 0);
}

static int 
do_command(struct descriptor_data * d, char *command)
{
    if (d->connected)
	ts_lastuseobject(d->player);
    if (!strcmp(command, QUIT_COMMAND)) {
	return 0;
    } else if (!strncmp(command, WHO_COMMAND, sizeof(WHO_COMMAND) - 1)) {
	char buf[BUFFER_LEN];
	if (d->output_prefix) {
	    queue_string(d, d->output_prefix);
	    queue_write(d, "\r\n", 2);
	}
	strcpy(buf, "@");
	strcat(buf, WHO_COMMAND);
	strcat(buf, " ");
	strcat(buf, command + sizeof(WHO_COMMAND) - 1);
	if (!d->connected || (FLAGS(d->player) & INTERACTIVE)) {
	    if (tp_secure_who) {
		queue_string(d,"Sorry, WHO is unavailable at this point.\r\n");
	    } else {
		dump_users(d, command + sizeof(WHO_COMMAND) - 1);
	    }
	} else {
	    if (can_move(d->player, buf, 2)) {
		do_move(d->player, buf, 2);
	    } else {
		dump_users(d, command + sizeof(WHO_COMMAND) - 1);
	    }
	}
	if (d->output_suffix) {
	    queue_string(d, d->output_suffix);
	    queue_write(d, "\r\n", 2);
	}
    } else if (!strncmp(command, PREFIX_COMMAND, sizeof(PREFIX_COMMAND) - 1)) {
	set_userstring(&d->output_prefix, command + sizeof(PREFIX_COMMAND) - 1);
    } else if (!strncmp(command, SUFFIX_COMMAND, sizeof(SUFFIX_COMMAND) - 1)) {
	set_userstring(&d->output_suffix, command + sizeof(SUFFIX_COMMAND) - 1);
    } else {
	if (d->connected) {
	    if (d->output_prefix) {
		queue_string(d, d->output_prefix);
		queue_write(d, "\r\n", 2);
	    }
	    process_command(d->player, command);
	    if (d->output_suffix) {
		queue_string(d, d->output_suffix);
		queue_write(d, "\r\n", 2);
	    }
	} else {
	    check_connect(d, command);
	}
    }
    return 1;
}

void 
interact_warn(dbref player)
{
    if (FLAGS(player) & INTERACTIVE) {
	char    buf[BUFFER_LEN];

	sprintf(buf, "***  %s  ***",
		(FLAGS(player) & READMODE) ?
		"You are currently using a program.  Use \"@Q\" to return to a more reasonable state of control." :
		(DBFETCH(player)->sp.player.insert_mode ?
		 "You are currently inserting MUF program text.  Use \".\" to return to the editor, then \"quit\" if you wish to return to your regularly scheduled Muck universe." :
		 "You are currently using the MUF program editor."));
	notify(player, buf);
    }
}

static void 
check_connect(struct descriptor_data * d, const char *msg)
{
    char    command[MAX_COMMAND_LEN];
    char    user[MAX_COMMAND_LEN];
    char    password[MAX_COMMAND_LEN];
    dbref   player;

    parse_connect(msg, command, user, password);

    if (!strncmp(command, "co", 2)) {
	player = connect_player(user, password);
	if (player == NOTHING) {
	    queue_string(d, connect_fail);
	    log_status("FAILED CONNECT %s on descriptor %d\n",
		       user, d->descriptor);
	} else {
	    if ((wizonly_mode ||
	         (tp_playermax && con_players_curr >= tp_playermax_limit)) &&
	        !TrueWizard(player)
	    ) {
		if (wizonly_mode) {
		    queue_string(d, "Sorry, but the game is in maintenance mode currently, and only wizards are allowed to connect.  Try again later.");
		} else {
		    queue_string(d, tp_playermax_bootmesg);
		}
		queue_string(d, "\r\n");
		d->booted = 1;
	    } else {
		log_status("CONNECTED: %s(%d) on descriptor %d\n",
			   NAME(player), player, d->descriptor);
		d->connected = 1;
		d->connected_at = current_systime;
		d->player = player;
		update_desc_count_table();
                remember_player_descr(player, d->descriptor);
		/* cks: someone has to initialize this somewhere. */
		DBFETCH(d->player)->sp.player.block = 0;
		spit_file(player, MOTD_FILE);
		announce_connect(player);
		interact_warn(player);
		if (sanity_violated && Wizard(player)) {
		    notify(player, "#########################################################################");
		    notify(player, "## WARNING!  The DB appears to be corrupt!  Please repair immediately! ##");
		    notify(player, "#########################################################################");
		}
		con_players_curr++;
	    }
	}
    } else if (!strncmp(command, "cr", 2)) {
	if (!tp_registration) {
	    if (wizonly_mode || (tp_playermax && con_players_curr >= tp_playermax_limit)) {
		if (wizonly_mode) {
		    queue_string(d, "Sorry, but the game is in maintenance mode currently, and only wizards are allowed to connect.  Try again later.");
		} else {
		    queue_string(d, tp_playermax_bootmesg);
		}
		queue_string(d, "\r\n");
		d->booted = 1;
	    } else {
		player = create_player(user, password);
		if (player == NOTHING) {
		    queue_string(d, create_fail);
		    log_status("FAILED CREATE %s on descriptor %d\n",
			       user, d->descriptor);
		} else {
		    log_status("CREATED %s(%d) on descriptor %d\n",
			       NAME(player), player, d->descriptor);
		    d->connected = 1;
		    d->connected_at = current_systime;
		    d->player = player;
		    update_desc_count_table();
                    remember_player_descr(player, d->descriptor);
		    /* cks: someone has to initialize this somewhere. */
		    DBFETCH(d->player)->sp.player.block = 0;
		    spit_file(player, MOTD_FILE);
		    announce_connect(player);
		    con_players_curr++;
		}
	    }
	} else {
	    queue_string(d, tp_register_mesg);
	    queue_string(d, "\r\n");
	    log_status("FAILED CREATE %s on descriptor %d\n",
		       user, d->descriptor);
        }
    } else if (!strncmp(command, "VERIFY", 6)) {
        char msgbuf[BUFFER_LEN];

	player = connect_player(user, password);
	if (player == NOTHING) {
	    strcpy(msgbuf, "VERIFICATION FAIL Invalid username or password.");
	} else {
	    if (strcmp(NAME(player), user) == 0)
	        sprintf(msgbuf, "VERIFICATION OK #%ld", (long int)player);
	    else
	        strcpy(msgbuf, "VERIFICATION FAIL Verification requires correct upper and lower case in names.");
	}
	strcat(msgbuf, "\r\n");
	queue_string(d, msgbuf);
	d->booted = 1;
    } else {
	welcome_user(d);
    }
}

void 
parse_connect(const char *msg, char *command, char *user, char *pass)
{
    char   *p;

    while (*msg && isascii(*msg) && isspace(*msg))
	msg++;
    p = command;
    while (*msg && isascii(*msg) && !isspace(*msg))
	*p++ = *msg++;
    *p = '\0';
    while (*msg && isascii(*msg) && isspace(*msg))
	msg++;
    p = user;
    while (*msg && isascii(*msg) && !isspace(*msg))
	*p++ = *msg++;
    *p = '\0';
    while (*msg && isascii(*msg) && isspace(*msg))
	msg++;
    p = pass;
    while (*msg && isascii(*msg) && !isspace(*msg))
	*p++ = *msg++;
    *p = '\0';
}


int 
boot_off(dbref player)
{
    /* WORK: use player.descrs */
    struct descriptor_data *d;
    struct descriptor_data *last = NULL;

    for (d = descriptor_list; d; d = d->next)
	if (d->connected && d->player == player)
	    last = d;

    if (last) {
	process_output(last);
	last->booted = 1;
	/* shutdownsock(last); */
	return 1;
    }
    return 0;
}

void 
boot_player_off(dbref player)
{
    int di;
    int* darr;
    int dcount;
    struct descriptor_data *d;
 
    if (Typeof(player) == TYPE_PLAYER) {
        darr = DBFETCH(player)->sp.player.descrs;
        dcount = DBFETCH(player)->sp.player.descr_count;
        if (!darr) {
            dcount = 0;
        }
    } else {
        dcount = 0;
        darr = NULL;
    }

    for (di = 0; di < dcount; di++) {
        d = descrdata_by_descr(darr[di]);
        if (d) {
            d->booted = 1;
        }
    }
}


void
close_sockets(const char *msg)
{
    struct descriptor_data *d, *dnext;
    int socknum;

    for (d = descriptor_list; d; d = dnext) {
        dnext = d->next;
        if(d->ssl) {
                SSL_write(d->ssl, msg, strlen(msg));
                SSL_write(d->ssl, shutdown_message, strlen(shutdown_message));
        } else {
                write(d->descriptor, msg, strlen(msg));
                write(d->descriptor, shutdown_message, strlen(shutdown_message));
        }
	closesock(d);
        clearstrings(d);			/** added to clean up **/
        freeqs(d);				/****/
        *d->prev = d->next;			/****/
        if (d->next)				/****/
            d->next->prev = d->prev;		/****/
	if (d->hostname)                        /****/
	    free((void *)d->hostname);          /****/
        if (d->username)                        /****/
            free((void *)d->username);          /****/
        FREE(d);				/****/
        ndescriptors--;				/****/
    }
    for (socknum = 0; socknum < numsocks; socknum++) {
	close(sock[socknum].s);
    }
}


void
do_armageddon(dbref player, const char *msg)
{
    char buf[BUFFER_LEN];
    if (!Wizard(player)) {
	notify(player, "Sorry, but you don't look like the god of War to me.");
	return;
    }
    sprintf(buf, "\r\nImmediate shutdown initiated by %s.\r\n", NAME(player));
    if (msg || *msg)
	strcat(buf, msg);
    log_status("ARMAGEDDON initiated by %s(%d): %s\n",
	    NAME(player), player, msg);
    fprintf(stderr, "ARMAGEDDON initiated by %s(%d): %s\n",
	    NAME(player), player, msg);
    close_sockets(buf);

#ifdef SPAWN_HOST_RESOLVER
    kill_resolver();
#endif

    exit(1);
}


void 
emergency_shutdown(void)
{
    close_sockets("\r\nEmergency shutdown due to server crash.");

#ifdef SPAWN_HOST_RESOLVER
    kill_resolver();
#endif

}


static void 
dump_users(struct descriptor_data * e, char *user)
{
    struct descriptor_data *d;
    int     wizard, players;
    long    now;
    char    buf[2048];
    char    pbuf[64];
#ifdef COMPRESS
    extern const char *uncompress(const char *);
#endif

    if (tp_who_doing) {
	wizard = e->connected && God(e->player);
    } else {
	wizard = e->connected && Wizard(e->player);
    }

    while (*user && (isspace(*user) || *user == '*')) {
        if (tp_who_doing && *user == '*' && e->connected && Wizard(e->player))
	    wizard = 1;
	user++;
    }

    if (wizard)
	/* S/he is connected and not quelled. Okay; log it. */
	log_command("WIZ: %s(%d) in %s(%d):  %s\n", NAME(e->player),
		    (int) e->player, NAME(DBFETCH(e->player)->location),
		    (int) DBFETCH(e->player)->location, "WHO");
    
    if (!*user) user = NULL;

    (void) time(&now);
    if (wizard) {
	queue_string(e, "Player Name                Location     On For Idle  Host\r\n");
    } else {
	if (tp_who_doing) {
	    queue_string(e, "Player Name           On For Idle  Doing...\r\n");
	} else {
	    queue_string(e, "Player Name           On For Idle\r\n");
	}
    }

    d = descriptor_list;
    players = 0;
    while (d) {
	if (d->connected &&
		(!tp_who_hides_dark ||
		    (wizard || !(FLAGS(d->player) & DARK))) &&
		++players &&
		(!user || string_prefix(NAME(d->player), user))
		) {
	    if (wizard) {
		/* don't print flags, to save space */
		sprintf(pbuf, "%.*s(#%d)", PLAYER_NAME_LIMIT + 1,
			NAME(d->player), (int) d->player);
#ifdef GOD_PRIV
               if (!God(e->player))
                       sprintf(buf, "%-*s [%6d] %10s %4s%s %s\r\n",
                               PLAYER_NAME_LIMIT + 10, pbuf,
                               (int) DBFETCH(d->player)->location,
                               time_format_1(now - d->connected_at),
                               time_format_2(now - d->last_time),
                               ((FLAGS(d->player) & INTERACTIVE) ? "*" : " "),
                               d->hostname);
               else
#endif
                       sprintf(buf, "%-*s [%6d] %10s %4s%s %s(%s)\r\n",
                               PLAYER_NAME_LIMIT + 10, pbuf,
                               (int) DBFETCH(d->player)->location,
                               time_format_1(now - d->connected_at),
                               time_format_2(now - d->last_time),
                               ((FLAGS(d->player) & INTERACTIVE) ? "*" : " "),
                               d->hostname, d->username);
	    } else {
		if (tp_who_doing) {
		    sprintf(buf, "%-*s %10s %4s%s %-44s\r\n",
			    PLAYER_NAME_LIMIT + 1,
			    NAME(d->player),
			    time_format_1(now - d->connected_at),
			    time_format_2(now - d->last_time),
			    ((FLAGS(d->player) & INTERACTIVE) ? "*" : " "),
			    GETDOING(d->player) ?
#ifdef COMPRESS
				uncompress(GETDOING(d->player))
#else
				GETDOING(d->player)
#endif
				: ""
			   );
		} else {
		    sprintf(buf, "%-*s %10s %4s%s\r\n",
			    PLAYER_NAME_LIMIT + 1,
			    NAME(d->player),
			    time_format_1(now - d->connected_at),
			    time_format_2(now - d->last_time),
			    ((FLAGS(d->player) & INTERACTIVE) ? "*" : " "));
		}
	    }
	    queue_string(e, buf);
	}
	d = d->next;
    }
    if (players > con_players_max) con_players_max = players;
    sprintf(buf, "%d player%s %s connected.  (Max was %d)\r\n", players,
	    (players == 1) ? "" : "s",
	    (players == 1) ? "is" : "are",
	    con_players_max);
    queue_string(e, buf);
}

char   *
time_format_1(long dt)
{
    register struct tm *delta;
    static char buf[64];

    delta = gmtime(&dt);
    if (delta->tm_yday > 0)
	sprintf(buf, "%dd %02d:%02d",
		delta->tm_yday, delta->tm_hour, delta->tm_min);
    else
	sprintf(buf, "%02d:%02d",
		delta->tm_hour, delta->tm_min);
    return buf;
}

char   *
time_format_2(long dt)
{
    register struct tm *delta;
    static char buf[64];

    delta = gmtime(&dt);
    if (delta->tm_yday > 0)
	sprintf(buf, "%dd", delta->tm_yday);
    else if (delta->tm_hour > 0)
	sprintf(buf, "%dh", delta->tm_hour);
    else if (delta->tm_min > 0)
	sprintf(buf, "%dm", delta->tm_min);
    else
	sprintf(buf, "%ds", delta->tm_sec);
    return buf;
}


void
announce_puppets(dbref player, const char *msg, const char *prop)
{
    dbref what, where;
    const char *ptr, *msg2;
    char buf[BUFFER_LEN];

    for (what = 0; what < db_top; what++) {
	if (Typeof(what) == TYPE_THING && (FLAGS(what) & ZOMBIE)) {
	    if (OWNER(what) == player) {
		where = getloc(what);
		if ((!Dark(where)) && (!Dark(player)) && (!Dark(what))) {
		    msg2 = msg;
		    if ((ptr = (char *)get_property_class(what, prop)) && *ptr)
			msg2 = ptr;
		    sprintf(buf, "%.512s %.3000s", PNAME(what), msg2);
		    notify_except(DBFETCH(where)->contents, what, buf, what);
		}
	    }
	}
    }
}

void 
announce_connect(dbref player)
{
    dbref   loc;
    char    buf[BUFFER_LEN];
    struct match_data md;
    dbref   exit;
    /* dbref   tmploc; */
    /* char   *tmpchar; */
    /* int     the_prog; */
    time_t tt;

    if ((loc = getloc(player)) == NOTHING)
	return;

    if (tp_rwho) {
	time(&tt);
	sprintf(buf, "%d@%s", player, tp_muckname);
	rwhocli_userlogin(buf, NAME(player), tt);
    }

    if ((!Dark(player)) && (!Dark(loc))) {
	sprintf(buf, "%s has connected.", PNAME(player));
	notify_except(DBFETCH(loc)->contents, player, buf, player);
    }

    exit = NOTHING;
    if (online(player) == 1) {
	init_match(player, "connect", TYPE_EXIT, &md);	/* match for connect */
	md.match_level = 1;
	match_all_exits(&md);
	exit = match_result(&md);
	if (exit == AMBIGUOUS) exit = NOTHING;
    }

    if (exit == NOTHING || !(FLAGS(exit) & STICKY))	{

        ts_useobject(getloc(player));

	if (can_move(player, "look", 1)) {
	    do_move(player, "look", 1);
	} else {
	    do_look_around (player);
	}
    }


    /*
     * See if there's a connect action.  If so, and the player is the first to
     * connect, send the player through it.  If the connect action is set
     * sticky, then suppress the normal look-around.
     */

    if (exit != NOTHING) do_move(player, "connect", 1);

    if (online(player) == 1) {
	announce_puppets(player, "wakes up.", "_/pcon");
    }

    /* queue up all _connect programs referred to by properties */
    envpropqueue(player, getloc(player), NOTHING, player, NOTHING,
		 "_connect", "Connect", 1, 1);
    envpropqueue(player, getloc(player), NOTHING, player, NOTHING,
		 "_oconnect", "Oconnect", 1, 0);

    ts_useobject(player);
    return;
}

static void 
announce_disconnect(struct descriptor_data *d)
{
    dbref   player = d->player;
    dbref   loc;
    /* dbref   tmploc; */
    char    buf[BUFFER_LEN];
    /* char   *tmpchar; */
    /* int     the_prog; */

    if ((loc = getloc(player)) == NOTHING)
	return;

    if (tp_rwho) {
	sprintf(buf, "%d@%s", player, tp_muckname);
	rwhocli_userlogout(buf);
    }

    if (dequeue_prog(player, 2))
	notify(player, "Foreground program aborted.");

    if ((!Dark(player)) && (!Dark(loc))) {
	sprintf(buf, "%s has disconnected.", PNAME(player));
	notify_except(DBFETCH(loc)->contents, player, buf, player);
    }

    /* trigger local disconnect action */
    if (online(player) == 1) {
	if (can_move(player, "disconnect", 1)) {
	    do_move(player, "disconnect", 1);
	}
	announce_puppets(player, "falls asleep.", "_/pdcon");
    }

    d->connected = 0;
    d->player = NOTHING;
    forget_player_descr(player, d->descriptor);
    update_desc_count_table();

    /* queue up all _connect programs referred to by properties */
    envpropqueue(player, getloc(player), NOTHING, player, NOTHING,
		 "_disconnect", "Disconnect", 1, 1);
    envpropqueue(player, getloc(player), NOTHING, player, NOTHING,
		 "_odisconnect", "Odisconnect", 1, 0);

    ts_lastuseobject(player);
    DBDIRTY(player);
}

#ifdef MUD_ID
void
do_setuid(char *name)
{
#include <pwd.h>
    struct passwd *pw;

    if ((pw = getpwnam(name)) == NULL) {
	log_status("can't get pwent for %s\n", name);
	exit(1);
    }
    if (setuid(pw->pw_uid) == -1) {
	log_status("can't setuid(%d): ", pw->pw_uid);
	perror("setuid");
	exit(1);
    }
}

#endif				/* MUD_ID */

struct descriptor_data *descr_count_table[FD_SETSIZE];
int current_descr_count = 0;

void
init_descr_count_lookup()
{
    int i;
    for (i = 0; i < FD_SETSIZE; i++) {
	descr_count_table[i] = NULL;
    }
}

void
update_desc_count_table()
{
    int c;
    struct descriptor_data *d;

    current_descr_count = 0;
    for (c = 0, d = descriptor_list; d; d = d->next)
    {
        if (d->connected)
	{
	    descr_count_table[c++] = d;
	    current_descr_count++;
	}
    }
}

static struct descriptor_data *
descrdata_by_count(int c)
{
    c--;
    if (c >= current_descr_count || c < 0) {
        return NULL;
    }
    return descr_count_table[c];
}

struct descriptor_data *descr_lookup_table[FD_SETSIZE];

void
init_descriptor_lookup()
{
    int i;
    for (i = 0; i < FD_SETSIZE; i++) {
	descr_lookup_table[i] = NULL;
    }
}

void
remember_player_descr(dbref player, int descr)
{
    int  count = DBFETCH(player)->sp.player.descr_count;
    int* arr   = DBFETCH(player)->sp.player.descrs;

    if (!arr) {
        arr = (int*)malloc(sizeof(int));
        arr[0] = descr;
        count = 1;
    } else {
        arr = (int*)realloc(arr,sizeof(int) * (count+1));
        arr[count] = descr;
        count++;
    }
    DBFETCH(player)->sp.player.descr_count = count;
    DBFETCH(player)->sp.player.descrs = arr;
}

void
forget_player_descr(dbref player, int descr)
{
    int  count = DBFETCH(player)->sp.player.descr_count;
    int* arr   = DBFETCH(player)->sp.player.descrs;

    if (!arr) {
        count = 0;
    } else if (count > 1) {
	int src, dest;
        for (src = dest = 0; src < count; src++) {
	    if (arr[src] != descr) {
		if (src != dest) {
		    arr[dest] = arr[src];
		}
		dest++;
	    }
	}
	if (dest != count) {
	    count = dest;
	    arr = (int*)realloc(arr,sizeof(int) * count);
        }
    } else {
        free((void*)arr);
        arr = NULL;
        count = 0;
    }
    DBFETCH(player)->sp.player.descr_count = count;
    DBFETCH(player)->sp.player.descrs = arr;
}

static void
remember_descriptor(struct descriptor_data *d)
{
    if (d) {
	descr_lookup_table[d->descriptor] = d;
    }
}

static void
forget_descriptor(struct descriptor_data *d)
{
    if (d) {
	descr_lookup_table[d->descriptor] = NULL;
    }
}

static struct descriptor_data *
lookup_descriptor(int c)
{
    if (c >= FD_SETSIZE || c < 0) {
        return NULL;
    }
    return descr_lookup_table[c];
}

static struct descriptor_data *
descrdata_by_descr(int i)
{
    return lookup_descriptor(i);
}

/*** JME ***/
int 
online(dbref player)
{
    return DBFETCH(player)->sp.player.descr_count;
}

int 
pcount()
{
    return current_descr_count;
}

int 
pidle(int c)
{
    struct descriptor_data *d;
    long    now;

    d = descrdata_by_count(c);

    (void) time(&now);
    if (d) {
	return (now - d->last_time);
    }

    return -1;
}

dbref 
pdbref(int c)
{
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
	return (d->player);
    }

    return NOTHING;
}

int 
pontime(int c)
{
    struct descriptor_data *d;
    long    now;

    d = descrdata_by_count(c);

    (void) time(&now);
    if (d) {
	return (now - d->connected_at);
    }

    return -1;
}

char   *
phost(int c)
{
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
	return ((char *) d->hostname);
    }

    return (char *) NULL;
}

char   *
puser(int c)
{
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
	return ((char *) d->username);
    }

    return (char *) NULL;
}

/*** Foxen ***/
int
pfirstconn(dbref who)
{
    struct descriptor_data *d;
    int count = 1;
    for (d = descriptor_list; d; d = d->next) {
	if (d->connected) {
	    if (d->player == who) return count;
	    count++;
	}
    }
    return 0;
}


void 
pboot(int c)
{
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
	process_output(d);
	d->booted = 1;
	/* shutdownsock(d); */
    }
}

void 
pnotify(int c, char *outstr)
{
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
	queue_string(d, outstr);
	queue_write(d, "\r\n", 2);
    }
}


int 
pdescr(int c)
{
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) return (d->descriptor);

    return -1;
}


int 
pnextdescr(int c)
{
    struct descriptor_data *d;

    d = descrdata_by_descr(c);
    if (d) d = d->next;
    while (d && (!d->connected)) d = d->next;
    if (d) return (d->descriptor);
    return (0);
}


int 
pdescrcon(int c)
{
    struct descriptor_data *d;
    int     i = 1;

    d = descriptor_list;
    while (d && (d->descriptor != c)) {
	if (d->connected) i++;
	d = d->next;
    }
    if (d && d->connected) {
	return (i);
    }
    return (0);
}


int 
pset_user(int c, dbref who)
{
    struct descriptor_data *d;

    d = descrdata_by_descr(c);
    if (d && d->connected) {
	announce_disconnect(d);
	if (who != NOTHING) {
	    d->player = who;
	    d->connected = 1;
	    update_desc_count_table();
            remember_player_descr(who, d->descriptor);
	    announce_connect(who);
	}
	return 1;
    }
    return 0;
}


dbref 
partial_pmatch(const char *name)
{
    struct descriptor_data *d;
    dbref last = NOTHING;

    d = descriptor_list;
    while (d) {
	if (d->connected && (last != d->player) &&
	        string_prefix(NAME(d->player), name)) {
	    if (last != NOTHING) {
		last = AMBIGUOUS;
		break;
	    }
	    last = d->player;
	}
	d = d->next;
    }
    return (last);
}


void
update_rwho()
{
    struct descriptor_data *d;
    char buf[BUFFER_LEN];

    rwhocli_pingalive();
    d = descriptor_list;
    while (d) {
	if (d->connected) {
	    sprintf(buf, "%d@%s", d->player, tp_muckname);
	    rwhocli_userlogin(buf, NAME(d->player), d->connected_at);
	}
	d = d->next;
    }
}


static void 
welcome_user(struct descriptor_data * d)
{
    FILE   *f;
    char   *ptr;
    char    buf[BUFFER_LEN];

    if ((f = fopen(WELC_FILE, "r")) == NULL) {
	queue_string(d, DEFAULT_WELCOME_MESSAGE);
	perror("spit_file: welcome.txt");
    } else {
	while (fgets(buf, sizeof buf, f)) {
	    ptr = index(buf, '\n');
	    if (ptr && ptr > buf && *(ptr-1) != '\r') {
	        *ptr++ = '\r';
	        *ptr++ = '\n';
	        *ptr++ = '\0';
	    }
	    queue_string(d, buf);
	}
	fclose(f);
    }
    if (wizonly_mode) {
	queue_string(d, "## The game is currently in maintenance mode, and only wizards will be able to connect.\r\n");
    } else if (tp_playermax && con_players_curr >= tp_playermax_limit) {
	if (tp_playermax_warnmesg && *tp_playermax_warnmesg) {
	    queue_string(d, tp_playermax_warnmesg);
	    queue_string(d, "\r\n");
	}
    }
}

void dump_status()
{    
    struct descriptor_data *d;
    long    now;
    char    buf[BUFFER_LEN];

    (void) time(&now);
    log_status("STATUS REPORT:\n");
    for (d = descriptor_list; d; d = d->next) {
	if (d->connected) {
           sprintf(buf, "PLAYING descriptor %d player %s(%d) from host %s(%s), %s.\n",
                   d->descriptor, NAME(d->player), d->player, d->hostname, d->username,
		    (d->last_time) ? "idle %d seconds" : "never used");
	} else {
           sprintf(buf, "CONNECTING descriptor %d from host %s(%s), %s.\n",
                   d->descriptor, d->hostname, d->username,
		    (d->last_time) ? "idle %d seconds" : "never used");
	}
	log_status(buf, now - d->last_time);
    }
}
