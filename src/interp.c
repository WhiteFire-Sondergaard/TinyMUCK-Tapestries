/* Muf Interpreter and dispatcher. */

#include "copyright.h"
#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "db.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "strings.h"
#include "interp.h"

/* This package performs the interpretation of mud forth programs.
   It is a basically push pop kinda thing, but I'm making some stuff
   inline for maximum efficiency.

   Oh yeah, because it is an interpreted language, please do type
   checking during this time.  While you're at it, any objects you
   are referencing should be checked against db_top.
   */

/* in cases of boolean expressions, we do return a value, the stuff
   that's on top of a stack when a mud-forth program finishes executing.
   In cases where they don't, leave a value, we just return a 0.  Note:
   this stuff does not return string or whatnot.  It at most can be
   relied on to return a boolean value.

   interp sets up a player's frames and so on and prepares it for
   execution.
   */


void 
p_null(PRIM_PROTOTYPE)
{
    return;
}

/* void    (*prim_func[]) (PRIM_PROTOTYPE) = */
void    (*prim_func[])() =
{
    p_null, p_null, p_null, p_null, p_null, p_null,
    /* JMP, READ,   SLEEP,  CALL,   EXECUTE,RETURN */
    PRIMS_CONNECTS_FUNCS,
    PRIMS_DB_FUNCS,
    PRIMS_MATH_FUNCS,
    PRIMS_MISC_FUNCS,
    PRIMS_PROPS_FUNCS,
    PRIMS_STACK_FUNCS,
    PRIMS_STRINGS_FUNCS
};

void 
RCLEAR(struct inst * oper, char *file, int line)
{
    if (oper->type == PROG_CLEARED) {
	fprintf(stderr, "Attempt to re-CLEAR() instruction from %s:%hd "
"previously CLEAR()ed in %s:%d\n", file, line, 
		(char *)oper->data.addr, // This is probably WRONG
	       	oper->line);
	return;
    }
    if (oper->type == PROG_ADD) {
	DBFETCH(oper->data.addr->progref)->sp.program.instances--;
	oper->data.addr->links--;
    }
    if ((oper->type == PROG_STRING || oper->type == PROG_FUNCTION)
	    && oper->data.string &&
	    --oper->data.string->links == 0)
	free((void *) oper->data.string);
    if (oper->type == PROG_LOCK && oper->data.lock != TRUE_BOOLEXP)
	free_boolexp(oper->data.lock);
    oper->line = line;
    oper->data.addr = (void *)file;
    oper->type = PROG_CLEARED;
}

int     valid_object(struct inst * oper);

int     top_pid = 1;
int     nargs = 0;

static struct frame *free_frames_list = NULL;

void
purge_free_frames(void)
{
    struct frame *ptr, *ptr2;
    int count = tp_free_frames_pool;
    for(ptr = free_frames_list; ptr && --count>0; ptr = ptr->next);
    while (ptr && ptr->next) {
	ptr2 = ptr->next;
	ptr->next = ptr->next->next;
	free(ptr2);
    }
}

void
purge_all_free_frames(void)
{
    struct frame *ptr;
    while (free_frames_list) {
	ptr = free_frames_list;
	free_frames_list = ptr->next;
	free(ptr);
    }
}



struct inst *
interp(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int rettyp)
{
    struct frame *fr;
    struct inst *mv;
    int     i;

    if (!MLevel(program) || !MLevel(OWNER(program)) ||
	    ((source != NOTHING) && !TrueWizard(OWNER(source)) &&
	     !can_link_to(OWNER(source), TYPE_EXIT, program))) {
	notify_nolisten(player, "Program call: Permission denied.", 1);
	return 0;
    }
    if (free_frames_list) {
	fr = free_frames_list;
	free_frames_list = fr->next;
    } else {
	fr = (struct frame *) malloc(sizeof(struct frame));
    }
    fr->next = NULL;
    fr->pid = top_pid++;
    fr->multitask = nosleeps;
    fr->perms=whichperms;
    fr->already_created = 0;
    fr->been_background = (nosleeps == 2);
    fr->trig = source;
    fr->started = current_systime;
    fr->instcnt = 0;
    fr->caller.top = 1;
    fr->caller.st[0] = source;
    fr->caller.st[1] = program;

    fr->system.top = 1;
    fr->system.st[0].progref = 0;
    fr->system.st[0].offset = 0;

    fr->argument.top = 0;
    fr->pc = DBFETCH(program)->sp.program.start;
    fr->writeonly = ((source == -1) || (Typeof(source) == TYPE_ROOM) ||
		     ((Typeof(source) == TYPE_PLAYER) && (!online(source))) ||
		     (FLAGS(player) & READMODE));
    fr->level = 0;

    /* set basic local variables */

    fr->varset.top = 0;
    fr->varset.st[0] = (vars *) malloc(sizeof(vars));
    for (i = 0; i < MAX_VAR; i++) {
	(*fr->varset.st[0])[i].type = PROG_INTEGER;
	(*fr->varset.st[0])[i].data.number = 0;
	fr->variables[i].type = PROG_INTEGER;
	fr->variables[i].data.number = 0;
    }

    fr->brkpt.debugging  = 0;
    fr->brkpt.bypass     = 0;
    fr->brkpt.isread     = 0;
    fr->brkpt.showstack  = 0;
    fr->brkpt.lastline   = 0;
    fr->brkpt.lastpc     = 0;
    fr->brkpt.lastlisted = 0;
    fr->brkpt.lastcmd    = NULL;
    fr->brkpt.breaknum   = -1;

    fr->brkpt.lastproglisted = NOTHING;
    fr->brkpt.proglines = NULL;

    fr->brkpt.count        = 1;
    fr->brkpt.temp[0]      = 1;
    fr->brkpt.level[0]     = -1;
    fr->brkpt.line[0]      = -1;
    fr->brkpt.linecount[0] = -2;
    fr->brkpt.pc[0]        = NULL;
    fr->brkpt.pccount[0]   = -2;
    fr->brkpt.prog[0]      = program;

    fr->proftime.tv_sec = 0;
    fr->proftime.tv_usec = 0;
    fr->totaltime.tv_sec = 0;
    fr->totaltime.tv_usec = 0;

    fr->variables[0].type = PROG_OBJECT;
    fr->variables[0].data.objref = player;
    fr->variables[1].type = PROG_OBJECT;
    fr->variables[1].data.objref = location;
    fr->variables[2].type = PROG_OBJECT;
    fr->variables[2].data.objref = source;
    fr->variables[3].type = PROG_STRING;
    fr->variables[3].data.string =
	(!*match_cmdname) ? 0 : alloc_prog_string(match_cmdname);

    ts_useobject(program);
    if (DBFETCH(program)->sp.program.code) {
	DBFETCH(program)->sp.program.code[3].data.number++;
    }
    DBFETCH(program)->sp.program.instances++;
    push(fr->argument.st, &(fr->argument.top), PROG_STRING, match_args ?
	 MIPSCAST alloc_prog_string(match_args) : 0);
    mv = interp_loop(player, program, fr, rettyp);
    return (mv);
}

static int err;
int     already_created;

/* clean up the stack. */
void 
prog_clean(struct frame * fr)
{
    int     i, j;
    struct frame *ptr;

    for(ptr = free_frames_list; ptr; ptr = ptr->next) {
	if (ptr == fr) {
	    fprintf(stderr, "prog_clean(): Tried to free an already freed program frame!\n");
	    abort();
	}
    }

    fr->system.top = 0;
    for (i = 0; i < fr->argument.top; i++)
	CLEAR(&fr->argument.st[i]);

    for (i = 1; i <= fr->caller.top; i++)
	DBFETCH(fr->caller.st[i])->sp.program.instances--;

    for (i = fr->varset.top; i > -1; i--) {
	for (j = 0; j < MAX_VAR; j++)
	    CLEAR(&((*fr->varset.st[i])[j]));
	free(fr->varset.st[i]);
	fr->varset.top--;
    }

    for (i = 0; i < MAX_VAR; i++)
	CLEAR(&fr->variables[i]);

    fr->argument.top = 0;
    fr->pc = 0;
    if (fr->brkpt.lastcmd)
	free(fr->brkpt.lastcmd);
    if (fr->brkpt.proglines) {
	free_prog_text(fr->brkpt.proglines);
	fr->brkpt.proglines = NULL;
    }

    fr->next = free_frames_list;
    free_frames_list = fr;
    err = 0;
}


void 
reload(struct frame * fr, int atop, int stop)
{
    fr->argument.top = atop;
    fr->system.top = stop;
}


int 
false(struct inst * p)
{
    return ((p->type == PROG_STRING && (p->data.string == 0 || !(*p->data.string->data)))
	    || (p->type == PROG_LOCK && p->data.lock == TRUE_BOOLEXP)
	    || (p->type == PROG_INTEGER && p->data.number == 0)
	    || (p->type == PROG_OBJECT && p->data.objref == NOTHING));
}


void 
copyinst(struct inst * from, struct inst * to)
{
    *to = *from;
    if (from->type == PROG_STRING && from->data.string) {
	from->data.string->links++;
    }
    if (from->type == PROG_ADD) {
	from->data.addr->links++;
	DBFETCH(from->data.addr->progref)->sp.program.instances++;
    }
    if (from->type == PROG_LOCK && from->data.lock != TRUE_BOOLEXP) {
	to->data.lock = copy_bool(from->data.lock);
    }
}


void
copyvars(vars *from, vars *to)
{
    int i;
    for (i = 0; i < MAX_VAR; i++) {
        copyinst(&(*from)[i], &(*to)[i]);
    }
}


void
calc_profile_timing(dbref prog, struct frame *fr)
{
    struct timeval tv;
    struct inst *first, *second;
    gettimeofday(&tv, NULL);
    tv.tv_usec -= fr->proftime.tv_usec;
    tv.tv_sec -= fr->proftime.tv_sec;
    if (tv.tv_usec < 0) {
        tv.tv_usec += 1000000;
        tv.tv_sec -= 1;
    }
    first = DBFETCH(prog)->sp.program.code;
    second = &first[1];
    first->data.number += tv.tv_sec;
    second->data.number += tv.tv_usec;
    if (second->data.number >= 1000000) {
        second->data.number -= 1000000;
        first->data.number += 1;
    }
    fr->totaltime.tv_sec += tv.tv_sec;
    fr->totaltime.tv_usec += tv.tv_usec;
    if (fr->totaltime.tv_usec > 1000000) {
	fr->totaltime.tv_usec -= 1000000;
	fr->totaltime.tv_sec += 1;
    }
}


static int interp_depth = 0;

void 
do_abort_loop(dbref player, dbref program, const char *msg,
	      struct frame * fr, struct inst * pc, int atop, int stop,
	      struct inst * clinst1, struct inst * clinst2)
{
    if (pc) {
	calc_profile_timing(program,fr);
    }
    if (clinst1)
	CLEAR(clinst1);
    if (clinst2)
	CLEAR(clinst2);
    reload(fr, atop, stop);
    fr->pc = pc;
    if (pc) {
	interp_err(player, program, pc, fr->argument.st, fr->argument.top,
		   fr->caller.st[1], insttotext(pc, 30, program), msg);
	if (controls(player, program))
	    muf_backtrace(player, program, STACK_SIZE, fr);
    } else {
	notify_nolisten(player, msg, 1);
    }
    interp_depth--;
    prog_clean(fr);
    DBSTORE(player, sp.player.block, 0);
}


struct inst * 
interp_loop(dbref player, dbref program, struct frame * fr, int rettyp)
{
    register struct inst *pc;
    register int atop;
    register struct inst *arg;
    register struct inst *temp1;
    register struct inst *temp2;
    register struct stack_addr *sys;
    register int instr_count;
    register int stop;
    int     i, tmp, writeonly, mlev;
    static struct inst retval;


    fr->level = ++interp_depth; /* increment interp level */

    /* load everything into local stuff */
    pc = fr->pc;
    atop = fr->argument.top;
    stop = fr->system.top;
    arg = fr->argument.st;
    sys = fr->system.st;
    writeonly = fr->writeonly;
    already_created = 0;
    fr->brkpt.isread = 0;

    if (!pc) {
	struct line *tmpline;

	tmpline = DBFETCH(program)->sp.program.first;
	DBFETCH(program)->sp.program.first = (struct line *) read_program(program);
	do_compile(OWNER(program), program);
	free_prog_text(DBFETCH(program)->sp.program.first);
	DBSTORE(program, sp.program.first, tmpline);
	pc = fr->pc = DBFETCH(program)->sp.program.start;
	if (!pc) {
	    abort_loop("Program not compilable. Cannot run.", NULL, NULL);
	}
	DBFETCH(program)->sp.program.code[3].data.number++;
    }
    err = 0;

    instr_count = 0;
    mlev = ProgMLevel(program);
    gettimeofday(&fr->proftime, NULL);

    /* This is the 'natural' way to exit a function */
    while (stop) {
	fr->instcnt++;
	instr_count++;
	if ((fr->multitask == PREEMPT) || (FLAGS(program) & BUILDER)) {
	    if (mlev == 4) {
		instr_count = 0;/* if program is wizbit, then clear count */
	    } else {
		/* else make sure that the program doesn't run too long */
		if (instr_count >= tp_max_instr_count)
		    abort_loop("Maximum preempt instruction count exceeded", NULL, NULL);
	    }
	} else {
	    /* if in FOREGROUND or BACKGROUND mode, '0 sleep' every so often. */
	    if ((fr->instcnt > tp_instr_slice * 4) &&
		    (instr_count >= tp_instr_slice)) {
		fr->pc = pc;
		reload(fr, atop, stop);
		DBSTORE(player, sp.player.block, (!fr->been_background));
		add_muf_delay_event(0, player, NOTHING, NOTHING, program, fr,
		    (fr->multitask==FOREGROUND) ? "FOREGROUND" : "BACKGROUND");
		interp_depth--;
		calc_profile_timing(program,fr);
		return NULL;
	    }
	}
	if ((FLAGS(program) & ZOMBIE) &&
		!fr->been_background &&
		controls(player, program)) {
	    fr->brkpt.debugging = 1;
	} else {
	    fr->brkpt.debugging = 0;
	}
	if (FLAGS(program) & DARK ||
		(fr->brkpt.debugging &&
		 fr->brkpt.showstack && !fr->brkpt.bypass)) {
	    char   *m = debug_inst(pc, arg, atop, program);
	    notify_nolisten(player, m, 1);
	}
	if (fr->brkpt.debugging) {
	    if (fr->brkpt.count) {
		for (i = 0; i < fr->brkpt.count; i++) {
		    if (
			    (!fr->brkpt.pc[i] || pc==fr->brkpt.pc[i]) &&
			      /* pc matches */
			    (fr->brkpt.line[i] == -1 ||
			     (fr->brkpt.lastline != pc->line &&
			      fr->brkpt.line[i] == pc->line)) &&
			      /* line matches */
			    (fr->brkpt.level[i] == -1 ||
			     stop <= fr->brkpt.level[i]) &&
			      /* level matches */
			    (fr->brkpt.prog[i] == NOTHING ||
			     fr->brkpt.prog[i] == program) &&
			      /* program matches */
			    (fr->brkpt.linecount[i] == -2 ||
			     (fr->brkpt.lastline != pc->line &&
			      fr->brkpt.linecount[i]-- <= 0)) &&
			      /* line count matches */
			    (fr->brkpt.pccount[i] == -2 ||
			     (fr->brkpt.lastpc != pc &&
			      fr->brkpt.pccount[i]-- <= 0))
			      /* pc count matches */
			    ) {
			if (fr->brkpt.bypass) {
			    if (fr->brkpt.pccount[i] == -1)
				fr->brkpt.pccount[i] = 0;
			    if (fr->brkpt.linecount[i] == -1)
				fr->brkpt.linecount[i] = 0;
			} else {
			    char *m;
			    char buf[BUFFER_LEN];
			    add_muf_read_event(player, program, fr);
			    reload(fr, atop, stop);
			    fr->pc = pc;
			    fr->brkpt.isread = 0;
			    fr->brkpt.breaknum = i;
			    fr->brkpt.lastlisted = 0;
			    fr->brkpt.bypass = 0;
			    DBSTORE(player, sp.player.curr_prog, program);
			    DBSTORE(player, sp.player.block, 0);
			    interp_depth--;
			    if (!fr->brkpt.showstack) {
				m = debug_inst(pc, arg, atop, program);
				notify_nolisten(player, m, 1);
			    }
			    if (pc <= DBFETCH(program)->sp.program.code ||
				    (pc-1)->line != pc->line) {
				list_proglines(player,program,fr,pc->line,0);
			    } else {
				m = (char *)show_line_prims(program,pc,15,1);
				sprintf(buf, "     %s", m);
				notify_nolisten(player, buf, 1);
			    }
			    calc_profile_timing(program,fr);
			    return NULL;
			}
		    }
		}
	    }
	    fr->brkpt.lastline = pc->line;
	    fr->brkpt.lastpc = pc;
	    fr->brkpt.bypass = 0;
	}
	if (mlev < 3) {
	    if (fr->instcnt > (tp_max_instr_count * ((mlev == 2) ? 4 : 1)))
		abort_loop("Maximum total instruction count exceeded.", NULL, NULL);
	}
	switch (pc->type) {
	    case PROG_INTEGER:
	    case PROG_ADD:
	    case PROG_OBJECT:
	    case PROG_VAR:
	    case PROG_LVAR:
	    case PROG_STRING:
	    case PROG_LOCK:
		if (atop >= STACK_SIZE)
		    abort_loop("Stack overflow.", NULL, NULL);
		copyinst(pc, arg + atop);
		pc++;
		atop++;
		break;

	    case PROG_FUNCTION:
		pc++;
		break;

	    case PROG_IF:
		if (atop < 1)
		    abort_loop("Stack Underflow.", NULL, NULL);
		temp1 = arg + --atop;
		if (false(temp1))
		    pc = pc->data.call;
		else
		    pc++;
		CLEAR(temp1);
		break;

	    case PROG_EXEC:
		if (stop >= STACK_SIZE)
		    abort_loop("System Stack Overflow", NULL, NULL);
		sys[stop].progref = program;
		sys[stop++].offset = pc + 1;
	    case PROG_JMP:
		pc = pc->data.call;
		break;

	    case PROG_PRIMITIVE:
		/*
		 * All pc modifiers and stuff like that should stay here,
		 * everything else call with an independent dispatcher.
		 */
		switch (pc->data.number) {
		    case IN_JMP:
			if (atop < 1)
			    abort_loop("Stack underflow.  Missing address.", NULL, NULL);
			temp1 = arg + --atop;
			if (temp1->type != PROG_ADD)
			    abort_loop("Argument is not an address.", temp1, NULL);
			if (temp1->data.addr->progref > db_top ||
				temp1->data.addr->progref < 0 ||
				(Typeof(temp1->data.addr->progref) != TYPE_PROGRAM))
			    abort_loop("Internal error.  Invalid address.", temp1, NULL);
			if (program != temp1->data.addr->progref) {
			    abort_loop("Destination outside current program.", temp1, NULL);
			}
			pc = temp1->data.addr->data;
			CLEAR(temp1);
			break;
		    case IN_EXECUTE:
			if (atop < 1)
			    abort_loop("Stack Underflow. Missing address.", NULL, NULL);
			temp1 = arg + --atop;
			if (temp1->type != PROG_ADD)
			    abort_loop("Argument is not an address.", temp1, NULL);
			if (temp1->data.addr->progref > db_top ||
				temp1->data.addr->progref < 0 ||
				(Typeof(temp1->data.addr->progref) != TYPE_PROGRAM))
			    abort_loop("Internal error.  Invalid address.", temp1, NULL);
			if (stop >= STACK_SIZE)
			    abort_loop("System Stack Overflow", temp1, NULL);
			sys[stop].progref = program;
			sys[stop++].offset = pc + 1;
			if (program != temp1->data.addr->progref) {
			    program = temp1->data.addr->progref;
			    fr->caller.st[++fr->caller.top] = program;
			    mlev = ProgMLevel(program);
			    DBFETCH(program)->sp.program.instances++;
			    fr->varset.top++;
			    fr->varset.st[fr->varset.top] = (vars *) malloc(sizeof(vars));
			    copyvars(fr->varset.st[fr->varset.top - 1],
				    fr->varset.st[fr->varset.top]);
			}
			pc = temp1->data.addr->data;
			CLEAR(temp1);
			break;
		    case IN_CALL:
			if (atop < 1)
			    abort_loop("Stack Underflow. Missing dbref argument.", NULL, NULL);
			temp1 = arg + --atop;
			temp2 = 0;
			if (temp1->type != PROG_OBJECT) {
			    temp2 = temp1;
			    if (atop < 1)
				abort_loop("Stack Underflow. Missing dbref of func.", temp1, NULL);
			    temp1 = arg + --atop;
			    if (temp2->type != PROG_STRING)
				abort_loop("Public Func. name string required. (2)", temp1, temp2);
			    if (!temp2->data.string)
				abort_loop("Null string not allowed. (2)", temp1, temp2);
			}
			if (temp1->type != PROG_OBJECT)
			    abort_loop("Dbref required. (1)", temp1, temp2);
			if (!valid_object(temp1)
				|| Typeof(temp1->data.objref) != TYPE_PROGRAM)
			    abort_loop("invalid object.", temp1, temp2);
			if (!(DBFETCH(temp1->data.objref)->sp.program.code)) {
			    struct line *tmpline;

			    tmpline = DBFETCH(temp1->data.objref)->sp.program.first;
			    DBFETCH(temp1->data.objref)->sp.program.first =
				(struct line *) read_program(temp1->data.objref);
			    do_compile(OWNER(temp1->data.objref), temp1->data.objref);
			    free_prog_text(DBFETCH(temp1->data.objref)->sp.program.first);
			    DBSTORE(temp1->data.objref, sp.program.first, tmpline);
			    if (!(DBFETCH(temp1->data.objref)->sp.program.code))
				abort_loop("Program not compilable.", temp1, temp2);
			}
			if (ProgMLevel(temp1->data.objref) == 0)
			    abort_loop("Permission denied", temp1, temp2);
			if (mlev < 4 && OWNER(temp1->data.objref) != ProgUID
				&& !Linkable(temp1->data.objref))
			    abort_loop("Permission denied", temp1, temp2);
			if (stop >= STACK_SIZE)
			    abort_loop("System Stack Overflow", temp1, temp2);
			sys[stop].progref = program;
			sys[stop++].offset = pc + 1;
			if (!temp2) {
			    pc = DBFETCH(temp1->data.objref)->sp.program.start;
			} else {
			    struct publics *pbs;
			    int     tmpint;

			    pbs = DBFETCH(temp1->data.objref)->sp.program.pubs;
			    while (pbs) {
				tmpint = string_compare(temp2->data.string->data, pbs->subname);
				if (!tmpint)
				    break;
				pbs = pbs->next;
			    }
			    if (!pbs)
				abort_loop("Public Function not found. (2)", temp2, temp2);
			    pc = pbs->addr.ptr;
			}
			if (temp1->data.objref != program) {
			    calc_profile_timing(program,fr);
			    gettimeofday(&fr->proftime, NULL);
			    program = temp1->data.objref;
			    fr->caller.st[++fr->caller.top] = program;
			    DBFETCH(program)->sp.program.instances++;
			    mlev = ProgMLevel(program);
			    fr->varset.top++;
			    fr->varset.st[fr->varset.top] = (vars *) malloc(sizeof(vars));
			    copyvars(fr->varset.st[fr->varset.top - 1],
				    fr->varset.st[fr->varset.top]);
			}
			DBFETCH(program)->sp.program.code[3].data.number++;
			ts_useobject(program);
			CLEAR(temp1);
			if (temp2)
			    CLEAR(temp2);
			break;
		    case IN_RET:
			if (stop > 1 && program != sys[stop - 1].progref) {
			    if (sys[stop - 1].progref > db_top ||
				    sys[stop - 1].progref < 0 ||
				    (Typeof(sys[stop - 1].progref) != TYPE_PROGRAM))
				abort_loop("Internal error.  Invalid address.", NULL, NULL);
			    DBFETCH(program)->sp.program.instances--;
			    calc_profile_timing(program,fr);
			    gettimeofday(&fr->proftime, NULL);
			    program = sys[stop - 1].progref;
			    mlev = ProgMLevel(program);
			    {
				int i;
				for (i = 0; i < MAX_VAR; i++) {
				    CLEAR(&CurrVar[i]);
				}
			    }
			    fr->caller.top--;
			    free(fr->varset.st[fr->varset.top--]);
			}
			pc = sys[--stop].offset;
			break;
		    case IN_READ:
			if (writeonly)
			    abort_loop("Program is write-only.", NULL, NULL);
			if (fr->multitask == BACKGROUND)
			    abort_loop("BACKGROUND programs are write only.", NULL, NULL);
			reload(fr, atop, stop);
			fr->brkpt.isread = 1;
			fr->pc = pc + 1;
			DBSTORE(player, sp.player.curr_prog, program);
			DBSTORE(player, sp.player.block, 0);
			add_muf_read_event(player, program, fr);
			interp_depth--;
			calc_profile_timing(program,fr);
			return NULL;
			/* NOTREACHED */
			break;
		    case IN_SLEEP:
			if (atop < 1)
			    abort_loop("Stack Underflow.", NULL, NULL);
			temp1 = arg + --atop;
			if (temp1->type != PROG_INTEGER)
			    abort_loop("Invalid argument type.", temp1, NULL);
			fr->pc = pc + 1;
			reload(fr, atop, stop);
			if (temp1->data.number < 0)
			    abort_loop("Timetravel beyond scope of muf.", temp1, NULL);
			add_muf_delay_event(temp1->data.number, player,
				NOTHING, NOTHING, program, fr, "SLEEPING");
			DBSTORE(player, sp.player.block, (!fr->been_background));
			interp_depth--;
			calc_profile_timing(program,fr);
			return NULL;
			/* NOTREACHED */
			break;
		    default:
			nargs = 0;
			reload(fr, atop, stop);
			tmp = atop;
			prim_func[pc->data.number - 1] (player, program, mlev, pc, arg, &tmp, fr);
			atop = tmp;
			pc++;
			break;
		}               /* switch */
		break;
	    case PROG_CLEARED:
		fprintf(stderr, "Attempt to execute instruction cleared by %s:%hd in program %d\n",
		       (char *)pc->data.addr,  // This is probably WRONG
		       pc->line, program);
		pc = NULL;
		abort_loop("Program internal error. Program erroneously freed from memory.", NULL, NULL);
	    default:
		pc = NULL;
		abort_loop("Program internal error. Unknown instruction type.", NULL, NULL);
	}                       /* switch */
	if (err) {
	    reload(fr, atop, stop);
	    prog_clean(fr);
	    DBSTORE(player, sp.player.block, 0);
	    interp_depth--;
	    calc_profile_timing(program,fr);
	    return NULL;
	}
    }                           /* while */

    DBSTORE(player, sp.player.block, 0);
    if (atop) {
	struct inst *rv;
	if (rettyp) {
	    copyinst(arg + atop - 1, &retval); 
	    rv = &retval;
	} else {
	    if (!false(arg + atop - 1)) {
		rv = (struct inst *)1;
	    } else {
		rv = NULL;
	    }
	}
	reload(fr, atop, stop);
	prog_clean(fr);
	interp_depth--;
	calc_profile_timing(program,fr);
	return rv;
    }
    reload(fr, atop, stop);
    prog_clean(fr);
    interp_depth--;
    calc_profile_timing(program,fr);
    return NULL;
}


void 
interp_err(dbref player, dbref program, struct inst *pc,
	   struct inst *arg, int atop, dbref origprog,
	   const char *msg1, const char *msg2)
{
    char    buf[BUFFER_LEN];
    int     errcount;

    err++;
    if (OWNER(origprog) == OWNER(player)) {
	strcpy(buf, "Program Error.  Your program just got the following error.");
    } else {
	sprintf(buf, "Programmer Error.  Please tell %s what you typed, and the following message.",
		NAME(OWNER(origprog)));
    }
    notify_nolisten(player, buf, 1);

    sprintf(buf, "%s(#%d), line %d; %s: %s", NAME(program), program, pc->line,
	    msg1, msg2);
    notify_nolisten(player, buf, 1);

    errcount = get_property_value(origprog, ".debug/errcount");
    errcount++;
    add_property(origprog, ".debug/errcount", NULL, errcount);
    add_property(origprog, ".debug/lasterr", buf, 0);
    add_property(origprog, ".debug/lastcrash", NULL, (int)current_systime);

    if (origprog != program) {
	errcount = get_property_value(program, ".debug/errcount");
	errcount++;
	add_property(program, ".debug/errcount", NULL, errcount);
	add_property(program, ".debug/lasterr", buf, 0);
	add_property(program, ".debug/lastcrash", NULL, (int)current_systime);
    }
}



void 
push(struct inst *stack, int *top, int type, void *res)
{
    stack[*top].type = type;
    if (type < PROG_STRING)
	stack[*top].data.number = *(int *) res;
    else
	stack[*top].data.string = (struct shared_string *) res;
    (*top)++;
}


int 
valid_player(struct inst * oper)
{
    return (!(oper->type != PROG_OBJECT || oper->data.objref >= db_top
	      || oper->data.objref < 0
	      || (Typeof(oper->data.objref) != TYPE_PLAYER)));
}



int 
valid_object(struct inst * oper)
{
    return (!(oper->type != PROG_OBJECT || oper->data.objref >= db_top
	      || (oper->data.objref < 0)
	      || Typeof(oper->data.objref) == TYPE_GARBAGE));
}


int 
is_home(struct inst * oper)
{
    return (oper->type == PROG_OBJECT && oper->data.objref == HOME);
}


int 
permissions(dbref player, dbref thing)
{

    if (thing == player || thing == HOME)
	return 1;

    switch (Typeof(thing)) {
	case TYPE_PLAYER:
	    return 0;
	case TYPE_EXIT:
	    return (OWNER(thing) == OWNER(player) || OWNER(thing) == NOTHING);
	case TYPE_ROOM:
	case TYPE_THING:
	case TYPE_PROGRAM:
	    return (OWNER(thing) == OWNER(player));
    }

    return 0;
}

dbref 
find_mlev(dbref prog, struct frame * fr, int st)
{
    if ((FLAGS(prog) & STICKY) && (FLAGS(prog) & HAVEN)) {
	if ((st > 1) && (TrueWizard(OWNER(prog))))
	    return (find_mlev(fr->caller.st[st - 1], fr, st - 1));
    }
    if (MLevel(prog) < MLevel(OWNER(prog))) {
	return (MLevel(prog));
    } else {
	return (MLevel(OWNER(prog)));
    }
}


dbref 
find_uid(dbref player, struct frame * fr, int st, dbref program)
{
    if ((FLAGS(program) & STICKY) || (fr->perms == STD_SETUID)) {
	if (FLAGS(program) & HAVEN) {
	    if ((st > 1) && (TrueWizard(OWNER(program))))
		return (find_uid(player, fr, st - 1, fr->caller.st[st - 1]));
	    return (OWNER(program));
	}
	return (OWNER(program));
    }
    if (ProgMLevel(program) < 2)
	return (OWNER(program));
    if ((FLAGS(program) & HAVEN) || (fr->perms == STD_HARDUID)) {
	if (fr->trig == NOTHING)
	    return (OWNER(program));
	return (OWNER(fr->trig));
    }
    return (OWNER(player));
}


void 
do_abort_interp(dbref player, const char *msg, struct inst * pc,
		struct inst * arg, int atop, struct frame *fr,
		struct inst * oper1, struct inst * oper2,
		struct inst * oper3, struct inst * oper4, int nargs,
		dbref program, char *file, int line)
{
    fr->pc = pc;
    calc_profile_timing(program,fr);
    interp_err(player, program, pc, arg, atop, fr->caller.st[1],
	       insttotext(pc, 30, program), msg);
    if (controls(player, program))
	muf_backtrace(player, program, STACK_SIZE, fr);
    switch (nargs) {
	case 4:
	    RCLEAR(oper4, file, line);
	case 3:
	    RCLEAR(oper3, file, line);
	case 2:
	    RCLEAR(oper2, file, line);
	case 1:
	    RCLEAR(oper1, file, line);
    }
    return;
}


void
do_abort_silent()
{
    err++;
}


