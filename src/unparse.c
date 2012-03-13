/* $Header: /home/foxen/CR/FB/src/RCS/unparse.c,v 1.1 1996/06/12 03:06:33 foxen Exp $ */

/*
 * $Log: unparse.c,v $
 * Revision 1.1  1996/06/12 03:06:33  foxen
 * Initial revision
 *
 * Revision 5.13  1994/03/14  12:20:58  foxen
 * Fb5.20 release checkpoint.
 *
 * Revision 5.12  1994/01/18  20:52:20  foxen
 * Version 5.15 release.
 *
 * Revision 5.11  1993/12/20  06:22:51  foxen
 * *** empty log message ***
 *
 * Revision 5.1  1993/12/17  00:07:33  foxen
 * initial revision.
 *
 * Revision 1.1  91/01/24  00:44:32  cks
 * changes for QUELL.
 *
 * Revision 1.0  91/01/22  21:06:47  cks
 * Initial revision
 *
 * Revision 1.6  90/09/18  08:02:47  rearl
 * Took out FILTER.
 *
 * Revision 1.5  90/09/16  04:43:11  rearl
 * Preparation code added for disk-based MUCK.
 *
 * Revision 1.4  90/08/27  03:34:48  rearl
 * Took out TEMPLE, other stuff.
 *
 * Revision 1.3  90/08/11  04:11:41  rearl
 * *** empty log message ***
 *
 * Revision 1.2  90/08/06  03:50:27  rearl
 * Added unparsing of ROOMS (exits? programs?) if they are CHOWN_OK.
 *
 * Revision 1.1  90/07/19  23:04:17  casie
 * Initial revision
 *
 *
 */

#include "copyright.h"
#include "config.h"

#include "db.h"
#include "externs.h"
#include "params.h"
#include "tune.h"
#include "interface.h"
#include "props.h"

const char *unparse_flags(dbref thing)
{
    static char buf[BUFFER_LEN];
    char   *p;
    const char *type_codes = "R-EPFG";

    p = buf;
    if (Typeof(thing) != TYPE_THING)
	*p++ = type_codes[Typeof(thing)];
    if (FLAGS(thing) & ~TYPE_MASK) {
	/* print flags */
	if (FLAGS(thing) & WIZARD)
	    *p++ = 'W';
	if (FLAGS(thing) & LINK_OK)
	    *p++ = 'L';

	if (FLAGS(thing) & KILL_OK)
	    *p++ = 'K';

	if (FLAGS(thing) & DARK)
	    *p++ = 'D';
	if (FLAGS(thing) & STICKY)
	    *p++ = 'S';
	if (FLAGS(thing) & QUELL)
	    *p++ = 'Q';
	if (FLAGS(thing) & BUILDER)
	    *p++ = 'B';
	if (FLAGS(thing) & CHOWN_OK)
	    *p++ = 'C';
	if (FLAGS(thing) & JUMP_OK)
	    *p++ = 'J';
	if (FLAGS(thing) & HAVEN)
	    *p++ = 'H';
	if (FLAGS(thing) & ABODE)
	    *p++ = 'A';
	if (FLAGS(thing) & VEHICLE)
	    *p++ = 'V';
	if (FLAGS(thing) & XFORCIBLE)
	    *p++ = 'X';
	if (FLAGS(thing) & ZOMBIE)
	    *p++ = 'Z';
	if (MLevRaw(thing)) {
	    *p++ = 'M';
	    switch (MLevRaw(thing)) {
		case 1:
		    *p++ = '1';
		    break;
		case 2:
		    *p++ = '2';
		    break;
		case 3:
		    *p++ = '3';
		    break;
	    }
	}
    }
    *p = '\0';
    return buf;
}

const char *
unparse_object(dbref player, dbref loc)
{
    static char buf[BUFFER_LEN];

    if (Typeof(player) != TYPE_PLAYER)
	player = OWNER(player);
    switch (loc) {
	case NOTHING:
	    return "*NOTHING*";
	case AMBIGUOUS:
	    return "*AMBIGUOUS*";
	case HOME:
	    return "*HOME*";
	default:
	    if (loc < 0 || loc > db_top)
#ifdef SANITY
	    {
	        sprintf(buf, "*INVALID*(#%d)", loc);
	        return buf;
	    }
#else
		return "*INVALID*";
#endif
#ifndef SANITY
	    if (!(FLAGS(player) & STICKY) &&
		    (can_link_to(player, NOTYPE, loc) ||
		     ((Typeof(loc) != TYPE_PLAYER) &&
		      (controls_link(player, loc) ||
		       (FLAGS(loc) & CHOWN_OK)))
		    )) {
		/* show everything */
#endif
		sprintf(buf, "%s(#%d%s)", PNAME(loc), loc, unparse_flags(loc));
		return buf;
#ifndef SANITY
	    } else {
		/* show only the name */
		return PNAME(loc);
	    }
#endif
    }
}

static char boolexp_buf[BUFFER_LEN];
static char *buftop;

static void 
unparse_boolexp1(dbref player, struct boolexp * b,
		 boolexp_type outer_type, int fullname)
{
    if ((buftop - boolexp_buf) > (BUFFER_LEN / 2))
	return;
    if (b == TRUE_BOOLEXP) {
	strcpy(buftop, "*UNLOCKED*");
	buftop += strlen(buftop);
    } else {
	switch (b->type) {
	    case BOOLEXP_AND:
		if (outer_type == BOOLEXP_NOT) {
		    *buftop++ = '(';
		}
		unparse_boolexp1(player, b->sub1, b->type, fullname);
		*buftop++ = AND_TOKEN;
		unparse_boolexp1(player, b->sub2, b->type, fullname);
		if (outer_type == BOOLEXP_NOT) {
		    *buftop++ = ')';
		}
		break;
	    case BOOLEXP_OR:
		if (outer_type == BOOLEXP_NOT || outer_type == BOOLEXP_AND) {
		    *buftop++ = '(';
		}
		unparse_boolexp1(player, b->sub1, b->type, fullname);
		*buftop++ = OR_TOKEN;
		unparse_boolexp1(player, b->sub2, b->type, fullname);
		if (outer_type == BOOLEXP_NOT || outer_type == BOOLEXP_AND) {
		    *buftop++ = ')';
		}
		break;
	    case BOOLEXP_NOT:
		*buftop++ = '!';
		unparse_boolexp1(player, b->sub1, b->type, fullname);
		break;
	    case BOOLEXP_CONST:
		if (fullname) {
#ifndef SANITY
		    strcpy(buftop, unparse_object(player, b->thing));
#endif
		} else {
		    sprintf(buftop, "#%d", b->thing);
		}
		buftop += strlen(buftop);
		break;
	    case BOOLEXP_PROP:
		strcpy(buftop, PropName(b->prop_check));
		strcat(buftop, ":");
		if (PropType(b->prop_check) == PROP_STRTYP)
		    strcat(buftop, PropDataStr(b->prop_check));
		buftop += strlen(buftop);
		break;
	    default:
		abort();	/* bad type */
		break;
	}
    }
}

const char *
unparse_boolexp(dbref player, struct boolexp * b, int fullname)
{
    buftop = boolexp_buf;
    unparse_boolexp1(player, b, BOOLEXP_CONST, fullname);  /* no outer type */
    *buftop++ = '\0';

    return boolexp_buf;
}
