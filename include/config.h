/*
 * config.h
 *
 * Tunable parameters -- Edit to you heart's content 
 *
 * Parameters that control system behavior, and tell the system
 * what resources are available (most of this is now done by
 * the configure script).
 *
 * Most of the goodies that used to be here are now in @tune.
 */
#include "copyright.h"

/************************************************************************
   Administrative Options 

   Various things that affect how the muck operates and what privs
 are compiled in.
 ************************************************************************/

/* Detaches the process as a daemon so that it don't cause problems
 * keeping a terminal line open and such. Logs normal output to a file
 * and writes out a netmuck.pid file 
 */
#undef DETACH

/* VampMUCK's Anonimity system -- It works on its own but it's pretty useless
 * without the proper MUF code. 
 */
#undef ANONYMITY

/* Makes God (#1) immune to @force, @newpassword, and being set !Wizard.  
 */
#define GOD_PRIV

/* Use to compress string data (recomended)
 */
#define COMPRESS

/* To use a simple disk basing scheme where properties aren't loaded
 * from the input file until they are needed, define this. 
 */
#define DISKBASE

/* To make the server save using fast delta dumps that only write out the
 * changed objects, except when @dump or @shutdown are used, or when too
 * many deltas have already been saved to disk, #define this. 
 */
#define DELTADUMPS

/*
 * Port where tinymuck lives -- Note: If you use a port lower than
 * 1024, then the program *must* run suid to root!
 * Port 4201 is a port with historical significance, as the original
 * TinyMUD Classic ran on that port.  It was the office number of James
 * Aspnes, who wrote TinyMUD from which TinyMUCK eventually was derived.
 */
#define TINYPORT 4201           /* Port that players connect to */

/*
 * Some systems can hang for up to 30 seconds while trying to resolve
 * hostnames.  Define this to use a non-blocking second process to resolve
 * hostnames for you.  NOTE:  You need to compile the 'resolver' program
 * (make resolver) and put it in the directory that the netmuck program is
 * run from.
 */
#define SPAWN_HOST_RESOLVER


/************************************************************************
   Game Options

   These are the ones players will notice. 
 ************************************************************************/

/* Make the `examine' command display full names for types and flags */
#define VERBOSE_EXAMINE

/* limit on player name length */
#define PLAYER_NAME_LIMIT 16

/************************************************************************
   Various Messages 
 
   Printed from the server at times, esp. during login.
 ************************************************************************/

/*
 * Welcome message if you don't have a welcome.txt
 */
#define DEFAULT_WELCOME_MESSAGE "Welcome to TinyMUCK.\r\nTo connect to your existing character, enter \"connect name password\"\r\nTo create a new character, enter \"create name password\"\r\nIMPORTANT! Use the news command to get up-to-date news on program changes.\r\n\r\nYou can disconnect using the QUIT command, which must be capitalized as shown.\r\n\r\nAbusing or harrassing other players will result in expellation.\r\nUse the WHO command to find out who is currently active.\r\n"

/*
 * Error messeges spewed by the help system.
 */
#define NO_NEWS_MSG "That topic does not exist.  Type 'news topics' to list the news topics available."
#define NO_HELP_MSG "That topic does not exist.  Type 'help index' to list the help topics available."
#define NO_MAN_MSG "That topic does not exist.  Type 'man' to list the MUF topics available."
#define NO_INFO_MSG "That file does not exist.  Type 'info' to get a list of the info files available."

/************************************************************************
   File locations
 
   Where the system looks for its datafiles.
 ************************************************************************/
#define WELC_FILE "data/welcome.txt" /* For the opening screen      */
#define MOTD_FILE "data/motd.txt"    /* For the message of the day  */

#define HELP_FILE "data/help.txt"    /* For the 'help' command      */
#define HELP_DIR  "data/help"        /* For 'help' subtopic files   */
#define NEWS_FILE "data/news.txt"    /* For the 'news' command      */
#define NEWS_DIR  "data/news"        /* For 'news' subtopic files   */
#define MAN_FILE  "data/man.txt"     /* For the 'man' command       */
#define MAN_DIR   "data/man"         /* For 'man' subtopic files    */
#define MPI_FILE  "data/mpihelp.txt" /* For the 'mpi' command       */
#define MPI_DIR   "data/mpihelp"     /* For 'mpi' subtopic files    */
#define INFO_DIR  "data/info/"
#define EDITOR_HELP_FILE "data/edit-help.txt" /* editor help file   */

#define DELTAFILE_NAME "data/deltas-file"  /* The file for deltas */
#define PARMFILE_NAME "data/parmfile.cfg"  /* The file for config parms */
#define WORDLIST_FILE "data/wordlist.txt"  /* File for compression dict. */

#define LOG_GRIPE   "logs/gripes"       /* Gripes Log */
#define LOG_STATUS  "logs/status"       /* System errors and stats */
#define LOG_CONC    "logs/concentrator" /* Concentrator errors and stats */
#define LOG_MUF     "logs/muf-errors"   /* Muf compiler errors and warnings. */
#define COMMAND_LOG "logs/commands"     /* Player commands */
#define PROGRAM_LOG "logs/programs"     /* text of changed programs */

#define MACRO_FILE  "muf/macros"
#define PID_FILE    "netmuck.pid"       /* Write the server pid to ... */

#define RESOLVER_PID_FILE "hostfind.pid"   /* Write the resolver pid to ... */

#ifdef LOCKOUT
# define LOCKOUT_FILE "data/lockout.txt"
#endif /* LOCKOUT */

#ifdef DETACH
# define LOG_FILE "logs/netmuck"           /* Log stdout to ... */      
# define LOG_ERR_FILE "logs/netmuck.err"   /* Log stderr to ... */      
#endif /* DETACH */

/************************************************************************
  System Dependency Defines. 

  You probably will not have to monkey with this unless the muck fails
 to compile for some reason.
 ************************************************************************/

/* If you get problems compiling strftime.c, define this. */
#undef USE_STRFTIME

/* Use this only if your realloc does not allocate in powers of 2
 * (if your realloc is clever, this option will cause you to waste space).
 * SunOS requires DB_DOUBLING.  ULTRIX doesn't.  */
#define  DB_DOUBLING

/* Prevent Many Fine Cores. */
#undef NOCOREDUMP

/* if do_usage() in wiz.c gives you problems compiling, define this */
#undef NO_USAGE_COMMAND

/* if do_memory() in wiz.c gives you problems compiling, define this */
#define NO_MEMORY_COMMAND

/* This gives some debug malloc profiling, but also eats some overhead,
   so only define if your using it. */
#undef MALLOC_PROFILING

/************************************************************************/
/************************************************************************/
/*    FOR INTERNAL USE ONLY.  DON'T CHANGE ANYTHING PAST THIS POINT.    */
/************************************************************************/
/************************************************************************/

#ifdef SANITY
#undef MALLOC_PROFILING
#endif

/*
 * Very general defines 
 */
#define TRUE  1
#define FALSE 0

/*
 * Memory/malloc stuff.
 */
#undef LOG_PROPS
#undef LOG_DISKBASE
#undef USE_BALLOC
#undef USE_BALLOC_FIXED
#undef DEBUGDBDIRTY
#define FLUSHCHANGED /* outdated, needs to be removed from the source. */

/*
 * Include all the good standard headers here.
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

#include "autoconf.h"

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

/*
 * Which set of memory commands do we have here...
 */
#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
# include <string.h>
/* An ANSI string.h and pre-ANSI memory.h might conflict.  */
# if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#  include <memory.h>
# endif /* not STDC_HEADERS and HAVE_MEMORY_H */
/* Map BSD funcs to ANSI ones. */
# define index		strchr
# define rindex		strrchr
# define bcopy(s, d, n) memcpy ((d), (s), (n))
# define bcmp(s1, s2, n) memcmp ((s1), (s2), (n))
# define bzero(s, n) memset ((s), 0, (n))
#else /* not STDC_HEADERS and not HAVE_STRING_H */
# include <strings.h>
/* Map ANSI funcs to BSD ones. */
# define strchr		index
# define strrchr	rindex
# define memcpy(d, s, n) bcopy((s), (d), (n))
# define memcmp(s1, s2, n) bcmp((s1), (s2), (n))
/* no real way to map memset to bzero, unfortunatly. */
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef HAVE_RANDOM
# define SRANDOM(seed)	srandom((seed))
# define RANDOM()	random()
#else
# define SRANDOM(seed)	srand((seed))
# define RANDOM()	rand()
#endif

/*
 * Time stuff.
 */
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

/*
 * Include some of the useful local headers here.
 */
#ifdef MALLOC_PROFILING
#include "crt_malloc.h"
#endif

/******************************************************************/
/* System configuration stuff... Figure out who and what we are.  */
/******************************************************************/

/*
 * Try and figure out what we are.
 * 
 * Not realy used for anything any more, probably can be scrapped,
 * will see in a version or so.
 */
#if defined(linux) || defined(__linux__) || defined(LINUX)
# define SYS_TYPE "Linux"
# define LINUX
#endif

#ifdef sgi
# define SYS_TYPE "SGI"
#endif

#ifdef sun
# define SYS_TYPE "SUN"
# define SUN_OS
# define BSD43
#endif

#ifdef ultrix
# define SYS_TYPE "ULTRIX"
# define ULTRIX
#endif

#ifdef bds4_3
# ifndef SYS_TYPE
#  define SYS_TYPE "BSD 4.3"
# endif
# define BSD43
#endif

#ifdef bds4_2
# ifndef SYS_TYPE
#  define SYS_TYPE "BSD 4.2"
# endif
#endif

#if defined(SVR3) 
# ifndef SYS_TYPE
#  define SYS_TYPE "SVR3"
# endif
#endif

#if defined(SYSTYPE_SYSV) || defined(_SYSTYPE_SYSV)
# ifndef SYS_TYPE
#  define SYS_TYPE "SVSV"
# endif
#endif

#ifndef SYS_TYPE
# define SYS_TYPE "UNKNOWN"
#endif

/******************************************************************/
/* Final line of defense for self configuration, systems we know  */
/* need special treatment.                                        */ 
/******************************************************************/

