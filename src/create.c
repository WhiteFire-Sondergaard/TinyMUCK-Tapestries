/* $Header: /home/foxen/CR/FB/src/RCS/create.c,v 1.1 1996/06/12 02:15:14 foxen Exp $ */

/*
 * $Log: create.c,v $
 * Revision 1.1  1996/06/12 02:15:14  foxen
 * Initial revision
 *
 * Revision 5.3  1994/03/14  12:20:58  foxen
 * Fb5.20 release checkpoint.
 *
 * Revision 5.2  1994/01/18  20:52:20  foxen
 * Version 5.15 release.
 *
 * Revision 5.1  1993/12/17  00:07:33  foxen
 * initial revision.
 *
 * Revision 1.9  90/09/28  12:19:26  rearl
 * Added MUCKER check to @edit command.
 *
 * Revision 1.8  90/09/18  07:54:22  rearl
 * Miscellaneous stuff -- moved .sp.program.locked to the new INTERNAL flag.
 *
 * Revision 1.7  90/09/16  04:41:50  rearl
 * Preparation code added for disk-based MUCK.
 *
 * Revision 1.6  90/08/27  03:21:46  rearl
 * Changes in link parsing, disk-based MUF source code.
 *
 * Revision 1.5  90/08/15  02:57:42  rearl
 * Removed some extraneous stuff, consolidated others, general cleanup.
 *
 * Revision 1.4  90/08/05  03:19:08  rearl
 * Redid matching routines.
 *
 * Revision 1.3  90/08/02  18:48:52  rearl
 * Fixed some calls to logging functions.
 *
 * Revision 1.2  90/08/02  02:15:29  rearl
 * Fixed JUMP_OK player <-> room correlations.  JUMP_OK players now
 * create JUMP_OK rooms when they @dig.
 *
 * Revision 1.1  90/07/19  23:03:22  casie
 * Initial revision
 *
 *
 */

#include "copyright.h"
#include "config.h"

/* Commands that create new objects */

#include "db.h"
#include "props.h"
#include "params.h"
#include "tune.h"
#include "interface.h"
#include "externs.h"
#include "match.h"
#include "strings.h"
#include <ctype.h>

struct line *read_program(dbref i);

/* parse_linkable_dest()
 *
 * A utility for open and link which checks whether a given destination
 * string is valid.  It returns a parsed dbref on success, and NOTHING
 * on failure.
 */

static dbref 
parse_linkable_dest(dbref player, dbref exit,
		    const char *dest_name)
{
    dbref   dobj;		/* destination room/player/thing/link */
    static char buf[BUFFER_LEN];
    struct match_data md;

    init_match(player, dest_name, NOTYPE, &md);
    match_absolute(&md);
    match_everything(&md);
    match_home(&md);

    if ((dobj = match_result(&md)) == NOTHING || dobj == AMBIGUOUS) {
	sprintf(buf, "I couldn't find '%s'.", dest_name);
	notify(player, buf);
	return NOTHING;

    }

    if (!tp_teleport_to_player && Typeof(dobj) == TYPE_PLAYER) {
	sprintf(buf, "You can't link to players.  Destination %s ignored.",
		unparse_object(player, dobj));
	notify(player, buf);
	return NOTHING;
    }

    if (!can_link(player, exit)) {
	notify(player, "You can't link that.");
	return NOTHING;
    }
    if (!can_link_to(player, Typeof(exit), dobj)) {
	sprintf(buf, "You can't link to %s.", unparse_object(player, dobj));
	notify(player, buf);
	return NOTHING;
    } else {
	return dobj;
    }
}

/* exit_loop_check()
 *
 * Recursive check for loops in destinations of exits.  Checks to see
 * if any circular references are present in the destination chain.
 * Returns 1 if circular reference found, 0 if not.
 */
int 
exit_loop_check(dbref source, dbref dest)
{

    int     i;

    if (source == dest)
	return 1;		/* That's an easy one! */
    if (Typeof(dest) != TYPE_EXIT)
	return 0;
    for (i = 0; i < DBFETCH(dest)->sp.exit.ndest; i++) {
	if ((DBFETCH(dest)->sp.exit.dest)[i] == source) {
	    return 1;		/* Found a loop! */
	}
	if (Typeof((DBFETCH(dest)->sp.exit.dest)[i]) == TYPE_EXIT) {
	    if (exit_loop_check(source, (DBFETCH(dest)->sp.exit.dest)[i])) {
		return 1;	/* Found one recursively */
	    }
	}
    }
    return 0;			/* No loops found */
}

/* use this to create an exit */
void 
do_open(dbref player, const char *direction, const char *linkto)
{
    dbref   loc, exit;
    dbref   good_dest[MAX_LINKS];
    char    buf[BUFFER_LEN];
    char    buf2[BUFFER_LEN];
    char   *rname, *qname;
    int     i, ndest;

    if (!Builder(player)) {
	notify(player, "That command is restricted to authorized builders.");
	return;
    }
    strcpy(buf2, linkto);
    for (rname = buf2; (*rname && (*rname != '=')); rname++);
    qname = rname;
    if (*rname)
	rname++;
    qname = '\0';

    while (((qname--) > buf2) && (isspace(*qname)))
	*qname = '\0';
    qname = buf2;
    for (; *rname && isspace(*rname); rname++);

    if ((loc = getloc(player)) == NOTHING)
	return;
    if (!*direction) {
	notify(player, "You must specify a direction or action name to open.");
	return;
    } else if (!ok_name(direction)) {
	notify(player, "That's a strange name for an exit!");
	return;
    }
    if (!controls(player, loc)) {
	notify(player, "Permission denied.");
	return;
    } else if (!payfor(player, tp_exit_cost)) {
	notify_fmt(player, "Sorry, you don't have enough %s to open an exit.",
                   tp_pennies);
	return;
    } else {
	/* create the exit */
	exit = new_object();

	/* initialize everything */
	NAME(exit) = alloc_string(direction);
	DBFETCH(exit)->location = loc;
	OWNER(exit) = OWNER(player);
	FLAGS(exit) = TYPE_EXIT;
	DBFETCH(exit)->sp.exit.ndest = 0;
	DBFETCH(exit)->sp.exit.dest = NULL;

	/* link it in */
	PUSH(exit, DBFETCH(loc)->exits);
	DBDIRTY(loc);

	/* and we're done */
	sprintf(buf, "Exit opened with number %d.", exit);
	notify(player, buf);

	/* check second arg to see if we should do a link */
	if (*qname != '\0') {
	    notify(player, "Trying to link...");
	    if (!payfor(player, tp_link_cost)) {
		notify_fmt(player, "You don't have enough %s to link.",
                           tp_pennies);
	    } else {
		ndest = link_exit(player, exit, (char *) qname, good_dest);
		DBFETCH(exit)->sp.exit.ndest = ndest;
		DBFETCH(exit)->sp.exit.dest = (dbref *) malloc(sizeof(dbref) * ndest);
		for (i = 0; i < ndest; i++) {
		    (DBFETCH(exit)->sp.exit.dest)[i] = good_dest[i];
		}
		DBDIRTY(exit);
	    }
	}
    }

    if (*rname) {
	sprintf(buf, "Registered as $%s", rname);
	notify(player, buf);
	sprintf(buf, "_reg/%s", rname);
	set_property(player, buf, PROP_REFTYP, (int)exit);
    }
}

/*
 * link_exit()
 *
 * This routine connects an exit to a bunch of destinations.
 *
 * 'player' contains the player's name.
 * 'exit' is the the exit whose destinations are to be linked.
 * 'dest_name' is a character string containing the list of exits.
 *
 * 'dest_list' is an array of dbref's where the valid destinations are
 * stored.
 *
 */

#ifdef mips
int
link_exit(player, exit, dest_name, dest_list)
    dbref   player, exit;
    char   *dest_name;
    dbref   dest_list[];

#else				/* !mips */
int 
link_exit(dbref player, dbref exit, char *dest_name, dbref * dest_list)
#endif				/* mips */
{
    char   *p, *q;
    int     prdest;
    dbref   dest;
    int     ndest;
    char    buf[BUFFER_LEN], qbuf[BUFFER_LEN];

    prdest = 0;
    ndest = 0;

    while (*dest_name) {
	while (isspace(*dest_name))
	    dest_name++;	/* skip white space */
	p = dest_name;
	while (*dest_name && (*dest_name != EXIT_DELIMITER))
	    dest_name++;
	q = (char *)strncpy(qbuf, p, BUFFER_LEN);	/* copy word */
	q[(dest_name - p)] = '\0';	/* terminate it */
	if (*dest_name)
	    for (dest_name++; *dest_name && isspace(*dest_name); dest_name++);

	if ((dest = parse_linkable_dest(player, exit, q)) == NOTHING)
	    continue;

	switch (Typeof(dest)) {
	    case TYPE_PLAYER:
	    case TYPE_ROOM:
	    case TYPE_PROGRAM:
		if (prdest) {
		    sprintf(buf, "Only one player, room, or program destination allowed. Destination %s ignored.", unparse_object(player, dest));
		    notify(player, buf);
		    continue;
		}
		dest_list[ndest++] = dest;
		prdest = 1;
		break;
	    case TYPE_THING:
		dest_list[ndest++] = dest;
		break;
	    case TYPE_EXIT:
		if (exit_loop_check(exit, dest)) {
		    sprintf(buf, "Destination %s would create a loop, ignored.",
			    unparse_object(player, dest));
		    notify(player, buf);
		    continue;
		}
		dest_list[ndest++] = dest;
		break;
	    default:
		notify(player, "Internal error: weird object type.");
		log_status("PANIC: weird object: Typeof(%d) = %d\n",
			   dest, Typeof(dest));
		break;
	}
	if (dest == HOME) {
	    notify(player, "Linked to HOME.");
	} else {
	    sprintf(buf, "Linked to %s.", unparse_object(player, dest));
	    notify(player, buf);
	}
	if (ndest >= MAX_LINKS) {
	    notify(player, "Too many destinations, rest ignored.");
	    break;
	}
    }
    return ndest;
}

/* do_link
 *
 * Use this to link to a room that you own.  It also sets home for
 * objects and things, and drop-to's for rooms.
 * It seizes ownership of an unlinked exit, and costs 1 penny
 * plus a penny transferred to the exit owner if they aren't you
 *
 * All destinations must either be owned by you, or be LINK_OK.
 */
void 
do_link(dbref player, const char *thing_name, const char *dest_name)
{
    dbref   thing;
    dbref   dest;
    dbref   good_dest[MAX_LINKS];
    struct match_data md;

    int     ndest, i;

    init_match(player, thing_name, TYPE_EXIT, &md);
    match_all_exits(&md);
    match_neighbor(&md);
    match_possession(&md);
    match_me(&md);
    match_here(&md);
    match_absolute(&md);
    match_registered(&md);
    if (Wizard(OWNER(player))) {
	match_player(&md);
    }
    if ((thing = noisy_match_result(&md)) == NOTHING)
	return;

    switch (Typeof(thing)) {
	case TYPE_EXIT:
	    /* we're ok, check the usual stuff */
	    if (DBFETCH(thing)->sp.exit.ndest != 0) {
		if (controls(player, thing)) {
		    notify(player, "That exit is already linked.");
		    return;
		} else {
		    notify(player, "Permission denied.");
		    return;
		}
	    }
	    /* handle costs */
	    if (OWNER(thing) == OWNER(player)) {
		if (!payfor(player, tp_link_cost)) {
		    notify_fmt(player, "It costs %d %s to link this exit.",
                               tp_link_cost, (tp_link_cost==1)?tp_penny:tp_pennies);
		    return;
		}
	    } else {
		if (!payfor(player, tp_link_cost + tp_exit_cost)) {
		    notify_fmt(player, "It costs %d %s to link this exit.",
                               (tp_link_cost+tp_exit_cost),
                               (tp_link_cost+tp_exit_cost == 1)?tp_penny:tp_pennies);
		    return;
		} else if (!Builder(player)) {
		    notify(player, "Only authorized builders may seize exits.");
		    return;
		} else {
		    /* pay the owner for his loss */
		    dbref   owner = OWNER(thing);

		    DBFETCH(owner)->sp.player.pennies += tp_exit_cost;
		    DBDIRTY(owner);
		}
	    }

	    /* link has been validated and paid for; do it */
	    OWNER(thing) = OWNER(player);
	    ndest = link_exit(player, thing, (char *) dest_name, good_dest);
	    if (ndest == 0) {
		notify(player, "No destinations linked.");
		DBFETCH(player)->sp.player.pennies += tp_link_cost;	/* Refund! */
		DBDIRTY(player);
		break;
	    }
	    DBFETCH(thing)->sp.exit.ndest = ndest;
	    DBFETCH(thing)->sp.exit.dest = (dbref *) malloc(sizeof(dbref) * ndest);
	    for (i = 0; i < ndest; i++) {
		(DBFETCH(thing)->sp.exit.dest)[i] = good_dest[i];
	    }
	    break;
	case TYPE_THING:
	case TYPE_PLAYER:
	    init_match(player, dest_name, TYPE_ROOM, &md);
	    match_neighbor(&md);
	    match_absolute(&md);
	    match_registered(&md);
	    match_me(&md);
	    match_here(&md);
            if (Typeof(thing) == TYPE_THING)
                match_possession(&md);
	    if ((dest = noisy_match_result(&md)) == NOTHING)
		return;
	    if (!controls(player, thing)
		    || !can_link_to(player, Typeof(thing), dest)) {
		notify(player, "Permission denied.");
		return;
	    }
	    if (parent_loop_check(thing, dest)) {
		notify(player, "That would cause a parent paradox.");
		return;
	    }
	    /* do the link */
	    if (Typeof(thing) == TYPE_THING) {
		DBFETCH(thing)->sp.thing.home = dest;
	    } else
		DBFETCH(thing)->sp.player.home = dest;
	    notify(player, "Home set.");
	    break;
	case TYPE_ROOM:	/* room dropto's */
	    init_match(player, dest_name, TYPE_ROOM, &md);
	    match_neighbor(&md);
            match_possession(&md);
	    match_registered(&md);
	    match_absolute(&md);
	    match_home(&md);
	    if ((dest = noisy_match_result(&md)) == NOTHING)
		break;
	    if (!controls(player, thing) || !can_link_to(player, Typeof(thing), dest)
		    || (thing == dest)) {
		notify(player, "Permission denied.");
	    } else {
		DBFETCH(thing)->sp.room.dropto = dest;	/* dropto */
		notify(player, "Dropto set.");
	    }
	    break;
	case TYPE_PROGRAM:
	    notify(player, "You can't link programs to things!");
	    break;
	default:
	    notify(player, "Internal error: weird object type.");
	    log_status("PANIC: weird object: Typeof(%d) = %d\n",
		       thing, Typeof(thing));
	    break;
    }
    DBDIRTY(thing);
    return;
}

/*
 * do_dig
 *
 * Use this to create a room.
 */
void 
do_dig(dbref player, const char *name, const char *pname)
{
    dbref   room;
    dbref   parent;
    dbref   newparent;
    char    buf[BUFFER_LEN];
    char    rbuf[BUFFER_LEN];
    char    qbuf[BUFFER_LEN];
    char   *rname;
    char   *qname;
    struct match_data md;

    if (!Builder(player)) {
	notify(player, "That command is restricted to authorized builders.");
	return;
    }
    if (*name == '\0') {
	notify(player, "You must specify a name for the room.");
	return;
    }
    if (!ok_name(name)) {
	notify(player, "That's a silly name for a room!");
	return;
    }
    if (!payfor(player, tp_room_cost)) {
	notify_fmt(player, "Sorry, you don't have enough %s to dig a room.",
                   tp_pennies);
	return;
    }
    room = new_object();

    /* Initialize everything */
    newparent = DBFETCH(DBFETCH(player)->location)->location;
    while ((newparent != NOTHING) && !(FLAGS(newparent) & ABODE))
	newparent = DBFETCH(newparent)->location;
    if (newparent == NOTHING)
	newparent = GLOBAL_ENVIRONMENT;

    NAME(room) = alloc_string(name);
    DBFETCH(room)->location = newparent;
    OWNER(room) = OWNER(player);
    DBFETCH(room)->exits = NOTHING;
    DBFETCH(room)->sp.room.dropto = NOTHING;
    FLAGS(room) = TYPE_ROOM | (FLAGS(player) & JUMP_OK);
    PUSH(room, DBFETCH(newparent)->contents);
    DBDIRTY(room);
    DBDIRTY(newparent);

    sprintf(buf, "%s created with room number %d.", name, room);
    notify(player, buf);

    strcpy(buf, pname);
    for (rname = buf; (*rname && (*rname != '=')); rname++);
    qname = rname;
    if (*rname)
	*(rname++) = '\0';
    while ((qname > buf) && (isspace(*qname)))
	*(qname--) = '\0';
    qname = buf;
    for (; *rname && isspace(*rname); rname++);
    rname = strcpy(rbuf, rname);
    qname = strcpy(qbuf, qname);

    if (*qname) {
	notify(player, "Trying to set parent...");
	init_match(player, qname, TYPE_ROOM, &md);
	match_absolute(&md);
	match_registered(&md);
	match_here(&md);
	if ((parent = noisy_match_result(&md)) == NOTHING
		|| parent == AMBIGUOUS) {
	    notify(player, "Parent set to default.");
	} else {
	    if (!can_link_to(player, Typeof(room), parent) || room == parent) {
		notify(player, "Permission denied.  Parent set to default.");
	    } else {
		moveto(room, parent);
		sprintf(buf, "Parent set to %s.", unparse_object(player, parent));
		notify(player, buf);
	    }
	}
    }

    if (*rname) {
	sprintf(buf, "_reg/%s", rname);
	set_property(player, buf, PROP_REFTYP, (int)room);
	sprintf(buf, "Room registered as $%s", rname);
	notify(player, buf);
    }
}

/*
  Use this to create a program.
  First, find a program that matches that name.  If there's one,
  then we put him into edit mode and do it.
  Otherwise, we create a new object for him, and call it a program.
  */
void
do_prog(dbref player, const char *name)
{
    dbref   i;
    int     jj;
    dbref   newprog;
    char    buf[BUFFER_LEN];
    struct match_data md;

    if (Typeof(player) != TYPE_PLAYER) {
	notify(player, "Only players can edit programs.");
	return;
    }
    if (!Mucker(player)) {
	notify(player, "You're no programmer!");
	return;
    }
    if (!*name) {
	notify(player, "No program name given.");
	return;
    }
    init_match(player, name, TYPE_PROGRAM, &md);

    match_possession(&md);
    match_neighbor(&md);
    match_registered(&md);
    match_absolute(&md);

    if ((i = match_result(&md)) == NOTHING) {
	newprog = new_object();

	NAME(newprog) = alloc_string(name);
	sprintf(buf, "A scroll containing a spell called %s", name);
	SETDESC(newprog, buf);
	DBFETCH(newprog)->location = player;
	FLAGS(newprog) = TYPE_PROGRAM;
	jj = MLevel(player);
	if (jj < 1)
	    jj = 2;
	if (jj > 3)
	    jj = 3;
	SetMLevel(newprog, jj);

	OWNER(newprog) = OWNER(player);
	DBFETCH(newprog)->sp.program.first = 0;
	DBFETCH(newprog)->sp.program.curr_line = 0;
	DBFETCH(newprog)->sp.program.siz = 0;
	DBFETCH(newprog)->sp.program.code = 0;
	DBFETCH(newprog)->sp.program.start = 0;
	DBFETCH(newprog)->sp.program.pubs = 0;

	DBFETCH(player)->sp.player.curr_prog = newprog;

	PUSH(newprog, DBFETCH(player)->contents);
	DBDIRTY(newprog);
	DBDIRTY(player);
	sprintf(buf, "Program %s created with number %d.", name, newprog);
	notify(player, buf);
	notify(player, "Entering editor.");
    } else if (i == AMBIGUOUS) {
	notify(player, "I don't know which one you mean!");
	return;
    } else {
	if ((Typeof(i) != TYPE_PROGRAM) || !controls(player, i)) {
	    notify(player, "Permission denied!");
	    return;
	}
	if (FLAGS(i) & INTERNAL) {
	    notify(player, "Sorry, this program is currently being edited.  Try again later.");
	    return;
	}
	DBFETCH(i)->sp.program.first = read_program(i);
	FLAGS(i) |= INTERNAL;
	DBFETCH(player)->sp.player.curr_prog = i;
	notify(player, "Entering editor.");
	/* list current line */
	do_list(player, i, 0, 0);
	DBDIRTY(i);
    }
    FLAGS(player) |= INTERACTIVE;
    DBDIRTY(player);
}

void
do_edit(dbref player, const char *name)
{
    dbref   i;
    struct match_data md;

    if (Typeof(player) != TYPE_PLAYER) {
	notify(player, "Only players can edit programs.");
	return;
    }
    if (!Mucker(player)) {
	notify(player, "You're no programmer!");
	return;
    }
    if (!*name) {
	notify(player, "No program name given.");
	return;
    }
    init_match(player, name, TYPE_PROGRAM, &md);

    match_possession(&md);
    match_neighbor(&md);
    match_registered(&md);
    match_absolute(&md);

    if ((i = noisy_match_result(&md)) == NOTHING || i == AMBIGUOUS)
	return;

    if ((Typeof(i) != TYPE_PROGRAM) || !controls(player, i)) {
	notify(player, "Permission denied!");
	return;
    }
    if (FLAGS(i) & INTERNAL) {
	notify(player, "Sorry, this program is currently being edited.  Try again later.");
	return;
    }
    FLAGS(i) |= INTERNAL;
    DBFETCH(i)->sp.program.first = read_program(i);
    DBFETCH(player)->sp.player.curr_prog = i;
    notify(player, "Entering editor.");
    /* list current line */
    do_list(player, i, 0, 0);
    FLAGS(player) |= INTERACTIVE;
    DBDIRTY(i);
    DBDIRTY(player);
}

/*
 * do_create
 *
 * Use this to create an object.
 */
void 
do_create(dbref player, char *name, char *acost)
{
    dbref   loc;
    dbref   thing;
    int     cost;

    static char buf[BUFFER_LEN];
    char    buf2[BUFFER_LEN];
    char   *rname, *qname;

    strcpy(buf2, acost);
    for (rname = buf2; (*rname && (*rname != '=')); rname++);
    qname = rname;
    if (*rname)
	*(rname++) = '\0';
    while ((qname > buf2) && (isspace(*qname)))
	*(qname--) = '\0';
    qname = buf2;
    for (; *rname && isspace(*rname); rname++);

    cost = atoi(qname);
    if (!Builder(player)) {
	notify(player, "That command is restricted to authorized builders.");
	return;
    }
    if (*name == '\0') {
	notify(player, "Create what?");
	return;
    } else if (!ok_name(name)) {
	notify(player, "That's a silly name for a thing!");
	return;
    } else if (cost < 0) {
	notify(player, "You can't create an object for less than nothing!");
	return;
    } else if (cost < tp_object_cost) {
	cost = tp_object_cost;
    }
    if (!payfor(player, cost)) {
	notify_fmt(player, "Sorry, you don't have enough %s.", tp_pennies);
	return;
    } else {
	/* create the object */
	thing = new_object();

	/* initialize everything */
	NAME(thing) = alloc_string(name);
	DBFETCH(thing)->location = player;
	OWNER(thing) = OWNER(player);
	DBFETCH(thing)->sp.thing.value = OBJECT_ENDOWMENT(cost);
	DBFETCH(thing)->exits = NOTHING;
	FLAGS(thing) = TYPE_THING;

	/* endow the object */
	if (DBFETCH(thing)->sp.thing.value > tp_max_object_endowment) {
	    DBFETCH(thing)->sp.thing.value = tp_max_object_endowment;
	}
	if ((loc = DBFETCH(player)->location) != NOTHING
		&& controls(player, loc)) {
	    DBFETCH(thing)->sp.thing.home = loc;	/* home */
	} else {
	    DBFETCH(thing)->sp.thing.home = player;
	    /* set thing's home to player instead */
	}

	/* link it in */
	PUSH(thing, DBFETCH(player)->contents);
	DBDIRTY(player);

	/* and we're done */
	sprintf(buf, "%s created with number %d.", name, thing);
	notify(player, buf);
	DBDIRTY(thing);
    }
    if (*rname) {
	sprintf(buf, "Registered as $%s", rname);
	notify(player, buf);
	sprintf(buf, "_reg/%s", rname);
	set_property(player, buf, PROP_REFTYP, (int)thing);
    }
}

/*
 * parse_source()
 *
 * This is a utility used by do_action and do_attach.  It parses
 * the source string into a dbref, and checks to see that it
 * exists.
 *
 * The return value is the dbref of the source, or NOTHING if an
 * error occurs.
 *
 */
dbref 
parse_source(dbref player, const char *source_name)
{
    dbref   source;
    struct match_data md;

    init_match(player, source_name, NOTYPE, &md);	/* source type can be
							 * any */
    match_neighbor(&md);
    match_me(&md);
    match_here(&md);
    match_possession(&md);
    match_registered(&md);
    match_absolute(&md);
    source = noisy_match_result(&md);

    if (source == NOTHING)
	return NOTHING;

    /* You can only attach actions to things you control */
    if (!controls(player, source)) {
	notify(player, "Permission denied.");
	return NOTHING;
    }
    if (Typeof(source) == TYPE_EXIT) {
	notify(player, "You can't attach an action to an action.");
	return NOTHING;
    }
    if (Typeof(source) == TYPE_PROGRAM) {
	notify(player, "You can't attach an action to a program.");
	return NOTHING;
    }
    return source;
}

/*
 * set_source()
 *
 * This routine sets the source of an action to the specified source.
 * It is called by do_action and do_attach.
 *
 */
void 
set_source(dbref player, dbref action, dbref source)
{
    switch (Typeof(source)) {
	case TYPE_ROOM:
	case TYPE_THING:
	case TYPE_PLAYER:
	    PUSH(action, DBFETCH(source)->exits);
	    break;
	default:
	    notify(player, "Internal error: weird object type.");
	    log_status("PANIC: tried to source %d to %d: type: %d\n",
		       action, source, Typeof(source));
	    return;
	    break;
    }
    DBDIRTY(source);
    DBSTORE(action, location, source);
    return;
}

int 
unset_source(dbref player, dbref loc, dbref action)
{

    dbref   oldsrc;

    if ((oldsrc = DBFETCH(action)->location) == NOTHING) {
	/* old-style, sourceless exit */
	if (!member(action, DBFETCH(loc)->exits)) {
	    return 0;
	}
	DBSTORE(DBFETCH(player)->location, exits,
		remove_first(DBFETCH(DBFETCH(player)->location)->exits, action));
    } else {
	switch (Typeof(oldsrc)) {
	    case TYPE_PLAYER:
	    case TYPE_ROOM:
	    case TYPE_THING:
		DBSTORE(oldsrc, exits,
			remove_first(DBFETCH(oldsrc)->exits, action));
		break;
	    default:
		log_status("PANIC: source of action #%d was type: %d.\n",
			   action, Typeof(oldsrc));
		return 0;
		/* NOTREACHED */
		break;
	}
    }
    return 1;
}

/*
 * do_action()
 *
 * This routine attaches a new existing action to a source object,
 * where possible.
 * The action will not do anything until it is LINKed.
 *
 */
void 
do_action(dbref player, const char *action_name, const char *source_name)
{
    dbref   action, source;
    static char buf[BUFFER_LEN];
    char    buf2[BUFFER_LEN];
    char   *rname, *qname;

    if (!Builder(player)) {
	notify(player, "That command is restricted to authorized builders.");
	return;
    }
    strcpy(buf2, source_name);
    for (rname = buf2; (*rname && (*rname != '=')); rname++);
    qname = rname;
    if (*rname)
	*(rname++) = '\0';
    while ((qname > buf2) && (isspace(*qname)))
	*(qname--) = '\0';
    qname = buf2;
    for (; *rname && isspace(*rname); rname++);

    if (!*action_name || !*qname) {
	notify(player, "You must specify an action name and a source object.");
	return;
    } else if (!ok_name(action_name)) {
	notify(player, "That's a strange name for an action!");
	return;
    } else if (!payfor(player, tp_exit_cost)) {
	notify_fmt(player,
                   "Sorry, you don't have enough %s to make an action.",
                   tp_pennies);
	return;
    }
    if (((source = parse_source(player, qname)) == NOTHING))
	return;

    action = new_object();

    NAME(action) = alloc_string(action_name);
    DBFETCH(action)->location = NOTHING;
    OWNER(action) = OWNER(player);
    DBFETCH(action)->sp.exit.ndest = 0;
    DBFETCH(action)->sp.exit.dest = NULL;
    FLAGS(action) = TYPE_EXIT;

    set_source(player, action, source);
    sprintf(buf, "Action created with number %d and attached.", action);
    notify(player, buf);
    DBDIRTY(action);

    if (*rname) {
	sprintf(buf, "Registered as $%s", rname);
	notify(player, buf);
	sprintf(buf, "_reg/%s", rname);
	set_property(player, buf, PROP_REFTYP, (int)action);
    }
}

/*
 * do_attach()
 *
 * This routine attaches a previously existing action to a source object.
 * The action will not do anything unless it is LINKed.
 *
 */
void 
do_attach(dbref player, const char *action_name, const char *source_name)
{
    dbref   action, source;
    dbref   loc;		/* player's current location */
    struct match_data md;

    if ((loc = DBFETCH(player)->location) == NOTHING)
	return;

    if (!Builder(player)) {
	notify(player, "That command is restricted to authorized builders.");
	return;
    }
    if (!*action_name || !*source_name) {
	notify(player, "You must specify an action name and a source object.");
	return;
    }
    init_match(player, action_name, TYPE_EXIT, &md);
    match_all_exits(&md);
    match_registered(&md);
    match_absolute(&md);

    if ((action = noisy_match_result(&md)) == NOTHING)
	return;

    if (Typeof(action) != TYPE_EXIT) {
	notify(player, "That's not an action!");
	return;
    } else if (!controls(player, action)) {
	notify(player, "Permission denied.");
	return;
    }
    if (((source = parse_source(player, source_name)) == NOTHING)
	    || Typeof(source) == TYPE_PROGRAM)
	return;

    if (!unset_source(player, loc, action)) {
	return;
    }
    set_source(player, action, source);
    notify(player, "Action re-attached.");
    if (MLevRaw(action)) {
	SetMLevel(action, 0);
        notify(player, "Action priority Level reset to zero.");
    }
}
