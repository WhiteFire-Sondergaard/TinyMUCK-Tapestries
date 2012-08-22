/*
  Timequeue event code by Foxen
*/

#include "copyright.h"
#include "config.h"
#include "params.h"
#include "match.h"

#include "db.h"
#include "tune.h"
#include "mpi.h"
#include "mlua.h"
#include "props.h"
#include "interface.h"
#include "externs.h"

#include <stdio.h>
#include <sys/types.h>
#include <time.h>


#define TQ_MUF_TYP 0
#define TQ_MPI_TYP 1
#define TQ_LUA_TYP 2

#define TQ_QUEUE    0x0
#define TQ_DELAY    0x1
#define TQ_LISTEN   0x2
#define TQ_READ     0x3
#define TQ_SUBMASK  0x7
#define TQ_OMESG    0x8

/*
 * Legacy, please use the above.
 */
#define TQ_MUF_QUEUE    TQ_QUEUE
#define TQ_MUF_DELAY    TQ_DELAY
#define TQ_MUF_LISTEN   TQ_LISTEN
#define TQ_MUF_READ     TQ_READ
 
#define TQ_MPI_QUEUE    TQ_QUEUE
#define TQ_MPI_DELAY    TQ_DELAY

#define TQ_MPI_SUBMASK  TQ_SUBMASK
#define TQ_MPI_LISTEN   TQ_LISTEN
#define TQ_MPI_OMESG    TQ_OMESG


/*
 * Events types and data:
 *  What, typ, sub, when, user, where, trig, prog, frame, str1, cmdstr, str3
 *  qmpi   1    0   1     user  loc    trig  --    --     mpi   cmd     arg
 *  dmpi   1    1   when  user  loc    trig  --    --     mpi   cmd     arg
 *  lmpi   1    8   1     spkr  loc    lstnr --    --     mpi   cmd     heard
 *  oqmpi  1   16   1     user  loc    trig  --    --     mpi   cmd     arg
 *  odmpi  1   17   when  user  loc    trig  --    --     mpi   cmd     arg
 *  olmpi  1   24   1     spkr  loc    lstnr --    --     mpi   cmd     heard
 *  qmuf   0    0   0     user  loc    trig  prog  --     stk_s cmd@    --
 *  lmuf   0    1   0     spkr  loc    lstnr prog  --     heard cmd@    --
 *  dmuf   0    2   when  user  loc    trig  prog  frame  mode  --      --
 *  rmuf   0    3   -1    user  loc    trig  prog  frame  mode  --      --
 */

class TimeNode {
public:
    static const unsigned int MUF_TYP = 0;
    static const unsigned int MPI_TYP = 1;
    static const unsigned int LUA_TYP = 2;

    static const unsigned int QUEUE   = 0x0;
    static const unsigned int DELAY   = 0x1;
    static const unsigned int LISTEN  = 0x2;
    static const unsigned int READ    = 0x3;
    static const unsigned int SUBMASK = 0x7;
    static const unsigned int OMESG   = 0x8;

private:
    TimeNode *next;         // Linked list
    int     typ;            // Script type
    int     subtyp;         // State
    time_t  when;           // When to next execute
    dbref   called_prog;    // Dbref of called program
    char   *called_data;    // str1
    char   *command;        // cmdstr
    char   *str3;           // str3
    dbref   uid;            // user or "speaker"
    dbref   loc;            // Location of event
    dbref   trig;           // triggering object
    struct frame *fr;       // Muf interp
    struct inst *where;     // Instruction pointer
    int     eventnum;       // event ID

    static TimeNode *head;
};

TimeNode *TimeNode::head = NULL;

typedef struct timenode {
    struct timenode *next;  // Linked list
    int     typ;            // Script type
    int     subtyp;         // State
    time_t  when;           // When to next execute
    dbref   called_prog;    // Dbref of called program
    char   *called_data;    // str1
    char   *command;        // cmdstr
    char   *str3;           // str3
    dbref   uid;            // user or "speaker"
    dbref   loc;            // Location of event
    dbref   trig;           // triggering object
    struct frame *fr;       // Muf interp
    struct inst *where;     // Instruction pointer
    int     eventnum;       // event ID
}      *timequeue;

/*
 * Various globals local to this file. To make sure this code is not
 * thread-safe.
 */

static timequeue tqhead = NULL;

void    prog_clean(struct frame * fr);
static int has_refs(dbref program, timequeue ptr);

extern int top_pid;
int     process_count = 0;

static timequeue free_timenode_list = NULL;
static int free_timenode_count = 0;

/*
 * Why this is defined here and not in the database code is beyond me.
 */
static int
valid_objref(dbref obj)
{
    return (!((obj >= db_top)
              || (obj >= 0 &&
                  Typeof(obj) == TYPE_GARBAGE)
              || (obj < 0)));
}


static  timequeue
alloc_timenode(int typ, int subtyp, time_t mytime, dbref player, dbref loc,
               dbref trig, dbref program, struct frame * fr,
               const char *strdata, const char *strcmd, const char *str3,
               timequeue nextone)
{
    timequeue ptr;

    if (free_timenode_list) {
        ptr = free_timenode_list;
        free_timenode_list = ptr->next;
        free_timenode_count--;
    } else {
        ptr = (timequeue) malloc(sizeof(struct timenode));
    }
    ptr->typ = typ;
    ptr->subtyp = subtyp;
    ptr->when = mytime;
    ptr->uid = player;
    ptr->loc = loc;
    ptr->trig = trig;
    ptr->fr = fr;
    ptr->called_prog = program;
    ptr->called_data = (char *) string_dup((char *) strdata);
    ptr->command = alloc_string(strcmd);
    ptr->str3 = alloc_string(str3);
    ptr->eventnum = (fr) ? fr->pid : top_pid++;
    ptr->next = nextone;
    return (ptr);
}

static void
free_timenode(timequeue ptr)
{
    if (ptr->command) free(ptr->command);
    if (ptr->called_data) free(ptr->called_data);
    if (ptr->str3) free(ptr->str3);
    if (ptr->fr) {
        if (ptr->fr->multitask != BACKGROUND)
            DBFETCH(ptr->uid)->sp.player.block = 0;
        prog_clean(ptr->fr);
        if (ptr->subtyp == TQ_READ) {
            FLAGS(ptr->uid) &= ~INTERACTIVE;
            FLAGS(ptr->uid) &= ~READMODE;
            notify_nolisten(ptr->uid, "Data input aborted.  The command you were using was killed.", 1);
        }
    }
    if (free_timenode_count < tp_free_frames_pool) {
        ptr->next = free_timenode_list;
        free_timenode_list = ptr;
        free_timenode_count++;
    } else {
        free(ptr);
    }
}

/*
 Does the player control process count (where count is really an eventnum)
 This is making some wild ass assumptions about who the process is running as.
 */
int
control_process(dbref player, int count)
{
    timequeue tmp, ptr = tqhead;

    tmp = ptr;
    while ((ptr) && (count != ptr->eventnum)) {
        tmp = ptr;
        ptr = ptr->next;
    }

    if (!ptr)
        return 0;

    if (!controls(player, ptr->called_prog) &&
            !controls(player, ptr->trig)) {
        return 0;
    }
    return 1;
}

int
add_event(int event_typ, int subtyp, int dtime, dbref player, dbref loc,
          dbref trig, dbref program, struct frame * fr,
          const char *strdata, const char *strcmd, const char *str3)
{
    timequeue ptr = tqhead;
    timequeue lastevent = NULL;
    time_t  rtime = time((time_t *) NULL) + (time_t) dtime;
    int mypids = 0;

    for (ptr = tqhead, mypids = 0; ptr; ptr = ptr->next) {
        if (ptr->uid == player) mypids++;
        lastevent = ptr;
    }

    if (subtyp == TQ_READ) {
        process_count++;
        if (lastevent) {
            lastevent->next = alloc_timenode(event_typ, subtyp, rtime,
                                             player, loc, trig, program, fr,
                                             strdata, strcmd, str3, NULL);
            return (lastevent->next->eventnum);
        } else {
            tqhead = alloc_timenode(event_typ, subtyp, rtime,
                                     player, loc, trig, program, fr,
                                     strdata, strcmd, str3, NULL);
            return (tqhead->eventnum);
        }
    }

    if (process_count > tp_max_process_limit ||
            (mypids > tp_max_plyr_processes && !Wizard(OWNER(player)))) {
        if (fr) {
            if (fr->multitask != BACKGROUND)
                DBFETCH(player)->sp.player.block = 0;
            prog_clean(fr);
        }
        notify_nolisten(player, "Event killed.  Timequeue table full.", 1);
        return 0;
    }
    process_count++;

    if (!tqhead) {
        tqhead = alloc_timenode(event_typ, subtyp, rtime, player, loc, trig,
                                program, fr, strdata, strcmd, str3, NULL);
        return (tqhead->eventnum);
    }
    if (rtime < tqhead->when || tqhead->subtyp == TQ_READ)
    {
        tqhead = alloc_timenode(event_typ, subtyp, rtime, player, loc, trig,
                                program, fr, strdata, strcmd, str3, tqhead);
        return (tqhead->eventnum);
    }

    ptr = tqhead;
    while ((ptr->next) && (rtime >= ptr->next->when) &&
            !(ptr->next->subtyp == TQ_READ))
    {
        ptr = ptr->next;
    }

    ptr->next = alloc_timenode(event_typ, subtyp, rtime, player, loc, trig,
                               program, fr, strdata, strcmd, str3, ptr->next);
    return (ptr->next->eventnum);
}


int
add_mpi_event(int delay, dbref player, dbref loc, dbref trig,
              const char *mpi, const char *cmdstr, const char *argstr,
              int listen_p, int omesg_p)
{
    int subtyp = TQ_MPI_QUEUE;

    if (delay >= 1) {
        subtyp = TQ_MPI_DELAY;
    }
    if (listen_p)  {
        subtyp |= TQ_MPI_LISTEN;
    }
    if (omesg_p) {
        subtyp |= TQ_MPI_OMESG;
    }
    return add_event(TQ_MPI_TYP, subtyp, delay, player, loc, trig,
                     NOTHING, NULL, mpi, cmdstr, argstr);
}


int
add_muf_queue_event(dbref player, dbref loc, dbref trig, dbref prog,
                    const char *argstr, const char *cmdstr, int listen_p)
{
    return add_event(TQ_MUF_TYP, (listen_p? TQ_LISTEN: TQ_QUEUE), 0,
                     player, loc, trig, prog, NULL, argstr, cmdstr, NULL);
}


int
add_muf_delayq_event(int delay, dbref player, dbref loc, dbref trig,
                    dbref prog, const char *argstr, const char *cmdstr,
                    int listen_p)
{
    return add_event(TQ_MUF_TYP, (listen_p? TQ_LISTEN: TQ_QUEUE),
                     delay, player, loc, trig, prog, NULL, argstr, cmdstr,
                     NULL);
}


int
add_muf_read_event(dbref player, dbref prog, struct frame *fr)
{
    FLAGS(player) |= (INTERACTIVE | READMODE);
    return add_event(TQ_MUF_TYP, TQ_READ, -1, player, -1, fr->trig,
                     prog, fr, "READ", NULL, NULL);
}


int
add_muf_delay_event(int delay, dbref player, dbref loc, dbref trig, dbref prog,
                    struct frame *fr, const char *mode)
{
    return add_event(TQ_MUF_TYP, TQ_DELAY, delay, player, loc, trig,
                     prog, fr, mode, NULL, NULL);
}

void
handle_read_event(dbref player, const char *command)
{
    struct frame *fr;
    timequeue ptr, lastevent;
    int flag;
    dbref prog;

    FLAGS(player) &= ~(INTERACTIVE | READMODE);

    ptr = tqhead;
    lastevent = NULL;
    while (ptr) {
        if (ptr->subtyp == TQ_READ && ptr->uid == player) {
            break;
        }
        lastevent = ptr;
        ptr = ptr->next;
    }

    /*
     * When execution gets to here, either ptr will point to the
     * READ event for the player, or else ptr will be NULL.
     */

    if (ptr) {
        /* remember our program, and our execution frame. */
        fr = ptr->fr;
        prog = ptr->called_prog;

        /* remove the READ timequeue node from the timequeue */
        process_count--;
        if (lastevent) {
            lastevent->next = ptr->next;
        } else {
            tqhead = ptr->next;
        }

        /* remember next timequeue node, to check for more READs later */
        lastevent = ptr;
        ptr = ptr->next;

        /* Make SURE not to let the program frame get freed.  We need it. */
        lastevent->fr = NULL;

        /*
         * Free up the READ timequeue node
         * we just removed from the queue.
         */
        free_timenode(lastevent);

        if (fr->brkpt.debugging && !fr->brkpt.isread) {

            /* We're in the MUF debugger!  Call it with the input line. */
            if(muf_debugger(player, prog, command, fr)) {

                /* MUF Debugger exited.  Free up the program frame & exit */
                prog_clean(fr);
                return;
            }

        } else {
            /* This is a MUF READ event. */
            if (!string_compare(command, BREAK_COMMAND)) {

                /* Whoops!  The user typed @Q.  Free the frame and exit. */
                prog_clean(fr);
                return;
            }

            if (fr->argument.top >= STACK_SIZE) {

                /*
                 * Uh oh! That MUF program's stack is full!
                 * Print an error, free the frame, and exit.
                 */
                notify_nolisten(player, "Program stack overflow.", 1);
                prog_clean(fr);
                return;
            }

            /*
             * Everything looks okay.  Lets stuff the input line
             * on the program's argument stack as a string item.
             */
            fr->argument.st[fr->argument.top].type = PROG_STRING;
            fr->argument.st[fr->argument.top++].data.string =
                alloc_prog_string(command);
        }

        /*
         * When using the MUF Debugger, the debugger will set the
         * INTERACTIVE bit on the user, if it does NOT want the MUF
         * program to resume executing.
         */
        flag = (FLAGS(player) & INTERACTIVE);

        if (!flag) {
            interp_loop(player, prog, fr, 0);
        }

        /*
         * Check for any other READ events for this player.
         * If there are any, set the READ related flags.
         */
        while (ptr) {
            if (ptr->typ == TQ_MUF_TYP && ptr->subtyp == TQ_MUF_READ) {
                if (ptr->uid == player) {
                    FLAGS(player) |= (INTERACTIVE | READMODE);
                }
            }
            ptr = ptr->next;
        }
    }
}


void
next_timequeue_event()
{
    dbref   tmpcp;
    int     tmpbl, tmpfg;
    timequeue lastevent, event;
    int     maxruns = 0;
    time_t  rtime = time((time_t *) NULL);

    lastevent = tqhead;
    while ((lastevent) && (rtime >= lastevent->when) && (maxruns < 30)) {
        lastevent = lastevent->next;
        maxruns++;
    }

    while (tqhead && (tqhead != lastevent) && (maxruns--)) {
        if (tqhead->typ == TQ_MUF_TYP && tqhead->subtyp == TQ_MUF_READ) {
            break;
        }
        event = tqhead;
        tqhead = tqhead->next;

        event->eventnum = 0;
        if (event->typ == TQ_MPI_TYP) {
            char cbuf[BUFFER_LEN];
            int ival;

            strcpy(match_args, event->str3? event->str3 : "");
            strcpy(match_cmdname, event->command? event->command : "");
            ival = (event->subtyp & TQ_MPI_OMESG)?
                    MPI_ISPUBLIC : MPI_ISPRIVATE;
            if (event->subtyp & TQ_MPI_LISTEN) {
                ival |= MPI_ISLISTENER;
                do_parse_mesg(event->uid, event->trig, event->called_data,
                              "(MPIlisten)", cbuf, ival);
            } else if ((event->subtyp & TQ_MPI_SUBMASK) == TQ_MPI_DELAY) {
                do_parse_mesg(event->uid, event->trig, event->called_data,
                              "(MPIdelay)", cbuf, ival);
            } else {
                do_parse_mesg(event->uid, event->trig, event->called_data,
                              "(MPIqueue)", cbuf, ival);
            }
            if (*cbuf) {
                if (!(event->subtyp & TQ_MPI_OMESG)) {
                    notify_nolisten(event->uid, cbuf, 1);
                } else {
                    char bbuf[BUFFER_LEN];
                    dbref plyr;
                    sprintf(bbuf, ">> %.4000s %.*s",
                            NAME(event->uid),
                            (int)(4000 - strlen(NAME(event->uid))),
                            pronoun_substitute(event->uid, cbuf));
                    plyr = DBFETCH(event->loc)->contents;
                    for (;plyr != NOTHING; plyr = DBFETCH(plyr)->next) {
                        if (Typeof(plyr)==TYPE_PLAYER && plyr!=event->uid)
                            notify_nolisten(plyr, bbuf, 0);
                    }
                }
            }
        } else if (event->typ == TQ_MUF_TYP) {
            if (Typeof(event->called_prog) == TYPE_PROGRAM) {
                if (event->subtyp == TQ_MUF_DELAY) {
                    tmpcp = DBFETCH(event->uid)->sp.player.curr_prog;
                    tmpbl = DBFETCH(event->uid)->sp.player.block;
                    tmpfg = (event->fr->multitask != BACKGROUND);
                    interp_loop(event->uid,event->called_prog,event->fr,0);
                    if (!tmpfg) {
                        DBFETCH(event->uid)->sp.player.block = tmpbl;
                    }
                } else {
                    strcpy(match_args,
                            event->called_data? event->called_data : "");
                    strcpy(match_cmdname,
                            event->command? event->command : "");
                    interp(event->uid, event->loc, event->called_prog,
                           event->trig, BACKGROUND, STD_HARDUID, 0);
                }
            }
        }
        event->fr = NULL;
        free_timenode(event);
        process_count--;
    }
}


int
in_timequeue(int pid)
{
    timequeue ptr = tqhead;

    if (!pid) return 0;
    if (!tqhead) return 0;
    while ((ptr) && (ptr->eventnum != pid))
        ptr = ptr->next;
    if (ptr)
        return 1;
    return 0;
}


long
next_event_time()
{
    time_t  rtime = time((time_t *) NULL);

    if (tqhead) {
        if (tqhead->when == -1) {
            return (-1L);
        } else if (rtime >= tqhead->when) {
            return (0L);
        } else {
            return ((long) (tqhead->when - rtime));
        }
    }
    return (-1L);
}

extern char *time_format_2(long dt);

void
list_events(dbref player)
{
    char    buf[BUFFER_LEN];
    char    buf2[BUFFER_LEN];
    int     count = 0;
    timequeue ptr = tqhead;
    time_t  rtime = time((time_t *) NULL);
    time_t  etime = 0;
    double  pcnt = 0;

    notify_nolisten(player, "     PID Next  Run KInst %CPU Prog#   Player", 1);

    while (ptr) {
        strcpy(buf2, ((ptr->when - rtime) > 0) ?
               time_format_2((long) (ptr->when - rtime)) : "Due");
        if (ptr->fr) {
            etime = rtime - ptr->fr->started;
            if (etime > 0) {
                pcnt = ptr->fr->totaltime.tv_sec;
                pcnt += ptr->fr->totaltime.tv_usec / 1000000;
                pcnt = pcnt * 100 / etime;
                if (pcnt > 100.0) {
                    pcnt = 100.0;
                }
            } else {
                pcnt = 0.0;
            }
        }
        if (ptr->typ == TQ_MUF_TYP && ptr->subtyp == TQ_MUF_DELAY) {
            (void) sprintf(buf, "%8d %4s %4s %5d %4.1f #%-6d %-16s %.512s",
                           ptr->eventnum, buf2,
                           time_format_2((long) etime),
                           (ptr->fr->instcnt / 1000), pcnt,
                           ptr->called_prog, NAME(ptr->uid),
                           ptr->called_data);
        } else if (ptr->typ == TQ_MUF_TYP && ptr->subtyp == TQ_MUF_READ) {
            (void) sprintf(buf, "%8d %4s %4s %5d %4.1f #%-6d %-16s %.512s",
                           ptr->eventnum, "--",
                           time_format_2((long) etime),
                           (ptr->fr->instcnt / 1000), pcnt,
                           ptr->called_prog, NAME(ptr->uid),
                           ptr->called_data);
        } else if (ptr->typ == TQ_MPI_TYP) {
            (void) sprintf(buf, "%8d %4s   --   MPI   -- #%-6d %-16s \"%.512s\"",
                           ptr->eventnum, buf2, ptr->trig, NAME(ptr->uid),
                           ptr->called_data);
        } else {
            (void) sprintf(buf, "%8d %4s   0s     0   -- #%-6d %-16s \"%.512s\"",
                           ptr->eventnum, buf2, ptr->called_prog,
                           NAME(ptr->uid), ptr->called_data);
        }
        if (Wizard(OWNER(player)) ||
            ((ptr->called_prog != NOTHING) &&
             (OWNER(ptr->called_prog) == OWNER(player))) ||
            (ptr->uid == player))
            notify_nolisten(player, buf, 1);
        else if (ptr->called_prog == NOTHING)
            fprintf(stderr, "Strangeness alert!  @ps produces %s\n",
                buf);
        ptr = ptr->next;
        count++;
    }
    sprintf(buf, "%d events.", count);
    notify_nolisten(player, buf, 1);
}

/*
 * Sleeponly values:
 *     0: kill all matching processes
 *     1: kill only matching sleeping processes
 *     2: kill only matching foreground processes
 */
int
dequeue_prog(dbref program, int sleeponly)
{
    int     count = 0;
    timequeue tmp, ptr;

    while (tqhead && ((tqhead->called_prog==program) ||
            has_refs(program, tqhead) || (tqhead->uid==program))
            && ((tqhead->fr) ? (!((tqhead->fr->multitask == BACKGROUND) &&
                                  (sleeponly == 2))) : (!sleeponly))) {
        ptr = tqhead;
        tqhead = tqhead->next;
        free_timenode(ptr);
        process_count--;
        count++;
    }

    if (tqhead) {
        tmp = tqhead;
        ptr = tqhead->next;
        while (ptr) {
            if ((ptr->called_prog == program) ||
                    (has_refs(program, ptr)) || 
                    ( (ptr->uid == program) && 
                      ((ptr->fr) ? (!((ptr->fr->multitask == BACKGROUND) &&
                      (sleeponly == 2))) : (!sleeponly)) )) {
                tmp->next = ptr->next;
                free_timenode(ptr);
                process_count--;
                count++;
                ptr = tmp;
            }
            tmp = ptr;
            ptr = ptr->next;
        }
    }
    for (ptr = tqhead; ptr; ptr = ptr->next) {
        if (ptr->typ == TQ_MUF_TYP && ptr->subtyp == TQ_MUF_READ) {
            FLAGS(ptr->uid) |= (INTERACTIVE | READMODE);
        }
    }
    return (count);
}


int
dequeue_process(int pid)
{
    timequeue tmp, ptr = tqhead;

    if (!pid) return 0;

    tmp = ptr;
    while ((ptr) && (pid != ptr->eventnum)) {
        tmp = ptr;
        ptr = ptr->next;
    }
    if (!tmp) return 0;
    if (!ptr) return 0;
    if (tmp == ptr) {
        tqhead = ptr->next;
    } else {
        tmp->next = ptr->next;
    }
    free_timenode(ptr);
    process_count--;
    for (ptr = tqhead; ptr; ptr = ptr->next) {
        if (ptr->typ == TQ_MUF_TYP && ptr->subtyp == TQ_MUF_READ) {
            FLAGS(ptr->uid) |= (INTERACTIVE | READMODE);
        }
    }
    return 1;
}

void
do_dequeue(dbref player, const char *arg1)
{
    char    buf[BUFFER_LEN];
    int     count;
    dbref   match;
    struct match_data md;
    timequeue tmp, ptr = tqhead;


    if (*arg1 == '\0') {
        notify_nolisten(player, "What event do you want to dequeue?", 1);
    } else {
        if (!string_compare(arg1, "all")) {
            if (!Wizard(OWNER(player))) {
                notify_nolisten(player, "Permission denied", 1);
                return;
            }
            while (ptr) {
                tmp = ptr;
                ptr = ptr->next;
                free_timenode(tmp);
                process_count--;
            }
            tqhead = NULL;
            notify_nolisten(player, "Time queue cleared.", 1);
        } else {
            if (!number(arg1)) {
                init_match(player, arg1, NOTYPE, &md);
                match_absolute(&md);
                match_everything(&md);

                match = noisy_match_result(&md);
                if (match == NOTHING) {
                    notify_nolisten(player, "I don't know what you want to dequeue!", 1);
                    return;
                }
                if (!valid_objref(match)) {
                    notify_nolisten(player, "I don't recognize that object.", 1);
                    return;
                }
                if ((!Wizard(OWNER(player))) &&
                        (OWNER(match) != OWNER(player))) {
                    notify_nolisten(player, "Permission denied.", 1);
                    return;
                }
                count = dequeue_prog(match, 0);
                if (!count) {
                    notify_nolisten(player, "That program wasn't in the time queue.", 1);
                    return;
                }
                if (count > 1) {
                    sprintf(buf, "%d processes dequeued.", count);
                } else {
                    sprintf(buf, "Process dequeued.");
                }
                notify_nolisten(player, buf, 1);
            } else {
                if ((count = atoi(arg1))) {
                    if (!control_process(player, count)) {
                        notify_nolisten(player, "Permission denied.", 1);
                        return;
                    }
                    if (!dequeue_process(count)) {
                        notify_nolisten(player, "No such process!", 1);
                        return;
                    }
                    process_count--;
                    notify_nolisten(player, "Process dequeued.", 1);
                } else {
                    notify_nolisten(player, "What process do you want to dequeue?", 1);
                }
            }
        }
    }
    return;
}


/* Checks the MUF timequeue for address references on the stack or */
/* dbref references on the callstack */
static int
has_refs(dbref program, timequeue ptr)
{
    int loop;
    if (ptr->typ != TQ_MUF_TYP || !(ptr->fr) ||
            Typeof(program) != TYPE_PROGRAM ||
            !(DBFETCH(program)->sp.program.instances))
        return 0;

    for (loop = 1; loop < ptr->fr->caller.top; loop++) {
        if (ptr->fr->caller.st[loop] == program)
            return 1;
    }

    for (loop = 0; loop < ptr->fr->argument.top; loop++) {
        if (ptr->fr->argument.st[loop].type == PROG_ADD &&
                ptr->fr->argument.st[loop].data.addr->progref == program)
            return 1;
    }

    return 0;
}


int
scan_instances(dbref program)
{
    timequeue tq = tqhead;
    int i = 0, loop;
    while (tq) {
        if (tq->typ == TQ_MUF_TYP && tq->fr) {
            if (tq->called_prog == program) {
                i++;
            }
            for (loop = 1; loop < tq->fr->caller.top; loop++) {
                if (tq->fr->caller.st[loop] == program)
                i++;
            }
            for (loop = 0; loop < tq->fr->argument.top; loop++) {
                if (tq->fr->argument.st[loop].type == PROG_ADD &&
                        tq->fr->argument.st[loop].data.addr->progref == program)
                    i++;
            }
        }
        tq = tq->next;
    }
    return i;
}


static int propq_level = 0;
void
propqueue(dbref player, dbref where, dbref trigger, dbref what, dbref xclude,
          const char *propname, const char *toparg, int mlev, int mt)
{
    const char *tmpchar;
    const char *pname;
    dbref   the_prog;
    char    buf[BUFFER_LEN];
    char    exbuf[BUFFER_LEN];

    the_prog = NOTHING;
    tmpchar = NULL;

    /* queue up program referred to by the given property */
    if (((the_prog = get_property_dbref(what, propname)) != NOTHING) ||
            (tmpchar = get_property_class(what, propname))) {
#ifdef COMPRESS
        if (tmpchar)
            tmpchar = uncompress(tmpchar);
#endif
        if ((tmpchar && *tmpchar) || the_prog != NOTHING) {
            if (tmpchar) {
                if (*tmpchar == '&') {
                    the_prog = AMBIGUOUS;
                } else if (*tmpchar == '#' && number(tmpchar+1)) {
                    the_prog = (dbref) atoi(++tmpchar);
                } else if (*tmpchar == '$') {
                    the_prog = find_registered_obj(what, tmpchar);
                } else if (number(tmpchar)) {
                    the_prog = (dbref) atoi(tmpchar);
                } else {
                    the_prog = NOTHING;
                }
            } else {
                if (the_prog == AMBIGUOUS)
                    the_prog = NOTHING;
            }
            if (the_prog != AMBIGUOUS) {
                if (the_prog < 0 || the_prog >= db_top) {
                    the_prog = NOTHING;
                } else if (Typeof(the_prog) != TYPE_PROGRAM) {
                    the_prog = NOTHING;
                } else if ((OWNER(the_prog) != OWNER(player)) &&
                        !(FLAGS(the_prog) & LINK_OK)) {
                    the_prog = NOTHING;
                } else if (MLevel(the_prog) < mlev) {
                    the_prog = NOTHING;
                } else if (MLevel(OWNER(the_prog)) < mlev) {
                    the_prog = NOTHING;
                } else if (the_prog == xclude) {
                    the_prog = NOTHING;
                }
            }
            if (propq_level < 8) {
                propq_level++;
                if (the_prog == AMBIGUOUS) {
                    char cbuf[BUFFER_LEN];
                    int ival;

                    strcpy(match_args, "");
                    strcpy(match_cmdname, toparg);
                    ival = (mt == 0)? MPI_ISPUBLIC : MPI_ISPRIVATE;
                    do_parse_mesg(player, what, tmpchar+1,
                                  "(MPIqueue)", cbuf, ival);
                    if (*cbuf) {
                        if (mt) {
                            notify_nolisten(player, cbuf, 1);
                        } else {
                            char bbuf[BUFFER_LEN];
                            dbref plyr;
                            sprintf(bbuf, ">> %.4000s",
                                    pronoun_substitute(player, cbuf));
                            plyr = DBFETCH(where)->contents;
                            while (plyr != NOTHING) {
                                if (Typeof(plyr)==TYPE_PLAYER && plyr!=player)
                                    notify_nolisten(plyr, bbuf, 0);
                                plyr = DBFETCH(plyr)->next;
                            }
                        }
                    }
                } else if (the_prog != NOTHING) {
                    strcpy(match_args, toparg? toparg : "");
                    strcpy(match_cmdname, "Queued event.");
                    interp(player, where, the_prog, trigger,
                           BACKGROUND, STD_HARDUID, 0);
                }
                propq_level--;
            } else {
                notify_nolisten(player, "Propqueue stopped to prevent infinite loop.", 1);
            }
        }
    }
    strcpy(buf, propname);
    if (is_propdir(what, buf)) {
        strcat(buf, "/");
        while ((pname = next_prop_name(what, exbuf, buf))) {
            strcpy(buf, pname);
            propqueue(player,where,trigger,what,xclude,buf,toparg,mlev,mt);
        }
    }
}


void
envpropqueue(dbref player, dbref where, dbref trigger, dbref what, dbref xclude,
             const char *propname, const char *toparg, int mlev, int mt)
{
    while (what != NOTHING) {
        propqueue(player,where,trigger,what,xclude,propname,toparg,mlev,mt);
        what = getparent(what);
    }
}


void
listenqueue(dbref player, dbref where, dbref trigger, dbref what, dbref xclude,
        const char *propname, const char *toparg, int mlev, int mt, int mpi_p)
{
    const char *tmpchar;
    const char *pname, *sep, *ptr;
    dbref   the_prog = NOTHING;
    char    buf[BUFFER_LEN];
    char    exbuf[BUFFER_LEN];
    char *ptr2;

    if (!(FLAGS(what) & LISTENER) && !(FLAGS(OWNER(what)) & ZOMBIE)) return;

    the_prog = NOTHING;
    tmpchar = NULL;

    /* queue up program referred to by the given property */
    if (((the_prog = get_property_dbref(what, propname)) != NOTHING) ||
            (tmpchar = get_property_class(what, propname))) {

        if (tmpchar) {
#ifdef COMPRESS
            tmpchar = uncompress(tmpchar);
#endif
            sep = tmpchar;
            while (*sep) {
                if (*sep == '\\') {
                    sep++;
                } else if (*sep == '=') {
                    break;
                }
                if (*sep) sep++;
            }
            if (*sep == '=') {
                for (ptr = tmpchar, ptr2 = buf; ptr < sep; *ptr2++ = *ptr++);
                *ptr2 = '\0';
                strcpy(exbuf, toparg);
                if (!equalstr(buf, exbuf)) {
                    tmpchar = NULL;
                } else {
                    tmpchar = ++sep;
                }
            }
        }

        if ((tmpchar && *tmpchar) || the_prog != NOTHING) {
            if (tmpchar) {
                if (*tmpchar == '&') {
                    the_prog = AMBIGUOUS;
                } else if (*tmpchar == '#' && number(tmpchar+1)) {
                    the_prog = (dbref) atoi(++tmpchar);
                } else if (*tmpchar == '$') {
                    the_prog = find_registered_obj(what, tmpchar);
                } else if (number(tmpchar)) {
                    the_prog = (dbref) atoi(tmpchar);
                } else {
                    the_prog = NOTHING;
                }
            } else {
                if (the_prog == AMBIGUOUS)
                    the_prog = NOTHING;
            }
            if (the_prog != AMBIGUOUS) {
                if (the_prog < 0 || the_prog >= db_top) {
                    the_prog = NOTHING;
                } else if (Typeof(the_prog) != TYPE_PROGRAM) {
                    the_prog = NOTHING;
                } else if (OWNER(the_prog) != OWNER(player) &&
                        !(FLAGS(the_prog) & LINK_OK)) {
                    the_prog = NOTHING;
                } else if (MLevel(the_prog) < mlev) {
                    the_prog = NOTHING;
                } else if (MLevel(OWNER(the_prog)) < mlev) {
                    the_prog = NOTHING;
                } else if (the_prog == xclude) {
                    the_prog = NOTHING;
                }
            }
            if (the_prog == AMBIGUOUS) {
                if (mpi_p) {
                    add_mpi_event(1, player, where, trigger, tmpchar+1,
                            (mt? "Listen" : "Olisten"), toparg, 1, (mt == 0));
                }
            } else if (the_prog != NOTHING) {
                add_muf_queue_event(player, where, trigger, the_prog, toparg,
                                    "(_Listen)", 1);
            }
        }
    }
    strcpy(buf, propname);
    if (is_propdir(what, buf)) {
        strcat(buf, "/");
        while ((pname = next_prop_name(what, exbuf, buf))) {
            strcpy(buf, pname);
            listenqueue(player, where, trigger, what, xclude, buf,
                        toparg, mlev, mt, mpi_p);
        }
    }
}


