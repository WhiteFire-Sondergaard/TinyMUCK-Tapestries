#include "copyright.h"

#define IN_JMP     1
#define IN_READ    2
#define IN_SLEEP   3
#define IN_CALL    4
#define IN_EXECUTE 5
#define IN_RET     6

#define BASE_MIN 1
#define BASE_MAX (6 + PRIMS_CONNECTS_CNT + PRIMS_DB_CNT + PRIMS_MATH_CNT + \
    PRIMS_MISC_CNT + PRIMS_PROPS_CNT + PRIMS_STACK_CNT + PRIMS_STRINGS_CNT)

/* now refer to tables to map instruction number to name */
extern const char *base_inst[];

extern char *insttotext(struct inst *, int, dbref);
/* and declare debug instruction diagnostic routine */
extern char *debug_inst(struct inst *, struct inst *, int, dbref);

