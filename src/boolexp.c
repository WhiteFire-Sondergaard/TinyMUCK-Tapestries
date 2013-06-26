/* $Header: /home/foxen/CR/FB/src/RCS/boolexp.c,v 1.1 1996/06/12 02:12:35 foxen Exp $ */

/*
 * $Log: boolexp.c,v $
 * Revision 1.1  1996/06/12 02:12:35  foxen
 * Initial revision
 *
 * Revision 5.5  1994/03/14  12:20:58  foxen
 * Fb5.20 release checkpoint.
 *
 * Revision 5.4  1994/03/14  12:08:46  foxen
 * Initial portability mods and bugfixes.
 *
 * Revision 5.3  1994/02/11  01:52:17  foxen
 * Changed property struct to contain name, instead of pointer to seperately
 * allocated propname.
 *
 * Revision 5.2  1994/01/18  20:52:20  foxen
 * Version 5.15 release.
 *
 * Revision 5.1  1993/12/17  00:07:33  foxen
 * initial revision.
 *
 * Revision 1.1  91/02/12  12:05:55  12:05:55  tygryss (Karen Rivers)
 * Initial revision
 *
 * Revision 1.8  90/09/16  04:40:58  rearl
 * Preparation code added for disk-based MUCK.
 *
 * Revision 1.7  90/09/10  02:19:08  rearl
 * Introduced string compression of properties, for the
 * COMPRESS compiler option.
 *
 * Revision 1.6  90/08/27  03:19:28  rearl
 * Added match_here to key match routines.
 *
 * Revision 1.5  90/08/11  03:49:51  rearl
 * *** empty log message ***
 *
 * Revision 1.4  90/08/05  03:18:56  rearl
 * Redid matching routines.
 *
 * Revision 1.3  90/08/02  18:49:20  rearl
 * Fixed some calls to logging functions.
 *
 * Revision 1.2  90/07/29  17:30:37  rearl
 * Added support for ROOM locks to programs.
 *
 * Revision 1.1  90/07/19  23:01:56  casie
 * Initial revision
 *
 *
 */

#include "copyright.h"
#include "config.h"

#include <ctype.h>
#include <stdio.h>

#include "strings.h"
#include "db.h"
#include "props.h"
#include "match.h"
#include "externs.h"
#include "params.h"
#include "tune.h"
#include "interface.h"
#include "balloc.h"

/* Lachesis note on the routines in this package:
 *   eval_booexp does just evaluation.
 *
 *   parse_boolexp makes potentially recursive calls to several different
 *   subroutines ---
 *          parse_boolexp_F
 *            This routine does the leaf level parsing and the NOT.
 *        parse_boolexp_E
 *            This routine does the ORs.
 *        pasre_boolexp_T
 *            This routine does the ANDs.
 *
 *   Because property expressions are leaf level expressions, I have only
 *   touched eval_boolexp_F, asking it to call my additional parse_boolprop()
 *   routine.
 */


memblk *bool_mem = NULL;

struct boolexp *
alloc_boolnode()
{
#ifdef USE_BALLOC_FIXED
    return ((struct boolexp *)
	    balloc_fixed(&bool_mem, 400, sizeof(struct boolexp)));
#else
    return ((struct boolexp *) malloc(sizeof(struct boolexp)));
#endif
}


void
free_boolnode(struct boolexp *ptr)
{
#ifdef USE_BALLOC_FIXED
    bfree(ptr);
#else
    free(ptr);
#endif
}


struct boolexp *
copy_bool(struct boolexp * old)
{
    struct boolexp *o;

    if (old == TRUE_BOOLEXP)
	return TRUE_BOOLEXP;

    o = alloc_boolnode();

    if (!o) return 0;

    o->type = old->type;

    switch (old->type) {
	case BOOLEXP_AND:
	case BOOLEXP_OR:
	    o->sub1 = copy_bool(old->sub1);
	    o->sub2 = copy_bool(old->sub2);
	    break;
	case BOOLEXP_NOT:
	    o->sub1 = copy_bool(old->sub1);
	    break;
	case BOOLEXP_CONST:
	    o->thing = old->thing;
	    break;
	case BOOLEXP_PROP:
	    if (!old->prop_check) {
		free_boolnode(o);
		return 0;
	    }
	    o->prop_check = alloc_propnode(PropName(old->prop_check));
	    SetPFlagsRaw(o->prop_check, PropFlagsRaw(old->prop_check));
	    switch (PropType(old->prop_check)) {
		case PROP_STRTYP:
		    SetPDataStr(o->prop_check,
			    alloc_string(PropDataStr(old->prop_check)));
		    break;
		default:
		    SetPDataVal(o->prop_check, PropDataVal(old->prop_check));
		    break;
	    }
	    break;
	default:
	    log_status((char *)"PANIC: copy_boolexp: Error in boolexp!\n");
	    abort();
    }
    return o;
}


int 
eval_boolexp_rec(dbref player, struct boolexp * b, dbref thing)
{
    if (b == TRUE_BOOLEXP) {
	return 1;
    } else {
	switch (b->type) {
	    case BOOLEXP_AND:
		return (eval_boolexp_rec(player, b->sub1, thing)
			&& eval_boolexp_rec(player, b->sub2, thing));
	    case BOOLEXP_OR:
		return (eval_boolexp_rec(player, b->sub1, thing)
			|| eval_boolexp_rec(player, b->sub2, thing));
	    case BOOLEXP_NOT:
		return !eval_boolexp_rec(player, b->sub1, thing);
	    case BOOLEXP_CONST:
#ifndef SANITY
		if (b->thing == NOTHING) return 0;
		if (Typeof(b->thing) == TYPE_PROGRAM) {
		    if (Typeof(player) == TYPE_PLAYER ||
			    Typeof(player) == TYPE_THING) {
            // struct inst *rv;
            // rv = create_and_run_interp_frame(player, DBFETCH(player)->location,
            //     b->thing, thing, PREEMPT, STD_HARDUID, 0);
            // return (rv != NULL);
            std::tr1::shared_ptr<InterpreterReturnValue> rv =
            Interpreter::create_and_run_interp(player, DBFETCH(player)->location,
                b->thing, thing, PREEMPT, STD_HARDUID, 0, NULL, NULL);
            return rv->Bool();
		    }
		}
		return (b->thing == player || b->thing == OWNER(player)
			|| member(b->thing, DBFETCH(player)->contents)
			|| b->thing == DBFETCH(player)->location);
#else /* !SANITY */
		return 0;
#endif /* !SANITY */
	    case BOOLEXP_PROP:
		if (PropType(b->prop_check) == PROP_STRTYP) {
		    if (has_property_strict(player, thing,
			    PropName(b->prop_check),
			    PropDataStr(b->prop_check), 0))
			return 1;
		    if (has_property(player, player,
			    PropName(b->prop_check),
			    PropDataStr(b->prop_check), 0))
			return 1;
		}
		return 0;
	    default:
		abort();	/* bad type */
	}
    }
}


#ifndef SANITY
int 
eval_boolexp(dbref player, struct boolexp *b, dbref thing)
{
    int result;

    b = copy_bool(b);
    result = eval_boolexp_rec(player, b, thing);
    free_boolexp(b);
    return (result);
}
#endif


/* If the parser returns TRUE_BOOLEXP, you lose */
/* TRUE_BOOLEXP cannot be typed in by the user; use @unlock instead */
static const char *parsebuf;
static dbref parse_player;
static int parse_dbload;

static void 
skip_whitespace(void)
{
    while (*parsebuf && isspace(*parsebuf))
	parsebuf++;
}

static struct boolexp *parse_boolexp_E(void);	/* defined below */
static struct boolexp *parse_boolprop(char *buf);	/* defined below */

/* F -> (E); F -> !F; F -> object identifier */
static struct boolexp *
parse_boolexp_F(void)
{
    struct boolexp *b;
    char   *p;
    struct match_data md;
    char    buf[BUFFER_LEN];
    char    msg[BUFFER_LEN];

    skip_whitespace();
    switch (*parsebuf) {
	case '(':
	    parsebuf++;
	    b = parse_boolexp_E();
	    skip_whitespace();
	    if (b == TRUE_BOOLEXP || *parsebuf++ != ')') {
		free_boolexp(b);
		return TRUE_BOOLEXP;
	    } else {
		return b;
	    }
	    /* break; */
	case NOT_TOKEN:
	    parsebuf++;
	    b = alloc_boolnode();
	    b->type = BOOLEXP_NOT;
	    b->sub1 = parse_boolexp_F();
	    if (b->sub1 == TRUE_BOOLEXP) {
		free_boolnode(b);
		return TRUE_BOOLEXP;
	    } else {
		return b;
	    }
	    /* break */
	default:
	    /* must have hit an object ref */
	    /* load the name into our buffer */
	    p = buf;
	    while (*parsebuf
		    && *parsebuf != AND_TOKEN
		    && *parsebuf != OR_TOKEN
		    && *parsebuf != ')') {
		*p++ = *parsebuf++;
	    }
	    /* strip trailing whitespace */
	    *p-- = '\0';
	    while (isspace(*p))
		*p-- = '\0';

	    /* check to see if this is a property expression */
	    if (index(buf, PROP_DELIMITER)) {
		return parse_boolprop(buf);
	    }
	    b = alloc_boolnode();
	    b->type = BOOLEXP_CONST;

	    /* do the match */
	    if (!parse_dbload) {
#ifndef SANITY
		init_match(parse_player, buf, TYPE_THING, &md);
		match_neighbor(&md);
		match_possession(&md);
		match_me(&md);
		match_here(&md);
		match_absolute(&md);
		match_registered(&md);
		match_player(&md);
		b->thing = match_result(&md);

		if (b->thing == NOTHING) {
		    sprintf(msg, "I don't see %s here.", buf);
		    notify(parse_player, msg);
		    free_boolnode(b);
		    return TRUE_BOOLEXP;
		} else if (b->thing == AMBIGUOUS) {
		    sprintf(msg, "I don't know which %s you mean!", buf);
		    notify(parse_player, msg);
		    free_boolnode(b);
		    return TRUE_BOOLEXP;
		} else {
		    return b;
		}
#endif
	    } else {
		if (*buf != '#' || !number(buf+1)) {
		    free_boolnode(b);
		    return TRUE_BOOLEXP;
		}
		b->thing = (dbref)atoi(buf+1);
		if (b->thing < 0 || b->thing >= db_top ||
			Typeof(b->thing) == TYPE_GARBAGE) {
		    free_boolnode(b);
		    return TRUE_BOOLEXP;
		} else {
		    return b;
		}
	    }
	    /* break */
    }
}

/* T -> F; T -> F & T */
static struct boolexp *
parse_boolexp_T(void)
{
    struct boolexp *b;
    struct boolexp *b2;

    if ((b = parse_boolexp_F()) == TRUE_BOOLEXP) {
	return b;
    } else {
	skip_whitespace();
	if (*parsebuf == AND_TOKEN) {
	    parsebuf++;

	    b2 = alloc_boolnode();
	    b2->type = BOOLEXP_AND;
	    b2->sub1 = b;
	    if ((b2->sub2 = parse_boolexp_T()) == TRUE_BOOLEXP) {
		free_boolexp(b2);
		return TRUE_BOOLEXP;
	    } else {
		return b2;
	    }
	} else {
	    return b;
	}
    }
}

/* E -> T; E -> T | E */
static struct boolexp *
parse_boolexp_E(void)
{
    struct boolexp *b;
    struct boolexp *b2;

    if ((b = parse_boolexp_T()) == TRUE_BOOLEXP) {
	return b;
    } else {
	skip_whitespace();
	if (*parsebuf == OR_TOKEN) {
	    parsebuf++;

	    b2 = alloc_boolnode();
	    b2->type = BOOLEXP_OR;
	    b2->sub1 = b;
	    if ((b2->sub2 = parse_boolexp_E()) == TRUE_BOOLEXP) {
		free_boolexp(b2);
		return TRUE_BOOLEXP;
	    } else {
		return b2;
	    }
	} else {
	    return b;
	}
    }
}

struct boolexp *
parse_boolexp(dbref player, const char *buf, int dbloadp)
{
    parsebuf = buf;
    parse_player = player;
    parse_dbload = dbloadp;
    return parse_boolexp_E();
}

/* parse a property expression
   If this gets changed, please also remember to modify set.c       */
static struct boolexp *
parse_boolprop(char *buf)
{
    char   *type = alloc_string(buf);
    char   *d_class = (char *) index(type, PROP_DELIMITER);
    char   *x;
    struct boolexp *b;
    PropPtr p;
    char   *temp;

    x = type;
    b = alloc_boolnode();
    b->type = BOOLEXP_PROP;
    b->sub1 = b->sub2 = 0;
    b->thing = NOTHING;
    while (isspace(*type)) type++;
    if (*type == PROP_DELIMITER) {
	/* Oops!  Clean up and return a TRUE */
	free((void *) x);
	free_boolnode(b);
	return TRUE_BOOLEXP;
    }
    /* get rid of trailing spaces */
    for (temp = d_class - 1; isspace(*temp); temp--);
    temp++;
    *temp = '\0';
    d_class++;
    while (isspace(*d_class) && *d_class)
	d_class++;
    if (!*d_class) {
	/* Oops!  CLEAN UP AND RETURN A TRUE */
	free((void *) x);
	free_boolnode(b);
	return TRUE_BOOLEXP;
    }
    /* get rid of trailing spaces */
    for (temp = d_class; !isspace(*temp) && *temp; temp++);
    *temp = '\0';

    b->prop_check = p = alloc_propnode(type);
    SetPDataStr(p, alloc_string(d_class));
    SetPType(p, PROP_STRTYP);
    free((void *) x);
    return b;
}


long 
size_boolexp(struct boolexp * b)
{
    long result = 0L;

    if (b == TRUE_BOOLEXP) {
	return 0L;
    } else {
	result = sizeof(*b);
	switch (b->type) {
	    case BOOLEXP_AND:
	    case BOOLEXP_OR:
		result += size_boolexp(b->sub2);
	    case BOOLEXP_NOT:
		result += size_boolexp(b->sub1);
	    case BOOLEXP_CONST:
		break;
	    case BOOLEXP_PROP:
		result += sizeof(*b->prop_check);
		result += strlen(PropName(b->prop_check)) + 1;
		if (PropDataStr(b->prop_check))
		    result += strlen(PropDataStr(b->prop_check)) + 1;
		break;
	    default:
		abort();	/* bad type */
	}
	return (result);
    }
}


struct boolexp *
negate_boolexp(struct boolexp * b)
{
    struct boolexp *n;

    /* Obscure fact: !NOTHING == NOTHING in old-format databases! */
    if (b == TRUE_BOOLEXP)
	return TRUE_BOOLEXP;

    n = alloc_boolnode();
    n->type = BOOLEXP_NOT;
    n->sub1 = b;

    return n;
}


static struct boolexp *
getboolexp1(FILE * f)
{
    struct boolexp *b;
    PropPtr p;
    char    buf[BUFFER_LEN];	/* holds string for reading in property */
    int     c;
    int     i;			/* index into buf */

    c = getc(f);
    switch (c) {
	case '\n':
	    ungetc(c, f);
	    return TRUE_BOOLEXP;
	    /* break; */
	case EOF:
	    abort();		/* unexpected EOF in boolexp */
	    break;
	case '(':
	    b = alloc_boolnode();
	    if ((c = getc(f)) == '!') {
		b->type = BOOLEXP_NOT;
		b->sub1 = getboolexp1(f);
		if (getc(f) != ')')
		    goto error;
		return b;
	    } else {
		ungetc(c, f);
		b->sub1 = getboolexp1(f);
		switch (c = getc(f)) {
		    case AND_TOKEN:
			b->type = BOOLEXP_AND;
			break;
		    case OR_TOKEN:
			b->type = BOOLEXP_OR;
			break;
		    default:
			goto error;
			/* break */
		}
		b->sub2 = getboolexp1(f);
		if (getc(f) != ')')
		    goto error;
		return b;
	    }
	    /* break; */
	case '-':
	    /* obsolete NOTHING key */
	    /* eat it */
	    while ((c = getc(f)) != '\n')
		if (c == EOF)
		    abort();	/* unexp EOF */
	    ungetc(c, f);
	    return TRUE_BOOLEXP;
	    /* break */
	case '[':
	    /* property type */
	    b = alloc_boolnode();
	    b->type = BOOLEXP_PROP;
	    b->sub1 = b->sub2 = 0;
	    i = 0;
	    while ((c = getc(f)) != PROP_DELIMITER && i < BUFFER_LEN) {
		buf[i] = c;
		i++;
	    }
	    if (i >= BUFFER_LEN && c != PROP_DELIMITER)
		goto error;
	    buf[i] = '\0';

	    p = b->prop_check = alloc_propnode(buf);

	    i = 0;
	    while ((c = getc(f)) != ']') {
		if (c == '\\') c = getc(f);
		buf[i] = c;
		i++;
	    }
	    buf[i] = '\0';
	    if (i >= BUFFER_LEN && c != ']')
		goto error;
	    if (!number(buf)) {
		SetPDataStr(p, alloc_string(buf));
		SetPType(p, PROP_STRTYP);
	    } else {
		SetPDataVal(p, atol(buf));
		SetPType(p, PROP_INTTYP);
	    }
	    return b;
	default:
	    /* better be a dbref */
	    ungetc(c, f);
	    b = alloc_boolnode();
	    b->type = BOOLEXP_CONST;
	    b->thing = 0;

	    /* NOTE possibly non-portable code */
	    /* Will need to be changed if putref/getref change */
	    while (isdigit(c = getc(f))) {
		b->thing = b->thing * 10 + c - '0';
	    }
	    ungetc(c, f);
	    return b;
    }

error:
    abort();			/* bomb out */
}

struct boolexp *
getboolexp(FILE * f)
{
    struct boolexp *b;

    b = getboolexp1(f);
    if (getc(f) != '\n')
	abort();		/* parse error, we lose */
    return b;
}

void 
free_boolexp(struct boolexp * b)
{
    if (b != TRUE_BOOLEXP) {
	switch (b->type) {
	    case BOOLEXP_AND:
	    case BOOLEXP_OR:
		free_boolexp(b->sub1);
		free_boolexp(b->sub2);
		free_boolnode(b);
		break;
	    case BOOLEXP_NOT:
		free_boolexp(b->sub1);
		free_boolnode(b);
		break;
	    case BOOLEXP_CONST:
		free_boolnode(b);
		break;
	    case BOOLEXP_PROP:
		free_propnode(b->prop_check);
		free_boolnode(b);
		break;
	}
    }
}


