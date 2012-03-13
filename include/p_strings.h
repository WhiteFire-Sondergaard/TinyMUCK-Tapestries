extern void prim_numberp(PRIM_PROTOTYPE);
extern void prim_stringcmp(PRIM_PROTOTYPE);
extern void prim_strcmp(PRIM_PROTOTYPE);
extern void prim_strncmp(PRIM_PROTOTYPE);
extern void prim_strcut (PRIM_PROTOTYPE);
extern void prim_strlen(PRIM_PROTOTYPE);
extern void prim_strcat(PRIM_PROTOTYPE);
extern void prim_atoi(PRIM_PROTOTYPE);
extern void prim_notify(PRIM_PROTOTYPE);
extern void prim_notify_exclude(PRIM_PROTOTYPE);
extern void prim_intostr(PRIM_PROTOTYPE);
extern void prim_explode(PRIM_PROTOTYPE);
extern void prim_subst(PRIM_PROTOTYPE);
extern void prim_instr(PRIM_PROTOTYPE);
extern void prim_rinstr(PRIM_PROTOTYPE);
extern void prim_pronoun_sub(PRIM_PROTOTYPE);
extern void prim_toupper(PRIM_PROTOTYPE);
extern void prim_tolower(PRIM_PROTOTYPE);
extern void prim_unparseobj(PRIM_PROTOTYPE);
extern void prim_smatch(PRIM_PROTOTYPE);
extern void prim_striplead(PRIM_PROTOTYPE);
extern void prim_striptail(PRIM_PROTOTYPE);
extern void prim_stringpfx(PRIM_PROTOTYPE);
extern void prim_strencrypt(PRIM_PROTOTYPE);
extern void prim_strdecrypt(PRIM_PROTOTYPE);

#define PRIMS_STRINGS_FUNCS prim_numberp, prim_stringcmp, prim_strcmp,        \
    prim_strncmp, prim_strcut, prim_strlen, prim_strcat, prim_atoi,           \
    prim_notify, prim_notify_exclude, prim_intostr,                           \
    prim_explode, prim_subst, prim_instr, prim_rinstr, prim_pronoun_sub,      \
    prim_toupper, prim_tolower, prim_unparseobj, prim_smatch, prim_striplead, \
    prim_striptail, prim_stringpfx, prim_strencrypt, prim_strdecrypt

#define PRIMS_STRINGS_NAMES "NUMBER?", "STRINGCMP", "STRCMP",  \
    "STRNCMP", "STRCUT", "STRLEN", "STRCAT", "ATOI",           \
    "NOTIFY", "NOTIFY_EXCLUDE", "INTOSTR",                     \
    "EXPLODE", "SUBST", "INSTR", "RINSTR", "PRONOUN_SUB",      \
    "TOUPPER", "TOLOWER", "UNPARSEOBJ", "SMATCH", "STRIPLEAD", \
    "STRIPTAIL", "STRINGPFX", "STRENCRYPT", "STRDECRYPT"

#define PRIMS_STRINGS_CNT 25

