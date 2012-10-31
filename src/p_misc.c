/* Primitives package */

#include "copyright.h"
#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "db.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "strings.h"
#include "interp.h"

#include "timenode.hpp"
#include "interpeter.h"

extern int force_level;


static struct inst *oper1, *oper2, *oper3, *oper4;
//static struct inst temp1, temp2, temp3;
static int /* tmp, */ result;
static dbref ref;
static char buf[BUFFER_LEN];
struct tm *time_tm;

void 
prim_time(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(3);
    {
	time_t  lt;
	struct tm *tm;

	lt = time((long *) 0);
	tm = localtime(&lt);
	result = tm->tm_sec;
	PushInt(result);
	result = tm->tm_min;
	PushInt(result);
	result = tm->tm_hour;
	PushInt(result);
    }
}


void
prim_date(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(3);
    {
	time_t  lt;
	struct tm *tm;

	lt = time((long *) 0);
	tm = localtime(&lt);
	result = tm->tm_mday;
	PushInt(result);
	result = tm->tm_mon + 1;
	PushInt(result);
	result = tm->tm_year + 1900;
	PushInt(result);
    }
}

void
prim_gmtoffset(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);
    result = get_tz_offset();
    PushInt(result);
}

void 
prim_systime(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    result = time(NULL);
    CHECKOFLOW(1);
    PushInt(result);
}


void 
prim_timesplit(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();		/* integer: time */
    if (oper1->type != PROG_INTEGER)
	abort_interp("Invalid argument");
    time_tm = localtime((time_t *) (&(oper1->data.number)));
    CHECKOFLOW(8);
    CLEAR(oper1);
    result = time_tm->tm_sec;
    PushInt(result);
    result = time_tm->tm_min;
    PushInt(result);
    result = time_tm->tm_hour;
    PushInt(result);
    result = time_tm->tm_mday;
    PushInt(result);
    result = time_tm->tm_mon + 1;
    PushInt(result);
    result = time_tm->tm_year + 1900;
    PushInt(result);
    result = time_tm->tm_wday + 1;
    PushInt(result);
    result = time_tm->tm_yday + 1;
    PushInt(result);
}


void 
prim_timefmt(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper2 = POP();		/* integer: time */
    oper1 = POP();		/* string: format */
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument (1)");
    if (oper1->data.string == (struct shared_string *) NULL)
	abort_interp("Illegal NULL string (1)");
    if (oper2->type != PROG_INTEGER)
	abort_interp("Invalid argument (2)");
    time_tm = localtime((time_t *) (&(oper2->data.number)));
    if (!format_time(buf, BUFFER_LEN, oper1->data.string->data, time_tm))
	abort_interp("Operation would result in overflow.");
    CHECKOFLOW(1);
    CLEAR(oper1);
    CLEAR(oper2);
    PushString(buf);
}


void 
prim_queue(PRIM_PROTOTYPE)
{
    dbref temproom;

    /* int dbref string -- */
    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();
    if (mlev < 3)
	abort_interp("Requires Mucker level 3 or better.");
    if (oper3->type != PROG_INTEGER)
	abort_interp("Non-integer argument (1).");
    if (oper2->type != PROG_OBJECT)
	abort_interp("Argument must be a dbref (2)");
    if (!valid_object(oper2))
	abort_interp("Invalid dbref (2)");
    if (Typeof(oper2->data.objref) != TYPE_PROGRAM)
	abort_interp("Object must be a program. (2)");
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (3).");

    if ((oper4 = fr->variables + 1)->type != PROG_OBJECT)
	temproom = DBFETCH(player)->location;
    else
	temproom = oper4->data.objref;

    result = add_prog_delayq_event(oper3->data.number, player, temproom,
		    NOTHING, oper2->data.objref, DoNullInd(oper1->data.string),
		     "Queued Event.", 0);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushInt(result);
}


void 
prim_kill(PRIM_PROTOTYPE)
{
    /* i -- i */
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Non-integer argument (1).");
    if (oper1->data.number == fr->pid) {
	do_abort_silent();
    } else {
	if (mlev < 3) {
	    if (!control_process(ProgUID, oper1->data.number)) {
		abort_interp("Permission Denied.");
            }
        }
        result = dequeue_process(oper1->data.number);
    }
    CLEAR(oper1);
    PushInt(result);
}


void 
prim_force(PRIM_PROTOTYPE)
{
    struct inst *oper1, *oper2;

    /* d s -- */
    CHECKOP(2);
    oper1 = POP();		/* string to @force */
    oper2 = POP();		/* player dbref */
    if (mlev < 4)
	abort_interp("Wizbit only primitive.");
    if (fr->level > 8)
	abort_interp("Interp call loops not allowed.");
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2).");
    if (oper2->type != PROG_OBJECT)
	abort_interp("Non-object argument (1).");
    ref = oper2->data.objref;
    if (ref < 0 || ref >= db_top)
	abort_interp("Invalid object to force. (1)");
    if (Typeof(ref) != TYPE_PLAYER && Typeof(ref) != TYPE_THING)
	abort_interp("Object to force not a thing or player. (1)");
    if (!oper1->data.string)
	abort_interp("Null string argument (2).");
    if (index(oper1->data.string->data, '\r'))
	abort_interp("Carriage returns not allowed in command string. (2).");
#ifdef GOD_PRIV
    if (God(oper2->data.objref) && !God(OWNER(program)))
	abort_interp("Cannot force god (1).");
#endif
    force_level++;
    process_command(oper2->data.objref, oper1->data.string->data);
    force_level--;
    CLEAR(oper1);
    CLEAR(oper2);
}


void 
prim_timestamps(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_OBJECT)
	abort_interp("Non-object argument (1).");
    if (!valid_object(oper1))
	abort_interp("Invalid object.");
    CHECKREMOTE(oper1->data.objref);
    CHECKOFLOW(4);
    ref = oper1->data.objref;
    CLEAR(oper1);
    result = DBFETCH(ref)->ts.created;
    PushInt(result);
    result = DBFETCH(ref)->ts.modified;
    PushInt(result);
    result = DBFETCH(ref)->ts.lastused;
    PushInt(result);
    result = DBFETCH(ref)->ts.usecount;
    PushInt(result);
}

extern int top_pid;

void 
prim_fork(PRIM_PROTOTYPE)
{
    int     i, j;
    struct frame *tmpfr;
    std::tr1::shared_ptr<Interpeter> interp;

    CHECKOP(0);
    CHECKOFLOW(1);

    if (mlev < 3)
	abort_interp("Permission Denied.");

    fr->pc = pc;

    tmpfr = (struct frame *) calloc(1, sizeof(struct frame));

    tmpfr->system.top = fr->system.top;
    for (i = 0; i < fr->system.top; i++)
	tmpfr->system.st[i] = fr->system.st[i];

    tmpfr->argument.top = fr->argument.top;
    for (i = 0; i < fr->argument.top; i++)
	copyinst(&fr->argument.st[i], &tmpfr->argument.st[i]);

    tmpfr->caller.top = fr->caller.top;
    for (i = 0; i <= fr->caller.top; i++) {
	tmpfr->caller.st[i] = fr->caller.st[i];
	if (i > 0) DBFETCH(fr->caller.st[i])->sp.program.instances++;
    }

    for (i = 0; i < MAX_VAR; i++)
	copyinst(&fr->variables[i], &tmpfr->variables[i]);

    tmpfr->varset.top = fr->varset.top;
    for (i = fr->varset.top; i >= 0; i--) {
	tmpfr->varset.st[i] = (vars *) calloc(1, sizeof(vars));
	for (j = 0; j < MAX_VAR; j++)
	    copyinst(&((*fr->varset.st[i])[j]), &((*tmpfr->varset.st[i])[j]));
    }

    tmpfr->pc = pc;
    tmpfr->pc++;
    tmpfr->level = fr->level;
    tmpfr->already_created = fr->already_created;
    tmpfr->trig = fr->trig;

    tmpfr->brkpt.debugging = 0;
    tmpfr->brkpt.count = 0;
    tmpfr->brkpt.showstack = 0;
    tmpfr->brkpt.isread = 0;
    tmpfr->brkpt.bypass = 0;
    tmpfr->brkpt.lastcmd = NULL;

    tmpfr->pid = top_pid++;
    tmpfr->multitask = BACKGROUND;
    tmpfr->writeonly = 1;
    tmpfr->started = time(NULL);
    tmpfr->instcnt = 0;

    /* child process gets a 0 returned on the stack */
    result = 0;
    push(tmpfr->argument.st, &(tmpfr->argument.top),
	 PROG_INTEGER, MIPSCAST & result);

    interp = std::tr1::shared_ptr<Interpeter>(new MUFInterpeter(
        player, NOTHING, program,
        NOTHING, NULL, NULL, NULL /* should be something to indicate fork */,
        NULL, tmpfr));

    result = add_prog_delay_event(0, player, NOTHING, NOTHING, program,
				interp, "BACKGROUND");

    /* parent process gets the child's pid returned on the stack */
    if (!result)
	result = -1;
    PushInt(result);
}


void 
prim_pid(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);
    result = fr->pid;
    PushInt(result);
}


void 
prim_stats(PRIM_PROTOTYPE)
{
    /* A WhiteFire special. :) */
    CHECKOP(1);
    oper1 = POP();
    if (mlev < 3)
	abort_interp("Requires Mucker Level 3.");
    if (!valid_player(oper1) && (oper1->data.objref != NOTHING))
	abort_interp("non-player argument (1)");
    ref = oper1->data.objref;
    CLEAR(oper1);
    {
	dbref   i;
	int     rooms, exits, things, players, programs, garbage;

	/* tmp, ref */
	rooms = exits = things = players = programs = garbage = 0;
	for (i = 0; i < db_top; i++) {
	    if (ref == NOTHING || OWNER(i) == ref) {
		switch (Typeof(i)) {
		    case TYPE_ROOM:
			rooms++;
			break;
		    case TYPE_EXIT:
			exits++;
			break;
		    case TYPE_THING:
			things++;
			break;
		    case TYPE_PLAYER:
			players++;
			break;
		    case TYPE_PROGRAM:
			programs++;
			break;
		    case TYPE_GARBAGE:
			garbage++;
			break;
		}
	    }
	}
	ref = rooms + exits + things + players + programs + garbage;
	PushInt(ref);
	PushInt(rooms);
	PushInt(exits);
	PushInt(things);
	PushInt(programs);
	PushInt(players);
	PushInt(garbage);
	/* push results */
    }
}

void 
prim_abort(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument");
    strcpy(buf, DoNullInd(oper1->data.string));
    abort_interp(buf);
}


void 
prim_ispidp(PRIM_PROTOTYPE)
{
    /* i -- i */
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Non-integer argument (1).");
    if (oper1->data.number == fr->pid) {
	result = 1;
    } else {
        result = in_timequeue(oper1->data.number);
    }
    CLEAR(oper1);
    PushInt(result);
}


void 
prim_parselock(PRIM_PROTOTYPE)
{
    struct boolexp *lok;

    CHECKOP(1);
    oper1 = POP();		/* string: lock string */
    CHECKOFLOW(1);
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument.");
    if (oper1->data.string != (struct shared_string *) NULL) {
	lok = parse_boolexp(ProgUID, oper1->data.string->data, 0);
    } else {
	lok = TRUE_BOOLEXP;
    }
    CLEAR(oper1);
    PushLock(lok);
}


void 
prim_unparselock(PRIM_PROTOTYPE)
{
    const char *ptr;

    CHECKOP(1);
    oper1 = POP();		/* lock: lock */
    if (oper1->type != PROG_LOCK)
	abort_interp("Invalid argument.");
    if (oper1->data.lock != (struct boolexp *) TRUE_BOOLEXP) {
	ptr = unparse_boolexp(ProgUID, oper1->data.lock, 0);
    } else {
	ptr = NULL;
    }
    CHECKOFLOW(1);
    CLEAR(oper1);
    if (ptr) {
	PushString(ptr);
    } else {
	PushNullStr;
    }
}


void 
prim_prettylock(PRIM_PROTOTYPE)
{
    const char *ptr;

    CHECKOP(1);
    oper1 = POP();		/* lock: lock */
    if (oper1->type != PROG_LOCK)
	abort_interp("Invalid argument.");
    ptr = unparse_boolexp(ProgUID, oper1->data.lock, 1);
    CHECKOFLOW(1);
    CLEAR(oper1);
    PushString(ptr);
}


void 
prim_testlock(PRIM_PROTOTYPE)
{
    struct inst *oper1, *oper2;

    /* d d - i */
    CHECKOP(2);
    oper1 = POP();		/* boolexp lock */
    oper2 = POP();		/* player dbref */
    if (fr->level > 8)
	abort_interp("Interp call loops not allowed.");
    if (!valid_object(oper2))
	abort_interp("Invalid argument (1).");
    if (Typeof(oper2->data.objref) != TYPE_PLAYER &&
        Typeof(oper2->data.objref) != TYPE_THING )
    {
	abort_interp("Invalid object type (1).");
    }
    CHECKREMOTE(oper2->data.objref);
    if (oper1->type != PROG_LOCK)
	abort_interp("Invalid argument (2).");
    result = eval_boolexp(oper2->data.objref, oper1->data.lock, player);
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}


void 
prim_sysparm(PRIM_PROTOTYPE)
{
    const char *ptr;
    const char *tune_get_parmstring(const char *name, int mlev);

    CHECKOP(1);
    oper1 = POP();		/* string: system parm name */
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument.");
    if (oper1->data.string) {
	ptr = tune_get_parmstring(oper1->data.string->data, mlev);
    } else {
	ptr = "";
    }
    CHECKOFLOW(1);
    CLEAR(oper1);
    PushString(ptr);
}


