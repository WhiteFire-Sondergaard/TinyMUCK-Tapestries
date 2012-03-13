extern void prim_pop (PRIM_PROTOTYPE);
extern void prim_dup (PRIM_PROTOTYPE);
extern void prim_at (PRIM_PROTOTYPE);
extern void prim_bang (PRIM_PROTOTYPE);
extern void prim_var (PRIM_PROTOTYPE);
extern void prim_localvar (PRIM_PROTOTYPE);
extern void prim_swap (PRIM_PROTOTYPE);
extern void prim_over (PRIM_PROTOTYPE);
extern void prim_pick (PRIM_PROTOTYPE);
extern void prim_put (PRIM_PROTOTYPE);
extern void prim_rot (PRIM_PROTOTYPE);
extern void prim_rotate (PRIM_PROTOTYPE);
extern void prim_dbtop (PRIM_PROTOTYPE);
extern void prim_depth (PRIM_PROTOTYPE);
extern void prim_version (PRIM_PROTOTYPE);
extern void prim_prog (PRIM_PROTOTYPE);
extern void prim_trig (PRIM_PROTOTYPE);
extern void prim_caller (PRIM_PROTOTYPE);
extern void prim_preempt (PRIM_PROTOTYPE);
extern void prim_foreground (PRIM_PROTOTYPE);
extern void prim_background (PRIM_PROTOTYPE);
extern void prim_intp(PRIM_PROTOTYPE);
extern void prim_stringp(PRIM_PROTOTYPE);
extern void prim_dbrefp(PRIM_PROTOTYPE);
extern void prim_addressp(PRIM_PROTOTYPE);
extern void prim_lockp(PRIM_PROTOTYPE);
extern void prim_checkargs(PRIM_PROTOTYPE);
extern void prim_mode(PRIM_PROTOTYPE);
extern void prim_setmode(PRIM_PROTOTYPE);
extern void prim_interp(PRIM_PROTOTYPE);

#define PRIMS_STACK_FUNCS prim_pop, prim_dup, prim_at, prim_bang, prim_var,  \
    prim_localvar, prim_swap, prim_over, prim_pick, prim_put, prim_rot,      \
    prim_rotate, prim_dbtop, prim_depth, prim_version, prim_prog, prim_trig, \
    prim_caller, prim_intp, prim_stringp, prim_dbrefp, prim_addressp,        \
    prim_lockp, prim_checkargs, prim_mode, prim_setmode, prim_interp

#define PRIMS_STACK_NAMES "POP", "DUP", "@", "!", "VARIABLE", \
    "LOCALVAR", "SWAP", "OVER", "PICK", "PUT", "ROT",         \
    "ROTATE", "DBTOP", "DEPTH", "VERSION", "PROG", "TRIG",    \
    "CALLER", "INT?", "STRING?", "DBREF?", "ADDRESS?",        \
    "LOCK?", "CHECKARGS", "MODE", "SETMODE", "INTERP"

#define PRIMS_STACK_CNT 27

