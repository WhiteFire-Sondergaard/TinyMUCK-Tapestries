/* $Header: /home/foxen/CR/FB/src/RCS/predicates.c,v 1.1 1996/06/12 02:47:32 foxen Exp $ */

/*
 * $Log: predicates.c,v $
 * Revision 1.1  1996/06/12 02:47:32  foxen
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
 * Revision 1.1  91/08/20  15:05:30  jearls
 * Initial revision
 *
 * Revision 1.2  91/03/24  01:19:30  lynx
 * added check for jump_ok or owned by owner of actions
 *
 * Revision 1.1  91/01/24  00:44:27  cks
 * changes for QUELL.
 *
 * Revision 1.0  91/01/22  20:54:34  cks
 * Initial revision
 *
 * Revision 1.8  90/09/18  08:01:39  rearl
 * Fixed a few broken things.
 *
 * Revision 1.7  90/09/16  04:42:42  rearl
 * Preparation code added for disk-based MUCK.
 *
 * Revision 1.6  90/09/10  02:22:30  rearl
 * Fixed some calls to pronoun_substitute.
 *
 * Revision 1.5  90/08/27  03:32:49  rearl
 * Major changes on several predicates, usage...
 *
 * Revision 1.4  90/08/15  03:07:34  rearl
 * Macroified some things.  Took out #ifdef GENDER.
 *
 * Revision 1.3  90/08/06  02:46:30  rearl
 * Put can_link() back in.  Added restricted() for flags.
 * With #define GOD_PRIV, sub-wizards can no longer set other
 * players to Wizard.
 *
 * Revision 1.2  90/08/02  18:54:04  rearl
 * Fixed controls() to return TRUE if exit is unlinked.  Everyone
 * controls an unlinked exit.  Got rid of can_link().
 *
 * Revision 1.1  90/07/19  23:03:58  casie
 * Initial revision
 *
 *
 */

#include "copyright.h"
#include "config.h"

/* Predicates for testing various conditions */

#include <ctype.h>

#include "db.h"
#include "props.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "externs.h"

int 
can_link_to(dbref who, object_flag_type what_type, dbref where)
{
    if (where == HOME)
	return 1;
    if (where < 0 || where > db_top)
	return 0;
    switch (what_type) {
	case TYPE_EXIT:
	    return (controls(who, where) || (FLAGS(where) & LINK_OK));
	    /* NOTREACHED */
	    break;
	case TYPE_PLAYER:
	    return (Typeof(where) == TYPE_ROOM && (controls(who, where)
						   || Linkable(where)));
	    /* NOTREACHED */
	    break;
	case TYPE_ROOM:
            return ((Typeof(where) == TYPE_ROOM || Typeof(where) == TYPE_THING)
		    && (controls(who, where) || Linkable(where)));
	    /* NOTREACHED */
	    break;
	case TYPE_THING:
            return ((Typeof(where) == TYPE_ROOM || Typeof(where) == TYPE_PLAYER || Typeof(where) == TYPE_THING)
		    && (controls(who, where) || Linkable(where)));
	    /* NOTREACHED */
	    break;
	case NOTYPE:
	    return (controls(who, where) || (FLAGS(where) & LINK_OK) ||
		    (Typeof(where) != TYPE_THING && (FLAGS(where) & ABODE)));
	    /* NOTREACHED */
	    break;
    }
    return 0;
}

int 
can_link(dbref who, dbref what)
{
    return (controls(who, what) || ((Typeof(what) == TYPE_EXIT)
				    && DBFETCH(what)->sp.exit.ndest == 0));
}

/*
 * Revision 1.2 -- SECURE_TELEPORT
 * you can only jump with an action from rooms that you own
 * or that are jump_ok, and you cannot jump to players that are !jump_ok.
 */

int 
could_doit(dbref player, dbref thing)
{
    dbref   source, dest, owner;

    if (Typeof(thing) == TYPE_EXIT) {
	if (DBFETCH(thing)->sp.exit.ndest == 0) {
	    return 0;
	}

	owner = OWNER(thing);
	source = DBFETCH(player)->location;
	dest = *(DBFETCH(thing)->sp.exit.dest);

	if (Typeof(dest) == TYPE_PLAYER) {
	    dbref destplayer = dest;
	    dest = DBFETCH(dest)->location;
	    if (!(FLAGS(destplayer) & JUMP_OK) || (FLAGS(dest) & BUILDER)) {
		return 0;
	    }
	}

	/* for actions */
	if ((DBFETCH(thing)->location != NOTHING) &&
		(Typeof(DBFETCH(thing)->location) != TYPE_ROOM))
	{

	    if ((Typeof(dest) == TYPE_ROOM || Typeof(dest) == TYPE_PLAYER) &&
		    (FLAGS(source) & BUILDER))
		return 0;

	    if (tp_secure_teleport && Typeof(dest) == TYPE_ROOM) {
		if ((dest != HOME) && (!controls(owner, source))
			&& ((FLAGS(source) & JUMP_OK) == 0)) {
		    return 0;
                }
	    }
	}
    }

    return (eval_boolexp(player, GETLOCK(thing), thing));
}


int
test_lock(dbref player, dbref thing, const char *lockprop)
{
    struct boolexp *lokptr;

    lokptr = get_property_lock(thing, lockprop);
    return (eval_boolexp(player, lokptr, thing));
}


int
test_lock_false_default(dbref player, dbref thing, const char *lockprop)
{
    struct boolexp *lok = get_property_lock(thing, lockprop);

    if (lok == TRUE_BOOLEXP) return 0;
    return (eval_boolexp(player, lok, thing));
}


int 
can_doit(dbref player, dbref thing, const char *default_fail_msg)
{
    dbref   loc;
    char    buf[BUFFER_LEN];
    char    buf2[BUFFER_LEN];

    if ((loc = getloc(player)) == NOTHING)
	return 0;

    if (!Wizard(OWNER(player)) && Typeof(player) == TYPE_THING &&
	    (FLAGS(thing) & ZOMBIE)) {
	notify(player, "Sorry, but zombies can't do that.");
	return 0;
    }
    if (!could_doit(player, thing)) {
	/* can't do it */
	if (GETFAIL(thing)) {
	    exec_or_notify(player, thing, GETFAIL(thing), "(@Fail)");
	} else if (default_fail_msg) {
	    notify(player, default_fail_msg);
	}
	if (GETOFAIL(thing) && !Dark(player)) {
	    parse_omessage(player, loc, thing, GETOFAIL(thing),
			    PNAME(player), "(@Ofail)");
	}
	return 0;
    } else {
	/* can do it */
	if (GETSUCC(thing)) {
	    exec_or_notify(player, thing, GETSUCC(thing), "(@Succ)");
	}
	if (GETOSUCC(thing) && !Dark(player)) {
	    parse_omessage(player, loc, thing, GETOSUCC(thing),
			    NAME(player), "(@Osucc)");
	}
	return 1;
    }
}

int 
can_see(dbref player, dbref thing, int can_see_loc)
{
    if (player == thing || Typeof(thing) == TYPE_EXIT
	    || Typeof(thing) == TYPE_ROOM)
	return 0;

    if (can_see_loc) {
	switch (Typeof(thing)) {
	    case TYPE_PROGRAM:
		return ((FLAGS(thing) & LINK_OK) || controls(player, thing));
            case TYPE_PLAYER:
		if (tp_dark_sleepers) {
		    return (!Dark(thing) && online(thing));
		}
	    default:
		return (!Dark(thing) ||
		     (controls(player, thing) && !(FLAGS(player) & STICKY)));
	}
    } else {
	/* can't see loc */
	return (controls(player, thing) && !(FLAGS(player) & STICKY));
    }
}

int 
controls_core(dbref who, dbref what, int wizcheck)
{
    dbref index;

    /* No one controls invalid objects */
    if (what < 0 || what >= db_top)
	return 0;

    /* No one controls garbage */
    if (Typeof(what) == TYPE_GARBAGE)
	return 0;

    if (Typeof(who) != TYPE_PLAYER)
	who = OWNER(who);

    /* Wizard controls everything else */
    if (Wizard(who))
	return 1;

    if (tp_realms_control) {
	/* Realm Owner controls everything under his environment. */
	for (index=what; index != NOTHING; index = getloc(index)) {
	    if ((OWNER(index) == who) && (Typeof(index) == TYPE_ROOM)
		    && Wizard(index))
		return 1;
	}
    }

    /* exits are also controlled by the owners of the source and destination */
    /* ACTUALLY, THEY AREN'T.  IT OPENS A BAD MPI SECURITY HOLE. */
    /*
     * if (Typeof(what) == TYPE_EXIT) {
     *    int     i = DBFETCH(what)->sp.exit.ndest;
     *
     *    while (i > 0) {
     *	      if (who == OWNER(DBFETCH(what)->sp.exit.dest[--i]))
     *            return 1;
     *    }
     *    if (who == OWNER(DBFETCH(what)->location))
     *        return 1;
     * }
     */

    /* owners control their own stuff */
    if (who == OWNER(what)) return 1;

    if (test_lock_false_default(who, what, "@/olk")) return 1;

    return 0;
}

/* Standard */
int controls(dbref who, dbref what)
{
    return(controls_core(who, what, TRUE));
}

/* Used by some MPI wierdness */
int controls_nowizperm(dbref who, dbref what)
{
    return(controls_core(who, what, FALSE));
}

/* Read only control */
int may_read_nowizperm(dbref who, dbref what)
{
    return(controls_core(who, what, FALSE) || 
           test_lock_false_default(who, what, "@/rlk")) ;
}

/* Read only control */
int may_read(dbref who, dbref what)
{
    return(controls_core(who, what, TRUE) || 
           test_lock_false_default(who, what, "@/rlk")) ;
}

int 
restricted(dbref player, dbref thing, object_flag_type flag)
{
    switch (flag) {
	case ABODE:
	    return (!TrueWizard(OWNER(player)) &&
		    (Typeof(thing) == TYPE_PROGRAM));
	    /* NOTREACHED */
	    break;
	case ZOMBIE:
	    if (Typeof(thing) == TYPE_PLAYER || 
                Typeof(thing) == TYPE_EXIT || 
		Typeof(thing) == TYPE_ROOM)
		return(!(Wizard(OWNER(player))));
	    if ((Typeof(thing) == TYPE_THING) &&
		    (FLAGS(OWNER(player)) & ZOMBIE))
		return(!(Wizard(OWNER(player))));
	    return(0);
	case VEHICLE:
	    if (Typeof(thing) == TYPE_PLAYER)
		return(!(Wizard(OWNER(player))));
	    if (tp_wiz_vehicles) {
		if (Typeof(thing) == TYPE_THING)
		    return(!(Wizard(OWNER(player))));
	    } else {
		if ((Typeof(thing) == TYPE_THING) && (FLAGS(player) & VEHICLE))
		    return(!(Wizard(OWNER(player))));
	    }
	    return(0);
	case DARK:
            if (!Wizard(OWNER(player))) {
		if (Typeof(thing) == TYPE_PLAYER)
		    return(1);
		if (!tp_exit_darking && Typeof(thing) == TYPE_EXIT)
		    return(1);
		if (!tp_thing_darking && Typeof(thing) == TYPE_THING)
		    return(1);
	    }
            return(0);

	    /* NOTREACHED */
	    break;
	case QUELL:
	    /* You cannot quell or unquell another wizard. */
	    return (TrueWizard(thing) && (thing != player) &&
		    (Typeof(thing) == TYPE_PLAYER));
	    /* NOTREACHED */
	    break;
	case MUCKER:
	case SMUCKER:
	case BUILDER:
	    return (!Wizard(OWNER(player)));
	    /* NOTREACHED */
	    break;
	case WIZARD:
	    if (Wizard(OWNER(player))) {

#ifdef GOD_PRIV
		return ((Typeof(thing) == TYPE_PLAYER) && !God(player));
#else				/* !GOD_PRIV */
		return 0;
#endif				/* GOD_PRIV */
	    } else
		return 1;
	    /* NOTREACHED */
	    break;
	default:
	    return 0;
	    /* NOTREACHED */
	    break;
    }
}

int 
payfor(dbref who, int cost)
{
    who = OWNER(who);
    if (Wizard(who)) {
	return 1;
    } else if (DBFETCH(who)->sp.player.pennies >= cost) {
	DBFETCH(who)->sp.player.pennies -= cost;
	DBDIRTY(who);
	return 1;
    } else {
	return 0;
    }
}

int 
word_start(const char *str, const char let)
{
    int     chk;

    for (chk = 1; *str; str++) {
	if (chk && *str == let)
	    return 1;
	chk = *str == ' ';
    }
    return 0;
}

int 
ok_name(const char *name)
{
    return (name
	    && *name
	    && *name != LOOKUP_TOKEN
	    && *name != REGISTERED_TOKEN
	    && *name != NUMBER_TOKEN
	    && !index(name, ARG_DELIMITER)
	    && !index(name, AND_TOKEN)
	    && !index(name, OR_TOKEN)
	    && !index(name, '\r')
	    && !word_start(name, NOT_TOKEN)
	    && string_compare(name, "me")
	    && string_compare(name, "home")
	    && string_compare(name, "here"));
}

int 
ok_player_name(const char *name)
{
    const char *scan;

    if (!ok_name(name) || strlen(name) > PLAYER_NAME_LIMIT)
	return 0;

    for (scan = name; *scan; scan++) {
	if (!(isprint(*scan) && !isspace(*scan))) {	/* was isgraph(*scan) */
	    return 0;
	}
    }

    /* lookup name to avoid conflicts */
    return (lookup_player(name) == NOTHING);
}

int 
ok_password(const char *password)
{
    const char *scan;

    if (*password == '\0')
	return 0;

    for (scan = password; *scan; scan++) {
	if (!(isprint(*scan) && !isspace(*scan))) {
	    return 0;
	}
    }

    return 1;
}

dbref what_zone(dbref obj)
{
    dbref index;

    switch (Typeof(obj)) {
        case TYPE_PLAYER:
            index = DBFETCH(obj)->sp.player.home;
            break;
        case TYPE_THING:
            index = DBFETCH(obj)->sp.thing.home;
            break;
        case TYPE_PROGRAM:
            index = OWNER(obj);
            break;
	default:
	    index = getloc(obj);
	    break;
    }

    for (; index != NOTHING; index = getloc(index))
    {
        if ((Typeof(index) == TYPE_ROOM) && (FLAGS(index) & ZONE))
            return index;
    }

    return 0; /* Default zone is #0 */
}

