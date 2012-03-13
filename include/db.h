/* $Header: /home/foxen/CR/FB/include/RCS/db.h,v 1.1 1996/06/17 17:29:45 foxen Exp foxen $
 *
 * $Log: db.h,v $
 * Revision 1.1  1996/06/17 17:29:45  foxen
 * Initial revision
 *
 * Revision 5.16  1994/02/27  21:24:26  foxen
 * Removed idiotic redefinition of malloc()
 *
 * Revision 5.15  1994/02/11  05:55:59  foxen
 * memory monitoring code and memory cleanup mods.
 *
 * Revision 5.14  1994/01/15  00:29:46  foxen
 * Added SETDOING() macro.
 *
 * Revision 5.13  1994/01/15  00:08:39  foxen
 * @doing mods.
 *
 * Revision 5.12  1994/01/06  03:16:30  foxen
 * Version 5.12
 *
 * Revision 5.1  1993/12/17  00:35:54  foxen
 * initial revision.
 *
 * Revision 1.1  91/01/24  00:43:41  cks
 * changes for QUELL.
 *
 * Revision 1.0  91/01/22  20:32:17  cks
 * Initial revision
 *
 * Revision 1.10  90/09/28  12:15:15  rearl
 * Added shared string structures to interp.c.
 *
 * Revision 1.9  90/09/18  08:05:45  rearl
 * Took out FILTER, added INTERNAL.  Moved stuff to params.h
 *
 * Revision 1.8  90/09/16  13:59:03  rearl
 * Disk based preparation code added.
 *
 * Revision 1.7  90/09/13  06:33:20  rearl
 * MAX_VAR increased to 53 == 50 user variables.
 *
 * Revision 1.6  90/09/01  06:03:46  rearl
 * Took out TEST_MALLOC references.
 *
 * Revision 1.5  90/08/27  14:06:31  rearl
 * Disk-based MUF source code and added necessary locks.
 *
 * Revision 1.4  90/08/15  03:50:57  rearl
 * Added some new macros and made others more useful...
 *
 * Revision 1.3  90/07/29  17:16:43  rearl
 * Moved some things to config.h, minor change in programs to allow
 * locks to rooms, @# in rooms, etc.
 *
 * Revision 1.2  90/07/19  23:06:07  casie
 * Removed comment log at top.
 *
 *
 */
#include "copyright.h"

#ifndef __DB_H
#define __DB_H

#include <stdio.h>
#include <time.h>
#include <sys/time.h>

/* max length of command argument to process_command */
#define MAX_COMMAND_LEN 1024
#define BUFFER_LEN ((MAX_COMMAND_LEN)*4)
#define FILE_BUFSIZ ((BUFSIZ)*8)

extern time_t current_systime;
extern char match_args[BUFFER_LEN];
extern char match_cmdname[BUFFER_LEN];

typedef int dbref;		/* offset into db */

#ifdef GDBM_DATABASE
#  define DBFETCH(x)  dbfetch(x)
#  define DBDIRTY(x)  dbdirty(x)
#else				/* !GDBM_DATABASE */
#  define DBFETCH(x)  (db + (x))
#  ifdef DEBUGDBDIRTY
#    define DBDIRTY(x)  {if (!(db[x].flags & OBJECT_CHANGED))  \
			   log2file("dirty.out", "#%d: %s %d\n", (int)x, \
			   __FILE__, __LINE__); \
		       db[x].flags |= OBJECT_CHANGED;}
#  else
#    define DBDIRTY(x)  {db[x].flags |= OBJECT_CHANGED;}
#  endif
#endif

#define DBSTORE(x, y, z)    {DBFETCH(x)->y = z; DBDIRTY(x);}
#define NAME(x)     (db[x].name)
#if defined(ANONYMITY)
#  define PNAME(x)  (name_mangle(x))
#  define RNAME(x)  (unmangle(player, name_mangle(x)))
#else
#  define PNAME(x)  (db[x].name)
#  define RNAME(x)  (db[x].name)
#endif
#define FLAGS(x)    (db[x].flags)
#define OWNER(x)    (db[x].owner)

/* defines for possible data access mods. */
#define GETMESG(x,y)   (get_property_class(x, y))
#define GETDESC(x)  GETMESG(x, "_/de")
#define GETIDESC(x) GETMESG(x, "_/ide")
#define GETSUCC(x)  GETMESG(x, "_/sc")
#define GETOSUCC(x) GETMESG(x, "_/osc")
#define GETFAIL(x)  GETMESG(x, "_/fl")
#define GETOFAIL(x) GETMESG(x, "_/ofl")
#define GETDROP(x)  GETMESG(x, "_/dr")
#define GETODROP(x) GETMESG(x, "_/odr")
#define GETDOING(x) GETMESG(x, "_/do")
#define GETOECHO(x) GETMESG(x, "_/oecho")
#define GETPECHO(x) GETMESG(x, "_/pecho")

#define SETMESG(x,y,z)    {add_property(x, y, z, 0);}
#define SETDESC(x,y)   SETMESG(x,"_/de",y)
#define SETIDESC(x,y)  SETMESG(x,"_/ide",y)
#define SETSUCC(x,y)   SETMESG(x,"_/sc",y)
#define SETFAIL(x,y)   SETMESG(x,"_/fl",y)
#define SETDROP(x,y)   SETMESG(x,"_/dr",y)
#define SETOSUCC(x,y)  SETMESG(x,"_/osc",y)
#define SETOFAIL(x,y)  SETMESG(x,"_/ofl",y)
#define SETODROP(x,y)  SETMESG(x,"_/odr",y)
#define SETDOING(x,y)  SETMESG(x,"_/do",y)
#define SETOECHO(x,y)  SETMESG(x,"_/oecho",y)
#define SETPECHO(x,y)  SETMESG(x,"_/pecho",y)

#define LOADMESG(x,y,z)    {add_prop_nofetch(x,y,z,0); DBDIRTY(x);}
#define LOADDESC(x,y)   LOADMESG(x,"_/de",y)
#define LOADIDESC(x,y)  LOADMESG(x,"_/ide",y)
#define LOADSUCC(x,y)   LOADMESG(x,"_/sc",y)
#define LOADFAIL(x,y)   LOADMESG(x,"_/fl",y)
#define LOADDROP(x,y)   LOADMESG(x,"_/dr",y)
#define LOADOSUCC(x,y)  LOADMESG(x,"_/osc",y)
#define LOADOFAIL(x,y)  LOADMESG(x,"_/ofl",y)
#define LOADODROP(x,y)  LOADMESG(x,"_/odr",y)

#define SETLOCK(x,y) {set_property(x, "_/lok", PROP_LOKTYP, (int)y);}
#define LOADLOCK(x,y) {set_property_nofetch(x, "_/lok", PROP_LOKTYP, (int)y); DBDIRTY(x);}
#define CLEARLOCK(x) {set_property(x, "_/lok", PROP_LOKTYP, (int)TRUE_BOOLEXP);}
#define GETLOCK(x) (get_property_lock(x, "_/lok"))

#define DB_PARMSINFO    0x0001
#define DB_COMPRESSED   0x0002

#define TYPE_ROOM   0x0
#define TYPE_THING  0x1
#define TYPE_EXIT   0x2
#define TYPE_PLAYER     0x3
#define TYPE_PROGRAM    0x4
#define TYPE_GARBAGE    0x6
#define NOTYPE      0x7		/* no particular type */
#define TYPE_MASK   0x7		/* room for expansion */
#define ANTILOCK    0x8		/* negates key (*OBSOLETE*) */
#define WIZARD      0x10	/* gets automatic control */
#define LINK_OK     0x20	/* anybody can link to this room */
#define DARK        0x40	/* contents of room are not printed */
#define INTERNAL        0x80	/* internal-use-only flag */
#define STICKY      0x100	/* this object goes home when dropped */
#define BUILDER     0x200	/* this player can use construction commands */
#define CHOWN_OK    0x400	/* this player can be @chowned to */
#define JUMP_OK     0x800	/* A room which can be jumped from, or */
                                /* a player who can be jumped to */
#define GENDER_MASK 0x3000	/* 2 bits of gender */
#define GENDER_SHIFT    12	/* 0x1000 is 12 bits over (for shifting) */
#define GENDER_UNASSIGNED   0x0	/* unassigned - the default */
#define GENDER_NEUTER   0x1	/* neuter */
#define GENDER_FEMALE   0x2	/* for women */
#define GENDER_MALE 0x3		/* for men */
#define GENDER_HERM 0x4		/* for herms, never used as a flag */

#define KILL_OK		0x4000	/* Kill_OK bit.  Means you can be killed. */

#define HAVEN           0x10000	/* can't kill here */
#define ABODE           0x20000	/* can set home here */

#define MUCKER          0x40000	/* programmer */

#define QUELL           0x80000	/* When set, a wizard is considered to not be
				 * a wizard. */
#define SMUCKER        0x100000	/* second programmer bit.  For levels */

#define INTERACTIVE    0x200000 /* when this is set, player is either editing
				 * a program or in a READ. */
#define READMODE     0x10000000 /* when set, player is in a READ */

#define OBJECT_CHANGED 0x400000 /* when an object is dbdirty()ed, set this */
#define SAVED_DELTA    0x800000 /* object last saved to delta file */

#define VEHICLE       0x1000000 /* Vehicle flag */
#define ZOMBIE        0x2000000 /* Zombie flag */
#define ZONE          0x2000000 /* Zone lag */

#define LISTENER      0x4000000 /* listener flag */
#define XFORCIBLE     0x8000000 /* externally forcible flag */
#define SANEBIT      0x10000000 /* used to check db sanity */


/* what flags to NOT dump to disk. */
#define DUMP_MASK    (INTERACTIVE | SAVED_DELTA | OBJECT_CHANGED | LISTENER | READMODE | SANEBIT)


typedef long object_flag_type;

#define GOD ((dbref) 1)

#ifdef GOD_PRIV
#define God(x) ((x) == (GOD))
#endif				/* GOD_PRIV */

#define DoNull(s) ((s) ? (s) : "")
#define Typeof(x) (x == HOME ? TYPE_ROOM : (FLAGS(x) & TYPE_MASK))
#define Wizard(x) ((FLAGS(x) & WIZARD) != 0 && (FLAGS(x) & QUELL) == 0)

/* TrueWizard is only appropriate when you care about whether the person
   or thing is, well, truely a wizard. Ie it ignores QUELL. */
#define TrueWizard(x) ((FLAGS(x) & WIZARD) != 0)
#define Dark(x) ((FLAGS(x) & DARK) != 0)

#define MLevRaw(x) (((FLAGS(x) & MUCKER)? 2:0) + ((FLAGS(x) & SMUCKER)? 1:0))

#define MLevel(x) ((FLAGS(x) & WIZARD)? 4 : \
		   (((FLAGS(x) & MUCKER)? 2 : 0) + \
		    ((FLAGS(x) & SMUCKER)? 1 : 0)))

#define SetMLevel(x,y) { FLAGS(x) &= ~(MUCKER | SMUCKER); \
			 if (y>=2) FLAGS(x) |= MUCKER; \
                         if (y%2) FLAGS(x) |= SMUCKER; }

#define PLevel(x) ((FLAGS(x) & (MUCKER | SMUCKER))? \
                   (((FLAGS(x) & MUCKER)? 2:0) + ((FLAGS(x) & SMUCKER)? 1:0) + 1) : \
                    ((FLAGS(x) & ABODE)? 0 : 1))

#define PREEMPT 0
#define FOREGROUND 1
#define BACKGROUND 2

#define Mucker(x) (MLevel(x) != 0)

#define Builder(x) ((FLAGS(x) & (WIZARD|BUILDER)) != 0)

#define Linkable(x) ((x) == HOME || \
                     (((Typeof(x) == TYPE_ROOM || Typeof(x) == TYPE_THING) ? \
                      (FLAGS(x) & ABODE) : (FLAGS(x) & LINK_OK)) != 0))


/* Boolean expressions, for locks */
typedef char boolexp_type;

#define BOOLEXP_AND 0
#define BOOLEXP_OR 1
#define BOOLEXP_NOT 2
#define BOOLEXP_CONST 3
#define BOOLEXP_PROP 4

struct boolexp {
    boolexp_type type;
    struct boolexp *sub1;
    struct boolexp *sub2;
    dbref   thing;
    struct plist *prop_check;
};

#define TRUE_BOOLEXP ((struct boolexp *) 0)

/* special dbref's */
#define NOTHING ((dbref) -1)	/* null dbref */
#define AMBIGUOUS ((dbref) -2)	/* multiple possibilities, for matchers */
#define HOME ((dbref) -3)	/* virtual room, represents mover's home */

/* editor data structures */

/* Line data structure */
struct line {
    const char *this_line;	/* the line itself */
    struct line *next, *prev;	/* the next line and the previous line */
};

/* stack and object declarations */
/* Integer types go here */
#define PROG_CLEARED     0
#define PROG_PRIMITIVE   1	/* forth prims and hard-coded C routines */
#define PROG_INTEGER     2	/* integer types */
#define PROG_OBJECT      3	/* database objects */
#define PROG_VAR         4	/* variables */
#define PROG_LVAR        5	/* variables */
/* Pointer types go here, numbered *AFTER* PROG_STRING */
#define PROG_STRING      6	/* string types */
#define PROG_FUNCTION    7	/* function names for debugging. */
#define PROG_LOCK        8	/* boolean expression */
#define PROG_ADD         9	/* program address - used in calls&jmps */
#define PROG_IF          10	/* A low level IF statement */
#define PROG_EXEC        11	/* EXECUTE shortcut */
#define PROG_JMP         12	/* JMP shortcut */

#define MAX_VAR         54	/* maximum number of variables including the
				 * basic ME and LOC                */
#define RES_VAR          4	/* no of reserved variables */

#define STACK_SIZE       1024	/* maximum size of stack */

struct shared_string {		/* for sharing strings in programs */
    int     links;		/* number of pointers to this struct */
    int     length;		/* length of string data */
    char    data[1];		/* shared string data */
};

struct prog_addr {              /* for 'addres references */
    int     links;              /* number of pointers */
    dbref   progref;            /* program dbref */
    struct inst *data;          /* pointer to the code */
};

struct stack_addr {             /* for the system calstack */
    dbref   progref;            /* program call was made from */
    struct inst *offset;        /* the address of the call */
};

struct inst {			/* instruction */
    short   type;
    short   line;
    union {
	struct shared_string *string;  /* strings */
	struct boolexp *lock;   /* booleam lock expression */
	int     number;		/* used for both primitives and integers */
	dbref   objref;		/* object reference */
	struct inst *call;	/* use in IF and JMPs */
	struct prog_addr *addr; /* the result of 'funcname */
    }       data;
};

typedef struct inst vars[MAX_VAR];

struct stack {
    int     top;
    struct inst st[STACK_SIZE];
};

struct sysstack {
    int     top;
    struct stack_addr st[STACK_SIZE];
};

struct callstack {
    int     top;
    dbref   st[STACK_SIZE];
};

struct varstack {
    int     top;
    vars   *st[STACK_SIZE];
};


#define MAX_BREAKS 16
struct debuggerdata {
    unsigned debugging:1;   /* if set, this frame is being debugged */
    unsigned bypass:1;      /* if set, bypass breakpoint on starting instr */
    unsigned isread:1;      /* if set, the prog is trying to do a read */
    unsigned showstack:1;   /* if set, show stack debug line, each inst. */
    int lastlisted;         /* last listed line */
    char *lastcmd;          /* last executed debugger command */
    short breaknum;         /* the breakpoint that was just caught on */

    dbref lastproglisted;   /* What program's text was last loaded to list? */
    struct line *proglines; /* The actual program text last loaded to list. */

    short count;            /* how many breakpoints are currently set */
    short temp[MAX_BREAKS];         /* is this a temp breakpoint? */
    short level[MAX_BREAKS];        /* level breakpnts.  If -1, no check. */
    struct inst *lastpc;            /* Last inst interped.  For inst changes. */
    struct inst *pc[MAX_BREAKS];    /* pc breakpoint.  If null, no check. */
    int pccount[MAX_BREAKS];        /* how many insts to interp.  -2 for inf. */
    int lastline;                   /* Last line interped.  For line changes. */
    int line[MAX_BREAKS];           /* line breakpts.  -1 no check. */
    int linecount[MAX_BREAKS];      /* how many lines to interp.  -2 for inf. */
    dbref prog[MAX_BREAKS];         /* program that breakpoint is in. */
};

#define STD_REGUID 0
#define STD_SETUID 1
#define STD_HARDUID 2

/* frame data structure necessary for executing programs */
struct frame {
    struct frame *next;
    struct sysstack system;	/* system stack */
    struct stack argument;	/* argument stack */
    struct callstack caller;	/* caller prog stack */
    struct varstack varset;	/* local variables */
    vars    variables;		/* global variables */
    struct inst *pc;		/* next executing instruction */
    int     writeonly;		/* This program should not do reads */
    int     multitask;		/* This program's multitasking mode */
    int     perms;              /* permissions restrictions on program */
    int     level;		/* prevent interp call loops */
    short   already_created;	/* this prog already created an object */
    short   been_background;	/* this prog has run in the background */
    dbref   trig;		/* triggering object */
    long    started;		/* When this program started. */
    int     instcnt;		/* How many instructions have run. */
    int     pid;		/* what is the process id? */
    struct debuggerdata brkpt;  /* info the debugger needs */
    struct timeval proftime;    /* profiling timing code */
    struct timeval totaltime;   /* profiling timing code */
};


struct publics {
    char   *subname;
    union {
	struct inst *ptr;
	int     no;
    }       addr;
    struct publics *next;
};

/* union of type-specific fields */

union specific {      /* I've been railroaded! */
    struct {			/* ROOM-specific fields */
	dbref   dropto;
    }       room;
    struct {			/* THING-specific fields */
	dbref   home;
	int     value;
    }       thing;
    struct {			/* EXIT-specific fields */
	int     ndest;
	dbref  *dest;
    }       exit;
    struct {			/* PLAYER-specific fields */
	dbref   home;
	int     pennies;
	dbref   curr_prog;	/* program I'm currently editing */
	short   insert_mode;	/* in insert mode? */
	short   block;
	const char *password;
        int*    descrs;
        short   descr_count;
    }       player;
    struct {			/* PROGRAM-specific fields */
	short   curr_line;	/* current-line */
	unsigned short instances;  /* #instances of this prog running */
	int     siz;		/* size of code */
	struct inst *code;	/* byte-compiled code */
	struct inst *start;	/* place to start executing */
	struct line *first;	/* first line */
	struct publics *pubs;	/* public subroutine addresses */
    }       program;
};


/* timestamps record */

struct timestamps {
    time_t created;
    time_t modified;
    time_t lastused;
    int    usecount;
};


struct object {

    const char *name;
    dbref   location;		/* pointer to container */
    dbref   owner;
    dbref   contents;
    dbref   exits;
    dbref   next;		/* pointer to next in contents/exits chain */
    struct plist *properties;

#ifdef DISKBASE
    long    propsfpos;
    time_t  propstime;
    dbref   nextold;
    dbref   prevold;
    short   propsmode;
    short   spacer;
#endif

    object_flag_type flags;
    struct timestamps ts;
    union specific sp;
    unsigned int     mpi_prof_sec;
    unsigned int     mpi_prof_usec;
    unsigned int     mpi_prof_use;
};

struct macrotable {
    char   *name;
    char   *definition;
    dbref   implementor;
    struct macrotable *left;
    struct macrotable *right;
};

/* Possible data types that may be stored in a hash table */
union u_hash_data {
    int     ival;		/* Store compiler tokens here */
    dbref   dbval;		/* Player hashing will want this */
    void   *pval;		/* compiler $define strings use this */
};

/* The actual hash entry for each item */
struct t_hash_entry {
    struct t_hash_entry *next;	/* Pointer for conflict resolution */
    const char *name;		/* The name of the item */
    union u_hash_data dat;	/* Data value for item */
};

typedef union u_hash_data hash_data;
typedef struct t_hash_entry hash_entry;
typedef hash_entry *hash_tab;

#define PLAYER_HASH_SIZE   (1024)	/* Table for player lookups */
#define COMP_HASH_SIZE     (256)/* Table for compiler keywords */
#define DEFHASHSIZE        (256)/* Table for compiler $defines */

extern struct object *db;
extern struct macrotable *macrotop;
extern dbref db_top;

#ifndef MALLOC_PROFILING
extern char *alloc_string(const char *);
#endif

extern struct shared_string *alloc_prog_string(const char *);

extern dbref new_object();	/* return a new object */

extern dbref getref(FILE *);	/* Read a database reference from a file. */

extern void putref(FILE *, dbref);	/* Write one ref to the file */

extern struct boolexp *getboolexp(FILE *);	/* get a boolexp */
extern void putboolexp(FILE *, struct boolexp *);	/* put a boolexp */

extern int db_write_object(FILE *, dbref);	/* write one object to file */

extern dbref db_write(FILE * f);/* write db to file, return # of objects */

extern dbref db_read(FILE * f);	/* read db from file, return # of objects */

 /* Warning: destroys existing db contents! */

extern void db_free(void);

extern dbref parse_dbref(const char *);	/* parse a dbref */

#define DOLIST(var, first) \
  for((var) = (first); (var) != NOTHING; (var) = DBFETCH(var)->next)
#define PUSH(thing, locative) \
    {DBSTORE((thing), next, (locative)); (locative) = (thing);}
#define getloc(thing) (DBFETCH(thing)->location)

/*
  Usage guidelines:

  To obtain an object pointer use DBFETCH(i).  Pointers returned by DBFETCH
  may become invalid after a call to new_object().

  To update an object, use DBSTORE(i, f, v), where i is the object number,
  f is the field (after ->), and v is the new value.

  If you have updated an object without using DBSTORE, use DBDIRTY(i) before
  leaving the routine that did the update.

  When using PUSH, be sure to call DBDIRTY on the object which contains
  the locative (see PUSH definition above).

  Some fields are now handled in a unique way, since they are always memory
  resident, even in the GDBM_DATABASE disk-based muck.  These are: name,
  flags and owner.  Refer to these by NAME(i), FLAGS(i) and OWNER(i).
  Always call DBDIRTY(i) after updating one of these fields.

  The programmer is responsible for managing storage for string
  components of entries; db_read will produce malloc'd strings.  The
  alloc_string routine is provided for generating malloc'd strings
  duplicates of other strings.  Note that db_free and db_read will
  attempt to free any non-NULL string that exists in db when they are
  invoked.
*/
#endif				/* __DB_H */
