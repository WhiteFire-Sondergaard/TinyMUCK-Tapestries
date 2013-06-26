/*
 * Interface functions for the Muck to call to compile and run Lua
 * stuff.
 */

#ifndef _MLUA_H
#define _MLUA_H

#include "lua.hpp" /* Some stuff requires this */
#include "interpreter.h"
#include <tr1/memory>

/* TODO:
 *
 * interp structure _gc
 * execute properties
 * yield callback
 * returning data
 * print -> output string
 */

/*
 * Program Restrictions... this should be @tuned.
 */
#define	MLUA_INST_MAX		40000L;
#define	MLUA_INST_MAX_NOYIELD	1000L;
#define MLUA_INST_TRY_YIELD	200L;

/*
 * Lua "frame". The muck refers to it as this, really it's the environment
 * a running lua program. It contains information about how it was executed,
 * what it is doing, and keeps enough information to resume it if it is
 * yielded.
 */
/* Event that triggered this program */
#define MLUA_EVENT_CMD		1
#define MLUA_EVENT_LOCK		2
#define MLUA_EVENT_ARRIVE	3
#define MLUA_EVENT_DEPART	4
#define MLUA_EVENT_CONNECT	5
#define MLUA_EVENT_MPI		6
#define MLUA_EVENT_LUA		7
#define MLUA_EVENT_LOOK		8

/* Multitasking Mode ... */
#define MLUA_FOREGROUND		1
#define MLUA_BACKGROUND		2	/* Can not read */
#define MLUA_CALLED		3	/* Can not sleep. */

/* Reason for sleep ... */
#define MLUA_NOTSLEEPING	(-1)
#define MLUA_FIRSTRUN		0
#define	MLUA_SLEEP		1	/* Volentary sleep */
#define MLUA_YIELD		2	/* Volentary yield */
#define MLUA_FYIELD		3	/* Forced Yeild */
#define	MLUA_READ		4	/* Waiting for user input */

/*
 * Macros that could be functions. Just so simple it's esier this way.
 */
#define mlua_is_background(i)	((i)->mode == MLUA_BACKGROUND)
#define mlua_is_foreground(i)	((i)->mode == MLUA_FOREGROUND)
#define mlua_pid(i)		((i)->pid)

struct mlua_interp	/* Mlua interpreter environment, */
{
    lua_State *L; 	/* The LUA state. 
			   DO NOT use this within Lua C functions. */
    // lua_State *tL;	/* Thread state */

    int pid;		/* Muck timequeue PID */
    
    struct mlua_interp *caller; /* interp that called us */

    int event;    	/* The event type */
    int mode;		/* Muti-tasking Mode */
    int why_sleep;	/* Why is this program sleeping? */

    dbref player; 	/* player responsible for this event */
    dbref euid;		/* effective UID for perms */
    dbref program;	/* What program is this. */
    dbref trigger;	/* What object caused this event */

    long inst_max;	/* Max instructions, period. */
    long inst_try_yield; /* After this many, start trying to yield */
    long inst_max_noyield; /* After this, terminate program */

    long inst_count;	/* Total instruction count */
    long inst_since_yield; /* Instructions since sleep. */

    char *command;	/* malloced string with command */
    char *prop;		/* malloced string with prop run */

    std::tr1::weak_ptr<Interpreter> interpreter;
};

/* 
 */
struct mlua_interp *mlua_create_interp(
    dbref program,
    const char *property,
    dbref location,
    dbref player,
//    dbref output,
    dbref trigger,
    dbref euid,
    int mode,
    int event);

/* This function resumes a suspended Lua program by being passed it's
 * interp structure.
 */
std::tr1::shared_ptr<InterpreterReturnValue> mlua_resume(struct mlua_interp *interp, const char *resume_arg);

/* Compile a program and store the data into the in-memory DB.
 * Optionally notify player of errors or success.
 */
int mlua_compile(dbref player, dbref program, int silent);

/* Free compiled program's memory.
 */
void mlua_decompile(dbref program);

/* Library function definitions. Really not meant to be used directly by
 * the rest of the Muck.
 */
int mlua_open_mdb(lua_State *L);

/*
 * Clean up an unused interpreter
 */
void mlua_free_interp(struct mlua_interp *interp);

/* _MLUA_H */
#endif
