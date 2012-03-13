extern void prim_time(PRIM_PROTOTYPE);
extern void prim_date(PRIM_PROTOTYPE);
extern void prim_gmtoffset(PRIM_PROTOTYPE);
extern void prim_systime(PRIM_PROTOTYPE);
extern void prim_timesplit(PRIM_PROTOTYPE);
extern void prim_timefmt(PRIM_PROTOTYPE);
extern void prim_queue(PRIM_PROTOTYPE);
extern void prim_kill(PRIM_PROTOTYPE);
extern void prim_force(PRIM_PROTOTYPE);
extern void prim_timestamps(PRIM_PROTOTYPE);
extern void prim_fork(PRIM_PROTOTYPE);
extern void prim_pid(PRIM_PROTOTYPE);
extern void prim_stats(PRIM_PROTOTYPE);
extern void prim_abort(PRIM_PROTOTYPE);
extern void prim_ispidp(PRIM_PROTOTYPE);
extern void prim_parselock(PRIM_PROTOTYPE);
extern void prim_unparselock(PRIM_PROTOTYPE);
extern void prim_prettylock(PRIM_PROTOTYPE);
extern void prim_testlock(PRIM_PROTOTYPE);
extern void prim_sysparm(PRIM_PROTOTYPE);

#define PRIMS_MISC_FUNCS prim_time, prim_date, prim_gmtoffset,           \
    prim_systime, prim_timesplit, prim_timefmt, prim_queue, prim_kill,   \
    prim_force, prim_timestamps, prim_fork, prim_pid, prim_stats,        \
    prim_abort, prim_ispidp, prim_parselock, prim_unparselock,           \
    prim_prettylock, prim_testlock, prim_sysparm

#define PRIMS_MISC_NAMES "TIME", "DATE", "GMTOFFSET",    \
    "SYSTIME", "TIMESPLIT", "TIMEFMT", "QUEUE", "KILL",  \
    "FORCE", "TIMESTAMPS", "FORK", "PID", "STATS",       \
    "ABORT", "ISPID?", "PARSELOCK", "UNPARSELOCK",       \
    "PRETTYLOCK", "TESTLOCK", "SYSPARM"

#define PRIMS_MISC_CNT 20

