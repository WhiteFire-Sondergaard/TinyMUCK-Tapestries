/* Primitives package */

#include "copyright.h"
#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include "db.h"
#include "tune.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "strings.h"
#include "interp.h"

static struct inst *oper1, *oper2, *oper3, *oper4;
static struct inst temp1, temp2 /*, temp3 */;
static int tmp, result;
static dbref ref;
static char buf[BUFFER_LEN];
static char *pname;


void 
prim_numberp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING || !oper1->data.string)
	result = 0;
    else
	result = number(oper1->data.string->data);
    CLEAR(oper1);
    PushInt(result);
}

void 
prim_stringcmp(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING || oper2->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (oper1->data.string == oper2->data.string)
	result = 0;
    else if (!(oper2->data.string && oper1->data.string))
	result = oper1->data.string ? -1 : 1;
    else {
#if defined(ANONYMITY)
	char	pad[16384];

	strcpy(pad, unmangle(player, oper2->data.string->data));
	result = string_compare(pad, unmangle(player, oper1->data.string->data));
#else
	result = string_compare(oper2->data.string->data, oper1->data.string->data);
#endif
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void 
prim_strcmp(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING || oper2->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (oper1->data.string == oper2->data.string)
	result = 0;
    else if (!(oper2->data.string && oper1->data.string))
	result = oper1->data.string ? -1 : 1;
    else {
#if defined(ANONYMITY)
	char	pad[16384];

	strcpy(pad, unmangle(player, oper2->data.string->data));
	result = strcmp(pad, unmangle(player, oper1->data.string->data));
#else
	result = strcmp(oper2->data.string->data, oper1->data.string->data);
#endif
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void 
prim_strncmp(PRIM_PROTOTYPE)
{
    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Non-integer argument.");
    if (oper2->type != PROG_STRING || oper3->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (oper2->data.string == oper3->data.string)
	result = 0;
    else if (!(oper3->data.string && oper2->data.string))
	result = oper2->data.string ? -1 : 1;
    else
	result = strncmp(oper3->data.string->data, oper2->data.string->data,
			 oper1->data.number);
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushInt(result);
}

void 
prim_strcut(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    temp1 = *(oper1 = POP());
    temp2 = *(oper2 = POP());
    if (temp1.type != PROG_INTEGER)
	abort_interp("Non-integer argument (2)");
    if (temp1.data.number < 0)
	abort_interp("Argument must be a positive integer.");
    if (temp2.type != PROG_STRING)
	abort_interp("Non-string argument (1)");
    if (!temp2.data.string) {
	PushNullStr;
	PushNullStr;
    } else {
	if (temp1.data.number > temp2.data.string->length) {
	    temp2.data.string->links++;
	    PushStrRaw(temp2.data.string);
	    PushNullStr;
	} else {
	    bcopy(temp2.data.string->data, buf, temp1.data.number);
	    buf[temp1.data.number] = '\0';
	    PushString(buf);
	    if (temp2.data.string->length > temp1.data.number) {
		bcopy(temp2.data.string->data + temp1.data.number, buf,
		      temp2.data.string->length - temp1.data.number + 1);
		PushString(buf);
	    } else {
		PushNullStr;
	    }
	}
    }
    CLEAR(&temp1);
    CLEAR(&temp2);
}

void 
prim_strlen(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (!oper1->data.string)
	result = 0;
    else
#if defined(ANONYMITY)
	result = strlen(unmangle(player, oper1->data.string->data));
#else
	result = oper1->data.string->length;
#endif
    CLEAR(oper1);
    PushInt(result);
}

void 
prim_strcat(PRIM_PROTOTYPE)
{
    struct shared_string *string;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING || oper2->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (!oper1->data.string && !oper2->data.string)
	string = NULL;
    else if (!oper2->data.string) {
	oper1->data.string->links++;
	string = oper1->data.string;
    } else if (!oper1->data.string) {
	oper2->data.string->links++;
	string = oper2->data.string;
    } else if (oper1->data.string->length + oper2->data.string->length
	       > (BUFFER_LEN) - 1) {
	abort_interp("Operation would result in overflow.");
    } else {
	bcopy(oper2->data.string->data, buf, oper2->data.string->length);
	bcopy(oper1->data.string->data, buf + oper2->data.string->length,
	      oper1->data.string->length + 1);
	string = alloc_prog_string(buf);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushStrRaw(string);
}

void 
prim_atoi(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING || !oper1->data.string)
	result = 0;
    else
	result = atoi(oper1->data.string->data);
    CLEAR(oper1);
    PushInt(result);
}

void 
prim_notify(PRIM_PROTOTYPE)
{
    struct inst *oper1, *oper2;
    char buf2[BUFFER_LEN*2];
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!valid_object(oper2))
	abort_interp("Invalid object argument (1)");
    CHECKREMOTE(oper2->data.objref);

    if (oper1->data.string) {
	strcpy(buf, oper1->data.string->data);
	if (tp_force_mlev1_name_notify &&
		mlev < 2  && player != oper2->data.objref) {
	    strcpy(buf2, PNAME(player));
	    strcat(buf2, " ");
	    if (!string_prefix(buf, buf2)) {
		strcat(buf2, buf);
		buf2[BUFFER_LEN-1] = '\0';
		strcpy(buf, buf2);
	    }
	}
	notify_listeners(player, program, oper2->data.objref,
			 getloc(oper2->data.objref), buf, 1);
    }
    CLEAR(oper1);
    CLEAR(oper2);
}


void 
prim_notify_exclude(PRIM_PROTOTYPE)
{
    /* roomD excludeDn ... excludeD1 nI messageS  -- */
    struct inst *oper1, *oper2;
    char buf2[BUFFER_LEN*2];
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper2->type != PROG_INTEGER)
	abort_interp("non-integer count argument (top-1)");
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string message argument (top)");
    strcpy(buf, DoNullInd(oper1->data.string));

    if (tp_force_mlev1_name_notify && mlev < 2) {
	strcpy(buf2, PNAME(player));
	strcat(buf2, " ");
	if (!string_prefix(buf, buf2)) {
	    strcat(buf2, buf);
	    buf2[BUFFER_LEN-1] = '\0';
	    strcpy(buf, buf2);
	}
    }

    result = oper2->data.number;
    CLEAR(oper1);
    CLEAR(oper2);
    {
	dbref   what, where, excluded[STACK_SIZE];
	int     count, i;

	CHECKOP(0);
	count = i = result;
	if (i >= STACK_SIZE || i < 0)
	    abort_interp("Count argument is out of range.");
	while (i > 0) {
	    CHECKOP(1);
	    oper1 = POP();
	    if (oper1->type != PROG_OBJECT)
		abort_interp("Invalid object argument.");
	    excluded[--i] = oper1->data.objref;
	    CLEAR(oper1);
	}
	CHECKOP(1);
	oper1 = POP();
	if (!valid_object(oper1))
	    abort_interp("Non-object argument (1)");
	where = oper1->data.objref;
	if (Typeof(where) != TYPE_ROOM && Typeof(where) != TYPE_THING &&
		Typeof(where) != TYPE_PLAYER)
	    abort_interp("Invalid location argument (1)");
	CHECKREMOTE(where);
	what = DBFETCH(where)->contents;
	CLEAR(oper1);
	if (*buf) {
	    while (what != NOTHING) {
		if (Typeof(what) != TYPE_ROOM) {
		    for (tmp = 0, i = count; i-- > 0;) {
			if (excluded[i] == what)
			    tmp = 1;
		    }
		} else {
		    tmp = 1;
		}
		if (!tmp)
		    notify_listeners(player, program, what, where, buf, 0);
		what = DBFETCH(what)->next;
	    }
	}

	if (tp_listeners) {
	    notify_listeners(player, program, where, where, buf, 0);
	    if (tp_listeners_env) {
		what = DBFETCH(where)->location;
		for (; what != NOTHING; what = DBFETCH(what)->location)
		    notify_listeners(player, program, what, where, buf, 0);
	    }
	}
    }
}

void 
prim_intostr(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type == PROG_STRING)
	abort_interp("Invalid argument.");
    sprintf(buf, "%d", oper1->data.number);
    CLEAR(oper1);
    PushString(buf);
}

void 
prim_explode(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    temp1 = *(oper1 = POP());
    temp2 = *(oper2 = POP());
    oper1 = &temp1;
    oper2 = &temp2;
    if (temp1.type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (temp2.type != PROG_STRING)
	abort_interp("Non-string argument (1)");
    if (!temp1.data.string)
	abort_interp("Empty string argument (2)");
    {
	int     i;
	const char *delimit = temp1.data.string->data;

	if (!temp2.data.string) {
	    result = 1;
	    CLEAR(&temp1);
	    CLEAR(&temp2);
	    PushNullStr;
	    PushInt(result);
	    return;
	} else {
	    result = 0;
	    bcopy(temp2.data.string->data, buf, temp2.data.string->length + 1);
	    for (i = temp2.data.string->length - 1; i >= 0; i--) {
		if (!strncmp(buf + i, delimit, temp1.data.string->length)) {
		    buf[i] = '\0';
		    CHECKOFLOW(1);
		    PushString((buf + i + temp1.data.string->length));
		    result++;
		}
	    }
	    CHECKOFLOW(1);
	    PushString(buf);
	    result++;
	}
    }
    CHECKOFLOW(1);
    CLEAR(&temp1);
    CLEAR(&temp2);
    PushInt(result);
}

void 
prim_subst(PRIM_PROTOTYPE)
{
    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (3)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (oper3->type != PROG_STRING)
	abort_interp("Non-string argument (1)");
    if (!oper1->data.string)
	abort_interp("Empty string argument (3)");
    {
	int     i = 0, j = 0, k = 0;
	const char *match;
	const char *replacement;
	char    xbuf[BUFFER_LEN];

	buf[0] = '\0';
	if (oper3->data.string) {
	    bcopy(oper3->data.string->data, xbuf, oper3->data.string->length + 1);
	    match = oper1->data.string->data;
	    replacement = DoNullInd(oper2->data.string);
	    k = *replacement ? oper2->data.string->length : 0;
	    while (xbuf[i]) {
		if (!strncmp(xbuf + i, match, oper1->data.string->length)) {
		    if ((j + k + 1) > BUFFER_LEN)
			abort_interp("Operation would result in overflow.");
		    strcat(buf, replacement);
		    i += oper1->data.string->length;
		    j += k;
		} else {
		    if ((j + 1) > BUFFER_LEN)
			abort_interp("Operation would result in overflow.");
		    buf[j++] = xbuf[i++];
		    buf[j] = '\0';
		}
	    }
	}
    }
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushString(buf);
}

void 
prim_instr(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument type (2)");
    if (!(oper1->data.string))
	abort_interp("Empty string argument (2)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (1)");
    if (!oper2->data.string) {
	result = 0;
    } else {

	const char *remaining = oper2->data.string->data;
	const char *match = oper1->data.string->data;
	int     step = 1;

	result = 0;
	do {
	    if (!strncmp(remaining, match, oper1->data.string->length)) {
		result = remaining - oper2->data.string->data + 1;
		break;
	    }
	    remaining += step;
	} while (remaining >= oper2->data.string->data && *remaining);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void 
prim_rinstr(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument type (2)");
    if (!(oper1->data.string))
	abort_interp("Empty string argument (2)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (1)");
    if (!oper2->data.string) {
	result = 0;
    } else {

	const char *remaining = oper2->data.string->data;
	const char *match = oper1->data.string->data;
	int     step = -1;

	remaining += oper2->data.string->length - 1;

	result = 0;
	do {
	    if (!strncmp(remaining, match, oper1->data.string->length)) {
		result = remaining - oper2->data.string->data + 1;
		break;
	    }
	    remaining += step;
	} while (remaining >= oper2->data.string->data && *remaining);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void 
prim_pronoun_sub(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();		/* oper1 is a string, oper2 a dbref */
    if (!valid_object(oper2))
	abort_interp("Invalid argument (1)");
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument (2)");
    if (oper1->data.string) {
	strcpy(buf, pronoun_substitute(oper2->data.objref,
				       oper1->data.string->data));
    } else {
	buf[0] = '\0';
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushString(buf);
}

void 
prim_toupper(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (oper1->data.string) {
	strcpy(buf, oper1->data.string->data);
    } else {
	buf[0] = '\0';
    }
    for (ref = 0; buf[ref]; ref++)
	buf[ref] = UPCASE(buf[ref]);
    CLEAR(oper1);
    PushString(buf);
}

void 
prim_tolower(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (oper1->data.string) {
	strcpy(buf, oper1->data.string->data);
    } else {
	buf[0] = '\0';
    }
    for (ref = 0; buf[ref]; ref++)
	buf[ref] = tolower(buf[ref]);
    CLEAR(oper1);
    PushString(buf);
}

void 
prim_unparseobj(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_OBJECT)
	abort_interp("Non-object argument.");
    {
	result = oper1->data.objref;
	switch (result) {
	    case NOTHING:
		sprintf(buf, "*NOTHING*");
		break;
	    case HOME:
		sprintf(buf, "*HOME*");
		break;
	    default:
		if (result < 0 || result > db_top)
		    sprintf(buf, "*INVALID*");
		else
		    sprintf(buf, "%s(#%d%s)", RNAME(result), result,
			    unparse_flags(result));
	}
	CLEAR(oper1);
	PushString(buf);
    }
}

void 
prim_smatch(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING || oper2->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (!oper1->data.string || !oper2->data.string)
	abort_interp("Null string argument.");
    {
	char    xbuf[BUFFER_LEN];

#if defined(ANONYMITY)
	strcpy(buf, unmangle(player, oper1->data.string->data));
	strcpy(xbuf, unmangle(player, oper2->data.string->data));
#else
	strcpy(buf, oper1->data.string->data);
	strcpy(xbuf, oper2->data.string->data);
#endif
	CLEAR(oper1);
	CLEAR(oper2);
	result = equalstr(buf, xbuf);
	PushInt(result);
    }
}

void 
prim_striplead(PRIM_PROTOTYPE)
{				/* string -- string' */
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Not a string argument.");
    strcpy(buf, DoNullInd(oper1->data.string));
    for (pname = buf; *pname && isspace(*pname); pname++);
    CLEAR(oper1);
    PushString(pname);
}

void 
prim_striptail(PRIM_PROTOTYPE)
{				/* string -- string' */
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Not a string argument.");
    strcpy(buf, DoNullInd(oper1->data.string));
    result = strlen(buf);
    while ((result-- > 0) && isspace(buf[result]))
	buf[result] = '\0';
    CLEAR(oper1);
    PushString(buf);
}

void 
prim_stringpfx(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING || oper2->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (oper1->data.string == oper2->data.string)
	result = 0;
    else if (!(oper2->data.string && oper1->data.string))
	result = oper1->data.string ? -1 : 1;
    else {
	result = string_prefix(oper2->data.string->data, oper1->data.string->data);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}


void 
prim_strencrypt(PRIM_PROTOTYPE)
{
    const char *ptr;

    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();
    if (oper1->type != PROG_STRING || oper2->type != PROG_STRING) {
	abort_interp("Non-string argument.");
    }
    if (!oper2->data.string || !*(oper2->data.string->data)) {
	abort_interp("Key cannot be a null string. (2)");
    }
#if defined(ANONYMITY)
    {
	char	pad[BUFFER_LEN * 2];
	char	pad2[BUFFER_LEN * 2];

	strcpy(pad, unmangle(player, DoNullInd(oper1->data.string)));
	strcpy(pad2, unmangle(player, DoNullInd(oper2->data.string)));
	ptr = strencrypt(pad, pad2);
    }
#else
    ptr = strencrypt(DoNullInd(oper1->data.string), oper2->data.string->data);
#endif
    CLEAR(oper1);
    CLEAR(oper2);
    PushString(ptr);
}


void 
prim_strdecrypt(PRIM_PROTOTYPE)
{
    const char *ptr;

    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();
    if (oper1->type != PROG_STRING || oper2->type != PROG_STRING) {
	abort_interp("Non-string argument.");
    }
    if (!oper2->data.string || !*(oper2->data.string->data)) {
	abort_interp("Key cannot be a null string. (2)");
    }
#if defined(ANONYMITY)
    {
	char	pad[BUFFER_LEN * 2];
	char	pad2[BUFFER_LEN * 2];

	strcpy(pad, unmangle(player, DoNullInd(oper1->data.string)));
	strcpy(pad2, unmangle(player, DoNullInd(oper2->data.string)));
	ptr = strdecrypt(pad, pad2);
    }
#else
    ptr = strdecrypt(DoNullInd(oper1->data.string), oper2->data.string->data);
#endif
    CLEAR(oper1);
    CLEAR(oper2);
    PushString(ptr);
}

