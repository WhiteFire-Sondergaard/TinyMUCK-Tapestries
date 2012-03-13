/* Primitives package */

#include "copyright.h"
#include "config.h"
#include "params.h"

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "db.h"
#include "tune.h"
#include "props.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "strings.h"
#include "interp.h"

static struct inst *oper1, *oper2, *oper3, *oper4;
/* static struct inst temp1, temp2, temp3; */
static int tmp, result;
static dbref ref;
static char buf[BUFFER_LEN];


void 
copyobj(dbref player, dbref old, dbref new)
{
    //struct object *oldp = DBFETCH(old);
    struct object *newp = DBFETCH(new);

    NAME(new) = alloc_string(NAME(old));
    newp->properties = copy_prop(old);
    newp->exits = NOTHING;
    newp->contents = NOTHING;
    newp->next = NOTHING;
    newp->location = NOTHING;
    moveto(new, player);

#ifdef DISKBASE
    newp->propsfpos = 0;
    newp->propsmode = PROPS_UNLOADED;
    newp->propstime = 0;
    newp->nextold = NOTHING;
    newp->prevold = NOTHING;
    dirtyprops(new);
#endif

    DBDIRTY(new);
}



void 
prim_addpennies(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (mlev < 2)
	abort_interp("Requires Mucker Level 2 or better.");
    if (!valid_object(oper2))
	abort_interp("Invalid object.");
    if (oper1->type != PROG_INTEGER)
	abort_interp("Non-integer argument (2)");
    ref = oper2->data.objref;
    if (Typeof(ref) == TYPE_PLAYER) {
	result = DBFETCH(ref)->sp.player.pennies;
	if (mlev < 4) {
	    if (oper1->data.number > 0) {
		if (result > (result + oper1->data.number))
		    abort_interp("Would roll over player's score.");
		if ((result + oper1->data.number) > tp_max_pennies)
		    abort_interp("Would exceed MAX_PENNIES.");
	    } else {
		if (result < (result + oper1->data.number))
		    abort_interp("Would roll over player's score.");
		if ((result + oper1->data.number) < 0)
		    abort_interp("Result would be negative.");
	    }
	}
	result += oper1->data.number;
	DBFETCH(ref)->sp.player.pennies += oper1->data.number;
	DBDIRTY(ref);
    } else if (Typeof(ref) == TYPE_THING) {
	if (mlev < 4)
	    abort_interp("Permission denied.");
	result = DBFETCH(ref)->sp.thing.value + oper1->data.number;
	if (result < 1)
	    abort_interp("Result must be positive.");
	DBFETCH(ref)->sp.thing.value += oper1->data.number;
	DBDIRTY(ref);
    } else {
	abort_interp("Invalid object type.");
    }
    CLEAR(oper1);
    CLEAR(oper2);
}

void 
prim_moveto(PRIM_PROTOTYPE)
{
    struct inst *oper1, *oper2 /* , *oper3, *oper4 */;
    //struct inst temp1, temp2, temp3;
    //int tmp, result;
    //dbref ref;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (fr->level > 8)
	abort_interp("Interp call loops not allowed.");
    if (!(valid_object(oper1) && valid_object(oper2)) && !is_home(oper1))
	abort_interp("Non-object argument.");
    {
	dbref   victim, dest;

	victim = oper2->data.objref;
	dest = oper1->data.objref;

	if (Typeof(dest) == TYPE_EXIT)
	    abort_interp("Destination argument is an exit.");
	if (Typeof(victim) == TYPE_EXIT && (mlev < 3))
	    abort_interp("Permission denied.");
	if (!(FLAGS(victim) & JUMP_OK)
		&& !permissions(ProgUID, victim) && (mlev < 3))
	    abort_interp("Object can't be moved.");
	switch (Typeof(victim)) {
	    case TYPE_PLAYER:
		if (Typeof(dest) != TYPE_ROOM &&
			!(Typeof(dest)==TYPE_THING && (FLAGS(dest)&VEHICLE)))
		    abort_interp("Bad destination.");
		/* Check permissions */
		if (parent_loop_check(victim, dest))
		    abort_interp("Things can't contain themselves.");
		if ((mlev < 3)) {
		    if (!(FLAGS(DBFETCH(victim)->location) & JUMP_OK)
			 && !permissions(ProgUID, DBFETCH(victim)->location))
			abort_interp("Source not JUMP_OK.");
		    if (!is_home(oper1) && !(FLAGS(dest) & JUMP_OK)
			    && !permissions(ProgUID, dest))
			abort_interp("Destination not JUMP_OK.");
		    if (Typeof(dest)==TYPE_THING
                            && getloc(victim) != getloc(dest))
                        abort_interp("Not in same location as vehicle.");
		}
		enter_room(victim, dest, program);
		break;
	    case TYPE_THING:
		if (parent_loop_check(victim, dest))
		    abort_interp("A thing cannot contain itself.");
		if (mlev < 3 && (FLAGS(victim) & VEHICLE) &&
			(FLAGS(dest) & VEHICLE) && Typeof(dest) != TYPE_THING)
		    abort_interp("Destination doesn't accept vehicles.");
		if (mlev < 3 && (FLAGS(victim) & ZOMBIE) &&
			(FLAGS(dest) & ZOMBIE) && Typeof(dest) != TYPE_THING)
		    abort_interp("Destination doesn't accept zombies.");
		ts_lastuseobject(victim);
	    case TYPE_PROGRAM:
		{
		    dbref   matchroom = NOTHING;

		    if (Typeof(dest) != TYPE_ROOM && Typeof(dest) != TYPE_PLAYER
			    && Typeof(dest) != TYPE_THING)
			abort_interp("Bad destination.");
		    if ((mlev < 3)) {
			if (permissions(ProgUID, dest))
			    matchroom = dest;
			if (permissions(ProgUID, DBFETCH(victim)->location))
			    matchroom = DBFETCH(victim)->location;
			if (matchroom != NOTHING && !(FLAGS(matchroom)&JUMP_OK)
				&& !permissions(ProgUID, victim))
			    abort_interp("Permission denied.");
		    }
		}
		if (Typeof(victim)==TYPE_THING && (FLAGS(victim) & ZOMBIE)) {
		    enter_room(victim, dest, program);
		} else {
		    moveto(victim, dest);
		}
		break;
	    case TYPE_EXIT:
		if ((mlev < 3) && (!permissions(ProgUID, victim)
			|| !permissions(ProgUID, dest)))
		    abort_interp("Permission denied.");
		if (Typeof(dest)!=TYPE_ROOM && Typeof(dest)!=TYPE_THING &&
			Typeof(dest) != TYPE_PLAYER)
		    abort_interp("Bad destination object.");
		if (!unset_source(ProgUID, getloc(player), victim))
		    break;
		set_source(ProgUID, victim, dest);
		SetMLevel(victim, 0);
		break;
	    case TYPE_ROOM:
		if (Typeof(dest) != TYPE_ROOM)
		    abort_interp("Bad destination.");
		if (victim == GLOBAL_ENVIRONMENT)
		    abort_interp("Permission denied.");
		if (dest == HOME) {
		    dest = GLOBAL_ENVIRONMENT;
		} else {
		    if ((mlev < 3) && (!permissions(ProgUID, victim)
			    || !can_link_to(ProgUID, NOTYPE, dest)))
			abort_interp("Permission denied.");
		    if (parent_loop_check(victim, dest)) {
			abort_interp("Parent room would create a loop.");
		    }
		}
		ts_lastuseobject(victim);
		moveto(victim, dest);
		break;
	    default:
		abort_interp("Invalid object type (1)");
	}
    }
    CLEAR(oper1);
    CLEAR(oper2);
}

void 
prim_pennies(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (!valid_object(oper1))
	abort_interp("Invalid argument.");
    CHECKREMOTE(oper1->data.objref);
    switch (Typeof(oper1->data.objref)) {
	case TYPE_PLAYER:
	    result = DBFETCH(oper1->data.objref)->sp.player.pennies;
	    break;
	case TYPE_THING:
	    result = DBFETCH(oper1->data.objref)->sp.thing.value;
	    break;
	default:
	    abort_interp("Invalid argument.");
    }
    CLEAR(oper1);
    PushInt(result);
}


void 
prim_dbcomp(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_OBJECT || oper2->type != PROG_OBJECT)
	abort_interp("Invalid argument type.");
    result = oper1->data.objref == oper2->data.objref;
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void 
prim_dbref(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Non-integer argument.");
    ref = (dbref) oper1->data.number;
    CLEAR(oper1);
    PushObject(ref);
}

void 
prim_contents(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (!valid_object(oper1))
	abort_interp("Invalid argument type.");
    CHECKREMOTE(oper1->data.objref);
    ref = DBFETCH(oper1->data.objref)->contents;
    while (mlev < 2 && ref != NOTHING &&
	    (FLAGS(ref) & DARK) && !controls(ProgUID, ref))
	ref = DBFETCH(ref)->next;
/*    if (Typeof(oper1->data.objref) != TYPE_PLAYER &&
	    Typeof(oper1->data.objref) != TYPE_PROGRAM)
	ts_lastuseobject(oper1->data.objref);
 */
    CLEAR(oper1);
    PushObject(ref);
}

void 
prim_exits(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (!valid_object(oper1))
	abort_interp("Invalid object.");
    ref = oper1->data.objref;
    CHECKREMOTE(ref);
    if ((mlev < 3) && !permissions(ProgUID, ref))
	abort_interp("Permission denied.");
    switch (Typeof(ref)) {
	case TYPE_ROOM:
	case TYPE_THING:
	    /* ts_lastuseobject(ref); */
	case TYPE_PLAYER:
	    ref = DBFETCH(ref)->exits;
	    break;
	default:
	    abort_interp("Invalid object.");
    }
    CLEAR(oper1);
    PushObject(ref);
}


void 
prim_next(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (!valid_object(oper1))
	abort_interp("Invalid object.");
    CHECKREMOTE(oper1->data.objref);
    ref = DBFETCH(oper1->data.objref)->next;
    while (mlev < 2 && ref != NOTHING && Typeof(ref) != TYPE_EXIT &&
	    ((FLAGS(ref) & DARK) || Typeof(ref) == TYPE_ROOM) &&
	    !controls(ProgUID, ref))
	ref = DBFETCH(ref)->next;
    CLEAR(oper1);
    PushObject(ref);
}

void 
prim_truename(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (!valid_object(oper1))
	abort_interp("Invalid argument type.");
    ref = oper1->data.objref;
    CHECKREMOTE(ref);
/*    if ((Typeof(ref) != TYPE_PLAYER) && (Typeof(ref) != TYPE_PROGRAM))
	ts_lastuseobject(ref); */
    if (NAME(ref)) {
	strcpy(buf, NAME(ref));
    } else {
	buf[0] = '\0';
    }
    CLEAR(oper1);
    PushString(buf);
}

void 
prim_name(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (!valid_object(oper1))
	abort_interp("Invalid argument type.");
    ref = oper1->data.objref;
    CHECKREMOTE(ref);
/*    if ((Typeof(ref) != TYPE_PLAYER) && (Typeof(ref) != TYPE_PROGRAM))
	ts_lastuseobject(ref); */
    if (NAME(ref)) {
	strcpy(buf, PNAME(ref));
    } else {
	buf[0] = '\0';
    }
    CLEAR(oper1);
    PushString(buf);
}

void 
prim_setname(PRIM_PROTOTYPE)
{
    char *password;
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!valid_object(oper2))
	abort_interp("Invalid argument type (1)");
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    ref = oper2->data.objref;
    if ((mlev < 4) && !permissions(ProgUID, ref))
	abort_interp("Permission denied.");
    {
	const char *b = DoNullInd(oper1->data.string);

	if (Typeof(ref) == TYPE_PLAYER) {
	    strcpy(buf, b);
	    b = buf;
	    if (mlev < 4)
		abort_interp("Permission denied.");
	    /* split off password */
	    for (password = buf;
		    *password && !isspace(*password);
		    password++);
	    /* eat whitespace */
	    if (*password) {
		*password++ = '\0';	/* terminate name */
		while (*password && isspace(*password))
		    password++;
	    }
	    /* check for null password */
	    if (!*password) {
		abort_interp("Player namechange requires password.");
	    } else if (strcmp(password, DoNull(DBFETCH(ref)->sp.player.password))) {
		abort_interp("Incorrect password.");
	    } else if (string_compare(b, NAME(ref))
		       && !ok_player_name(b)) {
		abort_interp("You can't give a player that name.");
	    }
	    /* everything ok, notify */
	    log_status("NAME CHANGE (MUF): %s(#%d) to %s\n",
		       NAME(ref), ref, b);
	    delete_player(ref);
	    if (NAME(ref))
		free((void *) NAME(ref));
	    ts_modifyobject(ref);
	    NAME(ref) = alloc_string(b);
	    add_player(ref);
	} else {
	    if (!ok_name(b))
		abort_interp("Invalid name.");
	    if (NAME(ref))
		free((void *) NAME(ref));
	    NAME(ref) = alloc_string(b);
	    ts_modifyobject(ref);
	    if (MLevRaw(ref))
		SetMLevel(ref, 0);
	}
    }
    CLEAR(oper1);
    CLEAR(oper2);
}

void 
prim_match(PRIM_PROTOTYPE)
{
    struct inst *oper1 /* , *oper2, *oper3, *oper4 */;
    //struct inst temp1, temp2, temp3;
    //int tmp, result;
    dbref ref;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (!oper1->data.string)
	abort_interp("Empty string argument.");
    {
	char    tmppp[BUFFER_LEN];
	struct match_data md;

	(void) strcpy(buf, match_args);
	(void) strcpy(tmppp, match_cmdname);
	init_match(player, oper1->data.string->data, NOTYPE, &md);
	if (oper1->data.string->data[0] == REGISTERED_TOKEN) {
	    match_registered(&md);
	} else {
	    match_all_exits(&md);
	    match_neighbor(&md);
	    match_possession(&md);
	    match_me(&md);
	    match_here(&md);
	    match_home(&md);
	}
	if (Wizard(ProgUID) || (mlev >= 4)) {
	    match_absolute(&md);
	    match_player(&md);
	}
	ref = match_result(&md);
	(void) strcpy(match_args, buf);
	(void) strcpy(match_cmdname, tmppp);
    }
    CLEAR(oper1);
    PushObject(ref);
}


void 
prim_rmatch(PRIM_PROTOTYPE)
{
    struct inst *oper1, *oper2 /* , *oper3, *oper4 */;
    //struct inst temp1, temp2, temp3;
    //int tmp, result;
    dbref ref;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument (2)");
    if (oper2->type != PROG_OBJECT
	    || oper2->data.objref < 0
	    || oper2->data.objref >= db_top
	    || Typeof(oper2->data.objref) == TYPE_PROGRAM
	    || Typeof(oper2->data.objref) == TYPE_EXIT)
	abort_interp("Invalid argument (1)");
    CHECKREMOTE(oper2->data.objref);
    {
	char    tmppp[BUFFER_LEN];
	struct match_data md;

	(void) strcpy(buf, match_args);
	(void) strcpy(tmppp, match_cmdname);
	init_match(player, DoNullInd(oper1->data.string), TYPE_THING, &md);
	match_rmatch(oper2->data.objref, &md);
	ref = match_result(&md);
	(void) strcpy(match_args, buf);
	(void) strcpy(match_cmdname, tmppp);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushObject(ref);
}


void 
prim_copyobj(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (!valid_object(oper1))
	abort_interp("Invalid object.");
    CHECKREMOTE(oper1->data.objref);
    if ((mlev < 3) && (fr->already_created))
	abort_interp("Can't create any more objects.");
    ref = oper1->data.objref;
    if (Typeof(ref) != TYPE_THING)
	abort_interp("Invalid object type.");
    if ((mlev < 3) && !permissions(ProgUID, ref))
	abort_interp("Permission denied.");
    fr->already_created++;
    {
	dbref   newobj;

	newobj = new_object();
	*DBFETCH(newobj) = *DBFETCH(ref);
	copyobj(player, ref, newobj);
	CLEAR(oper1);
	PushObject(newobj);
    }
}


void 
prim_set(PRIM_PROTOTYPE)
/* SET */
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument type (2)");
    if (!(oper1->data.string))
	abort_interp("Empty string argument (2)");
    if (!valid_object(oper2))
	abort_interp("Invalid object.");
    ref = oper2->data.objref;
    CHECKREMOTE(ref);
    tmp = 0;
    result = (*oper1->data.string->data == '!');
    {
	char   *flag = oper1->data.string->data;

	if (result)
	    flag++;

	if (string_prefix("dark", flag)
		|| string_prefix("debug", flag))
	    tmp = DARK;
	else if (string_prefix("abode", flag)
                || string_prefix("autostart", flag)
                || string_prefix("abate", flag))
	    tmp = ABODE;
	else if (string_prefix("chown_ok", flag))
	    tmp = CHOWN_OK;
	else if (string_prefix("haven", flag)
		 || string_prefix("harduid", flag))
	    tmp = HAVEN;
	else if (string_prefix("jump_ok", flag))
	    tmp = JUMP_OK;
	else if (string_prefix("link_ok", flag))
	    tmp = LINK_OK;

	else if (string_prefix("kill_ok", flag))
	    tmp = KILL_OK;

	else if (string_prefix("builder", flag))
	    tmp = BUILDER;
	else if (string_prefix("mucker", flag))
	    tmp = MUCKER;
	else if (string_prefix("nucker", flag))
	    tmp = SMUCKER;
	else if (string_prefix("interactive", flag))
	    tmp = INTERACTIVE;
	else if (string_prefix("sticky", flag)
		 || string_prefix("silent", flag))
	    tmp = STICKY;
	else if (string_prefix("wizard", flag))
	    tmp = WIZARD;
	else if (string_prefix("truewizard", flag))
	    tmp = WIZARD;
	else if (string_prefix("xforcible", flag))
	    tmp = XFORCIBLE;
	else if (string_prefix("zombie", flag))
	    tmp = ZOMBIE;
	else if (string_prefix("vehicle", flag))
	    tmp = VEHICLE;
	else if (string_prefix("quell", flag))
	    tmp = QUELL;
    }
    if (!tmp)
	abort_interp("Unrecognized flag.");
    if ((mlev < 4) && !permissions(ProgUID, ref))
	abort_interp("Permission denied.");

    if (((mlev < 4) && ((tmp == DARK && ((Typeof(ref) == TYPE_PLAYER)
			   || (!tp_exit_darking && Typeof(ref) == TYPE_EXIT)
			   || (!tp_thing_darking && Typeof(ref) == TYPE_THING)
					)
		        )
			|| ((tmp == ZOMBIE) && (Typeof(ref) == TYPE_THING)
			    && (FLAGS(ProgUID) & ZOMBIE))
			|| ((tmp == ZOMBIE) && (Typeof(ref) == TYPE_PLAYER))
			|| (tmp == BUILDER)
		    )
	    )
	    || (tmp == WIZARD) || (tmp == QUELL) || (tmp == INTERACTIVE)
	    || ((tmp == ABODE) && (Typeof(ref) == TYPE_PROGRAM))
	    || (tmp == MUCKER) || (tmp == SMUCKER) || (tmp == XFORCIBLE)
	    )
	abort_interp("Permission denied.");
    if (result && Typeof(ref) == TYPE_THING) {
	dbref obj = DBFETCH(ref)->contents;
	for (; obj != NOTHING; obj = DBFETCH(obj)->next) {
	    if (Typeof(obj) == TYPE_PLAYER) {
		abort_interp("Permission denied.");
	    }
	}
    }
    if (!result) {
	FLAGS(ref) |= tmp;
	DBDIRTY(ref);
    } else {
	FLAGS(ref) &= ~tmp;
	DBDIRTY(ref);
    }
    CLEAR(oper1);
    CLEAR(oper2);
}

void 
prim_mlevel(PRIM_PROTOTYPE)
/* MLEVEL */
{
    CHECKOP(1);
    oper1 = POP();
    if (!valid_object(oper1))
	abort_interp("Invalid object.");
    ref = oper1->data.objref;
    CHECKREMOTE(ref);
    result = MLevRaw(ref);
    CLEAR(oper1);
    PushInt(result);
}

void 
prim_flagp(PRIM_PROTOTYPE)
/* FLAG? */
{
    int     truwiz = 0;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument type (2)");
    if (!(oper1->data.string))
	abort_interp("Empty string argument (2)");
    if (!valid_object(oper2))
	abort_interp("Invalid object.");
    ref = oper2->data.objref;
    CHECKREMOTE(ref);
    tmp = 0;
    result = 0;
    {
	char   *flag = oper1->data.string->data;

	while (*flag == '!') {
	    flag++;
	    result = (!result);
	}
	if (!*flag)
	    abort_interp("Unknown flag.");

	if (string_prefix("dark", flag)
		|| string_prefix("debug", flag))
	    tmp = DARK;
	else if (string_prefix("abode", flag)
                || string_prefix("autostart", flag)
                || string_prefix("abate", flag))
	    tmp = ABODE;
	else if (string_prefix("chown_ok", flag))
	    tmp = CHOWN_OK;
	else if (string_prefix("haven", flag)
		 || string_prefix("harduid", flag))
	    tmp = HAVEN;
	else if (string_prefix("jump_ok", flag))
	    tmp = JUMP_OK;
	else if (string_prefix("link_ok", flag))
	    tmp = LINK_OK;

	else if (string_prefix("kill_ok", flag))
	    tmp = KILL_OK;

	else if (string_prefix("builder", flag))
	    tmp = BUILDER;
	else if (string_prefix("mucker", flag))
	    tmp = MUCKER;
	else if (string_prefix("nucker", flag))
	    tmp = SMUCKER;
	else if (string_prefix("interactive", flag))
	    tmp = INTERACTIVE;
	else if (string_prefix("sticky", flag)
		 || string_prefix("silent", flag))
	    tmp = STICKY;
	else if (string_prefix("wizard", flag))
	    tmp = WIZARD;
	else if (string_prefix("truewizard", flag)) {
	    tmp = WIZARD;
	    truwiz = 1;
	} else if (string_prefix("zombie", flag))
	    tmp = ZOMBIE;
	else if (string_prefix("xforcible", flag))
	    tmp = XFORCIBLE;
	else if (string_prefix("vehicle", flag))
	    tmp = VEHICLE;
	else if (string_prefix("quell", flag))
	    tmp = QUELL;
    }
    if (result) {
	if ((!truwiz) && (tmp == WIZARD)) {
	    result = (!Wizard(ref));
	} else {
	    result = (tmp && ((FLAGS(ref) & tmp) == 0));
	}
    } else {
	if ((!truwiz) && (tmp == WIZARD)) {
	    result = Wizard(ref);
	} else {
	    result = (tmp && ((FLAGS(ref) & tmp) != 0));
	}
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}


void 
prim_playerp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_OBJECT)
	abort_interp("Invalid argument type.");
    if (!valid_object(oper1) && !is_home(oper1)) {
	result = 0;
    } else {
	ref = oper1->data.objref;
	CHECKREMOTE(ref);
	result = (Typeof(ref) == TYPE_PLAYER);
    }
    PushInt(result);
}


void 
prim_thingp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_OBJECT)
	abort_interp("Invalid argument type.");
    if (!valid_object(oper1) && !is_home(oper1)) {
	result = 0;
    } else {
	ref = oper1->data.objref;
	CHECKREMOTE(ref);
	result = (Typeof(ref) == TYPE_THING);
    }
    PushInt(result);
}


void 
prim_roomp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_OBJECT)
	abort_interp("Invalid argument type.");
    if (!valid_object(oper1) && !is_home(oper1)) {
	result = 0;
    } else {
	ref = oper1->data.objref;
	CHECKREMOTE(ref);
	result = (Typeof(ref) == TYPE_ROOM);
    }
    PushInt(result);
}


void 
prim_programp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_OBJECT)
	abort_interp("Invalid argument type.");
    if (!valid_object(oper1) && !is_home(oper1)) {
	result = 0;
    } else {
	ref = oper1->data.objref;
	CHECKREMOTE(ref);
	result = (Typeof(ref) == TYPE_PROGRAM);
    }
    PushInt(result);
}


void 
prim_exitp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_OBJECT)
	abort_interp("Invalid argument type.");
    if (!valid_object(oper1) && !is_home(oper1)) {
	result = 0;
    } else {
	ref = oper1->data.objref;
	CHECKREMOTE(ref);
	result = (Typeof(ref) == TYPE_EXIT);
    }
    PushInt(result);
}


void 
prim_okp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    result = (valid_object(oper1));
    CLEAR(oper1);
    PushInt(result);
}

void 
prim_location(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (!valid_object(oper1))
	abort_interp("Invalid object.");
    CHECKREMOTE(oper1->data.objref);
    ref = DBFETCH(oper1->data.objref)->location;
    CLEAR(oper1);
    PushObject(ref);
}

void 
prim_owner(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (!valid_object(oper1))
	abort_interp("Invalid object.");
    CHECKREMOTE(oper1->data.objref);
    ref = OWNER(oper1->data.objref);
    CLEAR(oper1);
    PushObject(ref);
}

void 
prim_controls(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!valid_object(oper1))
	abort_interp("Invalid object. (2)");
    if (!valid_object(oper2))
	abort_interp("Invalid object. (1)");
    CHECKREMOTE(oper1->data.objref);
    result = controls(oper2->data.objref, oper1->data.objref);
    CLEAR(oper1);
    PushInt(result);
}

void 
prim_getlink(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (!valid_object(oper1))
	abort_interp("Invalid object.");
    CHECKREMOTE(oper1->data.objref);
    if (Typeof(oper1->data.objref) == TYPE_PROGRAM)
	abort_interp("Illegal object referenced.");
    switch (Typeof(oper1->data.objref)) {
	case TYPE_EXIT:
	    ref = (DBFETCH(oper1->data.objref)->sp.exit.ndest) ?
		(DBFETCH(oper1->data.objref)->sp.exit.dest)[0] : NOTHING;
	    break;
	case TYPE_PLAYER:
	    ref = DBFETCH(oper1->data.objref)->sp.player.home;
	    break;
	case TYPE_THING:
	    ref = DBFETCH(oper1->data.objref)->sp.thing.home;
	    break;
	case TYPE_ROOM:
	    ref = DBFETCH(oper1->data.objref)->sp.room.dropto;
	    break;
	default:
	    ref = NOTHING;
	    break;
    }
    CLEAR(oper1);
    PushObject(ref);
}

int 
prog_can_link_to(int mlev, dbref who, object_flag_type what_type, dbref where)
{
    if (where == HOME)
	return 1;
    if (where < 0 || where > db_top)
	return 0;
    switch (what_type) {
	case TYPE_EXIT:
	    return (mlev > 3 || permissions(who, where) || (FLAGS(where) & LINK_OK));
	    /* NOTREACHED */
	    break;
	case TYPE_PLAYER:
	    return (Typeof(where) == TYPE_ROOM && (mlev > 3 || permissions(who, where)
						   || Linkable(where)));
	    /* NOTREACHED */
	    break;
	case TYPE_ROOM:
	    return ((Typeof(where) == TYPE_ROOM)
		    && (mlev > 3 || permissions(who, where) || Linkable(where)));
	    /* NOTREACHED */
	    break;
	case TYPE_THING:
	    return ((Typeof(where) == TYPE_ROOM || Typeof(where) == TYPE_PLAYER)
		    && (mlev > 3 || permissions(who, where) || Linkable(where)));
	    /* NOTREACHED */
	    break;
	case NOTYPE:
	    return (mlev > 3 || permissions(who, where) || (FLAGS(where) & LINK_OK) ||
		    (Typeof(where) != TYPE_THING && (FLAGS(where) & ABODE)));
	    /* NOTREACHED */
	    break;
    }
    return 0;
}


void 
prim_setlink(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();		/* dbref: destination */
    oper2 = POP();		/* dbref: source */
    if ((oper1->type != PROG_OBJECT) || (oper2->type != PROG_OBJECT))
	abort_interp("setlink requires two dbrefs.");
    if (!valid_object(oper2) && oper2->data.objref != HOME)
	abort_interp("Invalid object. (1)");
    ref = oper2->data.objref;
    if (oper1->data.objref == -1) {
	if ((mlev < 4) && !permissions(ProgUID, ref))
	    abort_interp("Permission denied.");
	switch (Typeof(ref)) {
	    case TYPE_EXIT:
		DBSTORE(ref, sp.exit.ndest, 0);
		if (DBFETCH(ref)->sp.exit.dest) {
		    free((void *) DBFETCH(ref)->sp.exit.dest);
		    DBSTORE(ref, sp.exit.dest, NULL);
		}
		if (MLevRaw(ref))
		    SetMLevel(ref, 0);
		FLAGS(ref) &= ~ZONE; // Remove zone flag on Unlink
		break;
	    case TYPE_ROOM:
		DBSTORE(ref, sp.room.dropto, NOTHING);
		break;
	    default:
		abort_interp("Invalid object. (1)");
	}
    } else {
	if (!valid_object(oper1))
	    abort_interp("Invalid object. (2)");
	if (Typeof(ref) == TYPE_PROGRAM)
	    abort_interp("Program objects are not linkable. (1)");
	if (!prog_can_link_to(mlev, ProgUID, Typeof(ref), oper1->data.objref))
	    abort_interp("Can't link source to destination.");
	switch (Typeof(ref)) {
	    case TYPE_EXIT:
		if (DBFETCH(ref)->sp.exit.ndest != 0) {
		    if ((mlev < 4) && !permissions(ProgUID, ref))
			abort_interp("Permission denied.");
		    abort_interp("Exit is already linked.");
		}
                if (exit_loop_check(ref, oper1->data.objref))
		    abort_interp("Link would cause a loop.");
		DBFETCH(ref)->sp.exit.ndest = 1;
		DBFETCH(ref)->sp.exit.dest = (dbref *) malloc(sizeof(dbref));
		(DBFETCH(ref)->sp.exit.dest)[0] = oper1->data.objref;
		break;
	    case TYPE_PLAYER:
		if ((mlev < 4) && !permissions(ProgUID, ref))
		    abort_interp("Permission denied.");
		DBFETCH(ref)->sp.player.home = oper1->data.objref;
		break;
	    case TYPE_THING:
		if ((mlev < 4) && !permissions(ProgUID, ref))
		    abort_interp("Permission denied.");
		if (parent_loop_check(ref, oper1->data.objref))
		    abort_interp("That would cause a parent paradox.");
		DBFETCH(ref)->sp.thing.home = oper1->data.objref;
		break;
	    case TYPE_ROOM:
		if ((mlev < 4) && !permissions(ProgUID, ref))
		    abort_interp("Permission denied.");
		DBFETCH(ref)->sp.room.dropto = oper1->data.objref;
		break;
	}
    }
    CLEAR(oper1);
    CLEAR(oper2);
}

void 
prim_setown(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();		/* dbref: new owner */
    oper2 = POP();		/* dbref: what */
    if (!valid_object(oper2))
	abort_interp("Invalid argument (1)");
    if (!valid_player(oper1))
	abort_interp("Invalid argument (2)");
    ref = oper2->data.objref;
    if ((mlev < 4) && oper1->data.objref != player)
	abort_interp("Permission denied. (2)");
    if ((mlev < 4) && (!(FLAGS(ref) & CHOWN_OK) ||
			!test_lock(player, ref, "_/chlk")))
	abort_interp("Permission denied. (1)");
    switch (Typeof(ref)) {
	case TYPE_ROOM:
	    if ((mlev < 4) && DBFETCH(player)->location != ref)
		abort_interp("Permission denied: not in room. (1)");
	    break;
	case TYPE_THING:
	    if ((mlev < 4) && DBFETCH(ref)->location != player)
		abort_interp("Permission denied: object not carried. (1)");
	    break;
	case TYPE_PLAYER:
	    abort_interp("Permission denied: cannot set owner of player. (1)");
	case TYPE_EXIT:
	case TYPE_PROGRAM:
	    break;
	case TYPE_GARBAGE:
	    abort_interp("Permission denied: who would want to own garbage? (1)");
    }
    OWNER(ref) = OWNER(oper1->data.objref);
    DBDIRTY(ref);
}

void 
prim_newobject(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();		/* string: name */
    oper2 = POP();		/* dbref: location */
    if ((mlev < 3) && (fr->already_created))
	abort_interp("An object was already created this program run.");
    CHECKOFLOW(1);
    ref = oper2->data.objref;
    if (!valid_object(oper2) || (!valid_player(oper2) && (Typeof(ref) != TYPE_ROOM)))
	abort_interp("Invalid argument (1)");
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument (2)");
    CHECKREMOTE(ref);
    if ((mlev < 3) && !permissions(ProgUID, ref))
	abort_interp("Permission denied.");
    {
	const char *b = DoNullInd(oper1->data.string);
	dbref   loc;

	if (!ok_name(b))
	    abort_interp("Invalid name. (2)");

	ref = new_object();

	/* initialize everything */
	NAME(ref) = alloc_string(b);
	DBFETCH(ref)->location = oper2->data.objref;
	OWNER(ref) = OWNER(ProgUID);
	DBFETCH(ref)->sp.thing.value = 1;
	DBFETCH(ref)->exits = NOTHING;
	FLAGS(ref) = TYPE_THING;

	if ((loc = DBFETCH(player)->location) != NOTHING
		&& controls(player, loc)) {
	    DBFETCH(ref)->sp.thing.home = loc;	/* home */
	} else {
	    DBFETCH(ref)->sp.thing.home = DBFETCH(player)->sp.player.home;
	    /* set to player's home instead */
	}
    }

    /* link it in */
    PUSH(ref, DBFETCH(oper2->data.objref)->contents);
    DBDIRTY(oper2->data.objref);

    CLEAR(oper1);
    CLEAR(oper2);
    PushObject(ref);
}

void 
prim_newroom(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();		/* string: name */
    oper2 = POP();		/* dbref: location */
    if ((mlev < 3) && (fr->already_created))
	abort_interp("An object was already created this program run.");
    CHECKOFLOW(1);
    ref = oper2->data.objref;
    if (!valid_object(oper2) || (Typeof(ref) != TYPE_ROOM))
	abort_interp("Invalid argument (1)");
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument (2)");
    if ((mlev < 3) && !permissions(ProgUID, ref))
	abort_interp("Permission denied.");
    {
	const char *b = DoNullInd(oper1->data.string);

	if (!ok_name(b))
	    abort_interp("Invalid name. (2)");

	ref = new_object();

	/* Initialize everything */
	NAME(ref) = alloc_string(b);
	DBFETCH(ref)->location = oper2->data.objref;
	OWNER(ref) = OWNER(ProgUID);
	DBFETCH(ref)->exits = NOTHING;
	DBFETCH(ref)->sp.room.dropto = NOTHING;
	FLAGS(ref) = TYPE_ROOM | (FLAGS(player) & JUMP_OK);
	PUSH(ref, DBFETCH(oper2->data.objref)->contents);
	DBDIRTY(ref);
	DBDIRTY(oper2->data.objref);

	CLEAR(oper1);
	CLEAR(oper2);
	PushObject(ref);
    }
}

void 
prim_newexit(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();		/* string: name */
    oper2 = POP();		/* dbref: location */
    if (mlev < 3)
	abort_interp("Requires Mucker Level 3.");
    CHECKOFLOW(1);
    ref = oper2->data.objref;
    if (!valid_object(oper2) || ((!valid_player(oper2)) && (Typeof(ref) != TYPE_ROOM) && (Typeof(ref) != TYPE_THING)))
	abort_interp("Invalid argument (1)");
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument (2)");
    CHECKREMOTE(ref);
    if ((mlev < 4) && !permissions(ProgUID, ref))
	abort_interp("Permission denied.");
    {
	const char *b = DoNullInd(oper1->data.string);

	if (!ok_name(b))
	    abort_interp("Invalid name. (2)");

	ref = new_object();

	/* initialize everything */
	NAME(ref) = alloc_string(oper1->data.string->data);
	DBFETCH(ref)->location = oper2->data.objref;
	OWNER(ref) = OWNER(ProgUID);
	FLAGS(ref) = TYPE_EXIT;
	DBFETCH(ref)->sp.exit.ndest = 0;
	DBFETCH(ref)->sp.exit.dest = NULL;

	/* link it in */
	PUSH(ref, DBFETCH(oper2->data.objref)->exits);
	DBDIRTY(oper2->data.objref);

	CLEAR(oper1);
	CLEAR(oper2);
	PushObject(ref);
    }
}


void 
prim_lockedp(PRIM_PROTOTYPE)
{
    struct inst *oper1, *oper2;

    /* d d - i */
    CHECKOP(2);
    oper1 = POP();		/* objdbref */
    oper2 = POP();		/* player dbref */
    if (fr->level > 8)
	abort_interp("Interp call loops not allowed.");
    if (!valid_object(oper2))
	abort_interp("invalid object (1).");
    if (!valid_player(oper2) && Typeof(oper2->data.objref) != TYPE_THING)
	abort_interp("Non-player argument (1).");
    CHECKREMOTE(oper2->data.objref);
    if (!valid_object(oper1))
	abort_interp("invalid object (2).");
    CHECKREMOTE(oper1->data.objref);
    result = !could_doit(oper2->data.objref, oper1->data.objref);
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}


void 
prim_recycle(PRIM_PROTOTYPE)
{
    /* d -- */
    CHECKOP(1);
    oper1 = POP();		/* object dbref to recycle */
    if (oper1->type != PROG_OBJECT)
	abort_interp("Non-object argument (1).");
    if (!valid_object(oper1))
	abort_interp("Invalid object (1).");
    result = oper1->data.objref;
    if ((mlev < 3) || ((mlev < 4) && !permissions(ProgUID, result)))
	abort_interp("Permission denied.");
    if ((result == tp_player_start) || (result == GLOBAL_ENVIRONMENT))
	abort_interp("Cannot recycle that room.");
    if (Typeof(result) == TYPE_PLAYER)
	abort_interp("Cannot recycle a player.");
    if (result == program)
	abort_interp("Cannot recycle currently running program.");
    {
	int     ii;

	for (ii = 0; ii < fr->caller.top; ii++)
	    if (fr->caller.st[ii] == result)
		abort_interp("Cannot recycle active program.");
    }
    if (Typeof(result) == TYPE_EXIT)
	if (!unset_source(player, DBFETCH(player)->location, result))
	    abort_interp("Cannot recycle old style exits.");
    CLEAR(oper1);
    recycle(player, result);
}


void 
prim_setlockstr(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!valid_object(oper2))
	abort_interp("Invalid argument type (1)");
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    ref = oper2->data.objref;
    if ((mlev < 4) && !permissions(ProgUID, ref))
	abort_interp("Permission denied.");
    result = setlockstr(player, ref,
		oper1->data.string ? oper1->data.string->data : (char *) "");
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}


void 
prim_getlockstr(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (!valid_object(oper1))
	abort_interp("Invalid argument type");
    ref = oper1->data.objref;
    CHECKREMOTE(ref);
    if ((mlev < 3) && !permissions(ProgUID, ref))
	abort_interp("Permission denied.");
    {
	char   *tmpstr;

	tmpstr = (char *) unparse_boolexp(player, GETLOCK(ref), 0);
	CLEAR(oper1);
	PushString(tmpstr);
    }
}


void 
prim_part_pmatch(PRIM_PROTOTYPE)
{
    dbref ref;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (!oper1->data.string)
	abort_interp("Empty string argument.");
    if (mlev < 3)
	abort_interp("Permission denied.  Requires Mucker Level 3.");
    ref = partial_pmatch(oper1->data.string->data);
    CLEAR(oper1);
    PushObject(ref);
}


void
prim_checkpassword(PRIM_PROTOTYPE)
{
    char *ptr;

    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();

    if (mlev < 4)
       abort_interp("Requires Wizbit.");
    if (oper1->type != PROG_OBJECT)
       abort_interp("Player dbref expected. (1)");
    ref = oper1->data.objref;
    if ((ref != NOTHING && !valid_player(oper1)) || ref == NOTHING)
       abort_interp("Player dbref expected. (1)");
    if (oper2->type != PROG_STRING)
       abort_interp("Password string expected. (2)");
    ptr = oper2->data.string? oper2->data.string->data : "";
    if (ref != NOTHING && !strcmp(ptr, DBFETCH(ref)->sp.player.password))
       result=1;
    else
       result=0;

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}


