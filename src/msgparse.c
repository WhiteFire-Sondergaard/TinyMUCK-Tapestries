/* TO.DO list:
 *   {ldelete, {lreplace, {linsert,
 *   {create,  {action,   {dig,
 *   {case,    {hasflag,
 *
 *   Make dbref handling funs handle HOME.
 *   Fix problems with {muf:} being used with MPI _listeners.
 *
 */

/* Triggering ideas:
 *    _verbs/ and _overbs/
 */


/* do_parse_mesg()  Parses a message string, expanding {prop}s, {list}s, etc.
 * Written 4/93 - 5/93 by Foxen
 *
 * Args:
 *   dbref player          Player triggering this.
 *   dbref what            Object that mesg is on.
 *   dbref perms           Object permissions are checked from.
 *   const char *inbuf     A pointer to the input raw mesg string.
 *   char *abuf            Argument string for {&how}.
 *   char *outbuf          Buffer to return results in.
 *   int mesgtyp           1 for personal messages, 0 for omessages.
 *
 *   extern char *match_args     Arg string for {&args}
 *   extern char *match_cmdname  Arg string for {&cmd}
 *
 * Returns a pointer to output string.  Usually in outbuf.
 */


/* Comments:
 *     {prop:propname,obj}     Replaced with str value of given prop on obj.
 *     {prop:propname}         Same, but looks down trigger's env for prop.
 *     {exec:propname,obj}     Like {prop:propname,obj}, but evaluates result.
 *     {exec:propname}         Like {prop:propname}, but evaluates result.
 *     {index:propname,obj}    Basically {sub:{sub:obj,propname}}
 *     {index:propname}        Same as {sub:{sub:propname}}
 *     {rand:listname,obj}     Replaced by random element of given list.
 *     {rand:listname}         Same, but looks down trigger's env for list;
 *     {list:listname,obj}     Replaced by listname#/ list on obj.
 *     {list:listname}         Same, but looks down trigger's env for list.
 *     {lexec:listname,obj}    Eval list on obj, stripped, w/ obj perms.
 *     {lexec:listname}        Same, but looks down trigger's env for list.
 *     {concat:listname,obj}   Replaced by listname#/ list on obj.
 *     {concat:listname}       Same, but looks down trigger's env for list.
 *     {count:list}            Returns # of \r delimited lines in list.
 *     {count:list,sep}        Returns # of sep delimited lines in list.
 *
 *     {dice:X,Y,Z}            Returns random # as if rolled on YdX+Z dice.
 *     {dice:X,Y}              Returns random # as if rolled on YdX dice.
 *     {dice:X}                Returns random # between 1 and X.
 *     {nl}  or  \r            Break the line here and start a new line.
 *     {subst:str,old,new}     Replaces occurrences of old with new in str.
 *     {strlen:string}         Returns how long string is.
 *     {smatch:str,pattern}    Returns true if str matches wildcard pattern.
 *     {strip:string}          Strips spaces from head and tail of string.
 *     {tolower:string}        Lowercases a string.
 *     {toupper:string}        Uppercases a string.
 *     {instr:str1,str2}       Returns position of str2 within str1, else 0.
 *     {midstr:str,pos}        Returns character at position pos in str.
 *     {midstr:str,pos1,pos2}  Returns substring in str from pos1 to pos2.
 *     {lit:string}            Exactly returns string without evaluating it.
 *     {eval:string}           Evaluates the given string.
 *     {pronouns:string}       Does pronoun subst on string for player.
 *     {pronouns:string,obj}   Does pronoun subst on string for obj.
 *
 *     {inc:var}               Adds 1 to value of var.
 *     {dec:var}               Subtracts 1 from value of var.
 *     {add:expr1,expr2}       Adds the values of expr1 and expr2.
 *     {subt:expr1,expr2}      Subtracts the value of expr2 from expr1.
 *     {mult:expr1,expr2}      Multiplies the value of expr1 to expr2.
 *     {div:expr1,expr2}       Divides the value of expr1 by expr2.
 *     {mod:expr1,expr2}       Returns the modula of expr1 divided by expr2.
 *     {min:expr1,expr2}       Returns the lesser of expr1 and expr2.
 *     {max:expr1,expr2}       Returns the greater of expr1 and expr2.
 *     {abs:expr}              Returns the absolute value of expr.
 *     {sign:expr}             Returns sign of expr.  -1, 0, or 1.
 *     {sqr:expr}              Returns the square of expr.
 *     {sqrt:expr}             Returns the square root of expr.
 *     {dist:x,y}              Returns distance from 0,0 that x,y is.
 *     {dist:x,y,z}            Returns distance from 0,0,0 that x,y,z is.
 *     {dist:x,y,x2,y2}        Returns distance between x,y and x2,y2.
 *     {dist:x,y,z,x2,y2,z2}   Returns distance between x,y,z and x2,y2,z2.
 *
 *     {ref:obj}               Converts matching objname to dbref.
 *     {name:obj}              Gives first name of obj if exit, else full name.
 *     {fullname:obj}          Returns full name of obj.
 *     {owner:obj}             Returns owner of obj.
 *     {flags:obj}             Returns a flaglist string from obj. ie: PM2J
 *     {controls:obj}          True if obj is controlled by trig's owner.
 *     {controls:obj,player}   True if obj is controlled by player.
 *     {dbeq:obj1,obj2}        True if obj1 and obj2 refer to same object.
 *     {holds:obj1}            True if location of obj1 is player.
 *     {holds:obj1,obj2}       True if location of obj1 is obj2.
 *     {contains:obj1}         True if obj1 within locational env of player.
 *     {contains:obj1,obj2}    True if obj1 within locational env of obj2.
 *     {loc:obj}               Returns location of obj.
 *     {nearby:obj}            Returns whether obj is nearby to trigger object.
 *     {nearby:obj,obj2}       Returns whether obj is nearby to obj2.
 *     {money:obj}             Returns player's penny count, or thing's value.
 *     {type:obj}              Returns the type name of obj.  ie. "Room"
 *     {istype:obj,typ}        Returns true if obj is of the type typ.
 *     {awake:player}          Returns how many times player is connected.
 *     {contents:obj}          Returns list contents of object, by name.
 *     {contents:obj,type}     Same, but only lists objs of given type.
 *     {exits:obj}             Returns list of exits on object, by name.
 *     {links:obj}             Returns list of dests this obj is linked to.
 *     {force:obj,command}     Forces the given thing to do the given command.
 *     {locked:player,obj}     Tests the _/lok lock on obj against player.
 *     {testlock:obj,prop}     Tests lock prop on given obj against the player.
 *     {testlock:obj,prop,who} Tests lock prop on given obj against who.
 *     {testlock:obj,prop,who,def}  Same, but specifies default if no lock.
 *
 *     {online}                Returns a list of players who are online.  Wiz.
 *     {ontime:player}         Returns player online time in secs.  Fails w/ -1
 *     {idle:player}           Returns player idle time in secs.  Fails with -1
 *
 *     {isnum:number}          Is this a valid floating point number?
 *     {isdbref:dbref}         Is this a valid dbref?
 *     {version}               Returns server version number.
 *     {muckname}              Returns Name of this MUCK.
 *
 *     {v:var}                 Subs in value of given variable var.
 *     {&varname}              Same as {v:varname}
 *     {&1} ... {&9}           Used for argument subst in macros.
 *
 *     {set:var,value}         Sets variable var to value.
 *     {store:val,prop}        Stores val in the given prop on 'this'.
 *     {store:val,prop,obj}    Stores val in the given prop on obj.
 *
 *     {with:var,val,expr1,expr2..} eval exprs w/ var set to val. ret last expr
 *     {foreach:var,list,expr}      eval expr on each \r delim. line in list
 *     {foreach:var,list,expr,sep}  eval expr on each sep delim. line in list
 *     {filter:var,list,expr}       return lines in list where expr evals true
 *     {filter:var,list,exp,sep}    same, but input list lines are sep delimed.
 *     {filter:var,lst,exp,sep,s2}  same, but output list lines are s2 delimed.
 *     {parse:var,list,expr}        replace each list item with eval of expr.
 *     {parse:var,list,expr,sep}    same, but input list lines are sep delimed.
 *     {parse:var,list,exp,sep,s2}  same, but output list lines are s2 delimed.
 *     {fold:var1,var2,list,expr}   eval expr for each consecutive list item.
 *                                   eval of expr is compared against next
 *                                   item, etc.  Last eval of expr returned.
 *     {fold:v1,v2,lst,expr,sep}    Same, but expects list seperated by sep.
 *     {commas:list}                Returns str like "Tom, Dick and Harry."
 *     {commas:list,lastsep}        Same, except uses lastsep for "and".
 *     {commas:list,lsep,var,expr}  Same, but evals each list item with expr.
 *     {lsort:list}                 returns list with elements sorted
 *     {lsort:list,var1,var2,expr}  returns list, sorted via expr compares.
 *
 * In the above looping list expressions, variable var will be defined and
 * set to the value of the current list item under scrutiny.
 *
 *     {mklist:value...}       Returns a list with all given values.
 *     {sublist:list,pos1}     Returns item pos1 of given list
 *     {sublist:lst,pos1,pos2} Returns items between pos1 and pos2, inclusive.
 *     {sublist:lst,p1,p2,sep} Same, except seperates input list at sep marks.
 *
 *     {lunique:list}          returns list with all duplicates removed.
 *     {lunion:list1,list2}    returns all list1 & list2 items, lunique'd.
 *     {lcommon:list1,list2}   returns all items in both list1 & list2.
 *     {lremove:list1,list2}   returns all items in list1 that aren't in list2.
 *
 *
 *     {tzoffset}              Returns local time zone offset in secs from GMT.
 *     {time}                  Returns time str in the 24hr form hh:mm:ss
 *     {time:timezone}         Same, except w/ # hrs timezone offset from GMT.
 *     {date}                  Returns a date str in the form mm/dd/yy
 *     {date:timezone}         Same, except w/ # hrs timezone offset from GMT.
 *     {ftime:format}          Returns special format time/date string.
 *     {ftime:format,tz}       Same, except w/ # hrs timezone offset from GMT.
 *     {ftime:format,tz,secs}  Same as above, except specifies Systime to use.
 *     {timestr:secs}          Gives deltatime like "9d 12:56".
 *     {ltimestr:secs}         Longer: "1 week, 2 days, 12 hours, 56 mins".
 *     {stimestr:secs}         Gives most significant deltatime such as "9d".
 *     {secs}                  Returns systime secs since midnight 1/1/70 GMT
 *     {convtime:string}       Converts "HH:MM:SS Mo/Dy/Yr" string to systime.
 *     {convsecs}              Converts systime to a readable time string.
 *     {delay:secs,expr}       Eval and display expr after given # of secs.
 *
 *     {if:check,true}         Eval 'true' if 'check' is not "" or "0".
 *     {if:check,true,false}   Same.  Eval 'false' if 'check' is "" or "0".
 *     {while:check,expr}      Keeps evaling expr until check evals false.
 *     {eq:expr1,expr2}        Return "1" if exprs eval the same, else "0".
 *     {ne:expr1,expr2}        Return "0" if exprs eval the same, else "1".
 *     {gt:expr1,expr2}        Return "1" if expr1 evals greater than expr2.
 *     {ge:expr1,expr2}        Return "1" if expr1 evals >= expr2.
 *     {lt:expr1,expr2}        Return "1" if expr1 evals less than expr2.
 *     {le:expr1,expr2}        Return "1" if expr1 evals <= expr2.
 *     {not:expr}              Return "1" if expr is false.  Else returns "0".
 *     {or:expr1,expr2}        Return "1" if expr1 or expr2 are true.  Else "0"
 *     {xor:expr1,expr2}       Logical exclusive or of expr1 and expr2.
 *     {and:expr1,expr2}       Return "1" if expr1 and expr2 are both true.
 *
 *     {muf:prog,arg}          Runs MUF prog with string arg on the stack.
 *                              returns top stack value when the prog returns.
 *                              Does not preparse or strip spaces from arg.
 *
 *     {debug:expr}           This shows debugging info for expr.
 *
 *     {<macroname>:<args>}   Is replaced with macro in _msgmacs/ and evaled.
 *
 *     A false value is a null string or a "0".  A true value is non-false.
 *     C style shortcutting is used in {and and {or.  {if only evals the
 *     branch that it returns the value of.  {list and {concat eval each
 *     line within the list.  Math functions {add, {subt, {mult, {div, etc
 *     all use floating point.  Only 26 levels of recursion are allowed, so
 *     functions that deep return literally.  In {while, a max of 100 iter-
 *     ations are allowed before it exits automatically.  {concat and {list
 *     allow lists of a maximum length of 100 lines.  {while returns the
 *     string returned by the last evaluation of expr.  {contents and {exits
 *     will return a max of 100 names in the list.  All matching will be done
 *     relative to the trigger object.  If an {delay evaluation is a null
 *     string, then the notify or notify_except will not be done.
 *
 */

/* <{filter:{exits:here},1,{eq:{exec:{v:1},_obvious?},yes},{nl},> <}>
 * <exit1> <exit2> <exit3>
 */


#include "config.h"
#include <math.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>
#include "params.h"
#include "db.h"
#include "tune.h"

#include "mpi.h"
#include "externs.h"
#include "props.h"
#include "match.h"
#include "interp.h"
#include "interface.h"
#include "msgparse.h"
#include "mfun.h"

time_t mpi_prof_start_time;

int
Wizperms(dbref what)
{
    if (Wizard(what) && TrueWizard(OWNER(what))) return 1;
    return 0;
}


int
safeputprop(dbref obj, dbref perms, char *buf, char *val)
{
    char *ptr;
    if (!buf) return 0;
    while (*buf == PROPDIR_DELIMITER) buf++;
    if (!*buf) return 0;

    /* disallow CR's and :'s in prop names. */
    for (ptr = buf; *ptr; ptr++)
        if (*ptr == '\r' || *ptr == PROP_DELIMITER) return 0;

    if (!Wizperms(perms)) {
        if (Prop_Hidden(buf)) return 0;
        if (Prop_SeeOnly(buf)) return 0;
	if (string_prefix(buf, "_msgmacs/")) return 0;
    }
    if (!val) {
        remove_property(obj, buf);
    } else {
	add_property(obj, buf, val, 0);
    }
    return 1;
}


const char *
safegetprop_strict(dbref player, dbref what, dbref perms, const char *inbuf)
{
    const char *ptr;
    char  bbuf[BUFFER_LEN];
    static char  vl[32];

    if (!inbuf) {
        notify_nolisten(player, "PropFetch: Propname required.", 1);
        return NULL;
    }
    while (*inbuf == PROPDIR_DELIMITER)
        inbuf++;
    if (!*inbuf) {
        notify_nolisten(player, "PropFetch: Propname required.", 1);
        return NULL;
    }
    strcpy(bbuf, inbuf);
    ptr = get_property_class(what, bbuf);
    if (!ptr) {
        int i;

	i = get_property_value(what, bbuf);
	if (!i) {
	    dbref dd;

	    dd = get_property_dbref(what, bbuf);
	    if (dd == NOTHING) {
		*vl = '\0';
		ptr = vl;
		return ptr;
	    } else {
		sprintf(vl, "#%d", dd);
		ptr = vl;
	    }
	} else {
	    sprintf(vl, "%d", i);
	    ptr = vl;
	}
    }

#ifdef COMPRESS
    ptr = uncompress(ptr);
#endif

    if (!Wizperms(perms)) {
        if (Prop_Hidden(bbuf)) {
            notify_nolisten(player, "PropFetch: Permission denied.", 1);
            return NULL;
        }
//        if (Prop_Private(bbuf) && OWNER(perms) != OWNER(what)) {
        if (Prop_Private(bbuf) && !controls_nowizperm(OWNER(perms), what)) {
            notify_nolisten(player, "PropFetch: Permission denied.", 1);
            return NULL;
        }
    }
    return ptr;
}


const char *
safegetprop_limited(dbref player, dbref what, dbref whom, dbref perms, const char *inbuf)
{
    const char *ptr;

    while (what != NOTHING) {
	if (OWNER(what) == whom || Wizard(what) ||
	    safegetprop_strict(player, what, perms, "~mpi_macros_ok")
	) {
	    ptr = safegetprop_strict(player, what, perms, inbuf);
	    if (!ptr || *ptr) return ptr;
	}
        what = getparent(what);
    }
    return "";
}


const char *
safegetprop(dbref player, dbref what, dbref perms, const char *inbuf)
{
    const char *ptr;

    while (what != NOTHING) {
	ptr = safegetprop_strict(player, what, perms, inbuf);
        if (!ptr || *ptr) return ptr;
        what = getparent(what);
    }
    return "";
}


char *
stripspaces(char *buf, char *in)
{
    char *ptr;
    for (ptr = in; *ptr == ' '; ptr++);
    strcpy(buf, ptr);
    ptr = strlen(buf) + buf - 1;
    while (*ptr == ' ' && ptr > buf)
        *(ptr--) = '\0';
    return buf;
}


char *
string_substitute(const char *str, const char *oldstr, const char *newstr, char *buf, int maxlen)
{
    const char *ptr = str;
    char *ptr2 = buf;
    const char *ptr3;
    int len = strlen(oldstr);

    if (len == 0) {
	strcpy(buf, str);
        return buf;
    }
    while (*ptr) {
        if (!strncmp(ptr, oldstr, len)) {
            for(ptr3 = newstr; ((ptr2 - buf) < (maxlen - 2)) && *ptr3;)
                *(ptr2++) = *(ptr3++);
            ptr += len;
        } else {
            *(ptr2++) = *(ptr++);
        }
    }
    *ptr2 = '\0';
    return buf;
}


const char *
get_list_item(dbref player, dbref what, dbref perms, const char *listname, int itemnum)
{
    char    buf[BUFFER_LEN];
    const char *ptr;

    sprintf(buf, "%.512s#/%d", listname, itemnum);
    ptr = safegetprop(player, what, perms, buf);
    if (!ptr || *ptr) return ptr;

    sprintf(buf, "%.512s/%d", listname, itemnum);
    ptr = safegetprop(player, what, perms, buf);
    if (!ptr || *ptr) return ptr;

    sprintf(buf, "%.512s%d", listname, itemnum);
    return (safegetprop(player, what, perms, buf));
}


int
get_list_count(dbref player, dbref obj, dbref perms, const char *listname)
{
    char    buf[BUFFER_LEN];
    const char *ptr;
    int i;

    sprintf(buf, "%.512s#", listname);
    ptr = safegetprop(player, obj, perms, buf);
    if (ptr && *ptr) return (atoi(ptr));

    sprintf(buf, "%.512s/#", listname);
    ptr = safegetprop(player, obj, perms, buf);
    if (ptr && *ptr) return (atoi(ptr));

    for (i = 1; i < MAX_MFUN_LIST_LEN; i++) {
	ptr = get_list_item(player, obj, perms, listname, i);
        if (!ptr) return 0;
        if (!*ptr) break;
    }
    if (i-- < MAX_MFUN_LIST_LEN) return i;
    return MAX_MFUN_LIST_LEN;
}



char   *
get_concat_list(dbref player, dbref what, dbref perms, dbref obj, const char *listname, char *buf, int maxchars, int mode)
{
    int line_limit = MAX_MFUN_LIST_LEN;
    int     i;
    const char *ptr;
    char   *pos = buf;
    int     cnt = get_list_count(what, obj, perms, listname);

    if (cnt == 0) {
        return NULL;
    }
    maxchars -= 2;
    *buf = '\0';
    for (i = 1; ((pos - buf) < (maxchars - 1)) && i <= cnt && line_limit--; i++) {
        ptr = get_list_item(what, obj, perms, listname, i);
        if (ptr) {
            while (mode && isspace(*ptr))
                ptr++;
            if (pos > buf) {
                if (!mode) {
                    *(pos++) = '\r';
                    *pos = '\0';
                } else if (mode == 1) {
                    char    ch = *(pos - 1);

                    if ((pos - buf) >= (maxchars - 2))
                        break;
                    if (ch == '.' || ch == '?' || ch == '!')
                        *(pos++) = ' ';
                    *(pos++) = ' ';
                    *pos = '\0';
		} else {
                    *pos = '\0';
                }
            }
            while (((pos - buf) < (maxchars - 1)) && *ptr)
                *(pos++) = *(ptr++);
	    if (mode) {
		while (pos > buf && *(pos - 1) == ' ') pos--;
	    }
            *pos = '\0';
            if ((pos - buf) >= (maxchars - 1))
                break;
        }
    }
    return (buf);
}


int
mesg_read_perms(dbref player, dbref perms, dbref obj)
{
    if (
        Wizperms(perms) ||
        may_read_nowizperm(OWNER(perms), obj))
        return 1;

    return 0;
}


int
isneighbor(dbref d1, dbref d2)
{
    if (d1 == d2) return 1;
    if (Typeof(d1) != TYPE_ROOM)
	if (getloc(d1) == d2) return 1;
    if (Typeof(d2) != TYPE_ROOM)
	if (getloc(d2) == d1) return 1;
    if (Typeof(d1) != TYPE_ROOM && Typeof(d2) != TYPE_ROOM)
	if (getloc(d1) == getloc(d2)) return 1;
    return 0;
}


int
mesg_local_perms(dbref player, dbref perms, dbref obj)
{

    if (obj == player) return 1;
    if (isneighbor(player, obj)) return 1;
    if (isneighbor(perms, obj)) return 1;
    if ((getloc(obj) != NOTHING) && 
         may_read_nowizperm(OWNER(perms), getloc(obj)))
        return 1;
    if (mesg_read_perms(player, perms, obj)) return 1;
    return 0;
}


dbref
mesg_dbref_raw(dbref player, dbref what, dbref perms, const char *buf)
{
    struct match_data md;
    dbref   obj = UNKNOWN;

    if (buf && *buf) {
        if (!string_compare(buf, "this")) {
            obj = what;
        } else if (!string_compare(buf, "me")) {
            obj = player;
        } else if (!string_compare(buf, "here")) {
            obj = getloc(player);
        } else if (!string_compare(buf, "home")) {
            obj = HOME;
        } else {
            init_match(player, buf, NOTYPE, &md);
	    match_absolute(&md);
	    match_all_exits(&md);
	    match_neighbor(&md);
	    match_possession(&md);
	    match_registered(&md);
            obj = match_result(&md);
	    if (obj == NOTHING) {
		init_match_remote(player, what, buf, NOTYPE, &md);
		match_player(&md);
		match_all_exits(&md);
		match_neighbor(&md);
		match_possession(&md);
		match_registered(&md);
		obj = match_result(&md);
	    }
        }
    }

    if (obj < 0 || obj >= db_top || Typeof(obj) == TYPE_GARBAGE)
        obj = UNKNOWN;
    return obj;
}


dbref
mesg_dbref_read(dbref player, dbref what, dbref perms, char *buf)
{
    dbref obj = mesg_dbref_raw(player, what, perms, buf);

    if (obj == UNKNOWN) return obj;
    if (!mesg_read_perms(player, perms, obj)) {
        obj = PERMDENIED;
    }
    return obj;
}


dbref
mesg_dbref_write(dbref player, dbref what, dbref perms, char *buf)
{
    dbref obj = mesg_dbref_raw(player, what, perms, buf);

    if (obj == UNKNOWN) return obj;

    if (!Wizperms(perms) && !controls_nowizperm(OWNER(perms), obj)) {
        obj = PERMDENIED;
    }
    return obj;
}

dbref
mesg_dbref_exec(dbref player, dbref what, dbref perms, char *buf)
{
    return mesg_dbref_write(player, what, perms, buf);
}

dbref
mesg_dbref_proximity(dbref player, dbref what, dbref perms, char *buf)
{
    dbref obj = mesg_dbref_raw(player, what, perms, buf);

    if (obj == UNKNOWN) return obj;
    if (!mesg_local_perms(player, perms, obj)) {
        obj = PERMDENIED;
    }
    return obj;
}


char *
ref2str(dbref obj, char *buf)
{
    if (obj < -3 || obj >= db_top) {
        sprintf(buf, "Bad");
        return buf;
    }

    if (obj >= 0 && Typeof(obj) == TYPE_PLAYER) {
        sprintf(buf, "*%s", RNAME(obj));
    } else {
        sprintf(buf, "#%d", obj);
    }
    return buf;
}


int
truestr(char *buf)
{
    while (isspace(*buf)) buf++;
    if (!*buf || (number(buf) && !atoi(buf)))
        return 0;
    return 1;
}


/******** MPI Variable handling ********/

struct mpivar {
    char name[MAX_MFUN_NAME_LEN + 1];
    char *buf;
};

static struct mpivar varv[MPI_MAX_VARIABLES];
int varc = 0;

int
new_mvar(const char *varname, char *buf)
{
    if (strlen(varname) > MAX_MFUN_NAME_LEN) return 1;
    if (varc > MPI_MAX_VARIABLES) return 2;
    strcpy(varv[varc].name, varname);
    varv[varc++].buf = buf;
    return 0;
}

char *
get_mvar(const char *varname)
{
    int i = 0;
    for (i = varc-1; i >= 0 && string_compare(varname, varv[i].name); i--);
    if (i < 0) return NULL;
    return varv[i].buf;
}

int
free_top_mvar()
{
    if (varc < 1) return 1;
    varc--;
    return 0;
}


/***** MPI function handling *****/


struct mpifunc {
    char name[MAX_MFUN_NAME_LEN + 1];
    char *buf;
};

static struct mpifunc funcv[MPI_MAX_FUNCTIONS];
int funcc = 0;

int
new_mfunc(const char *funcname, const char *buf)
{
    if (strlen(funcname) > MAX_MFUN_NAME_LEN) return 1;
    if (funcc > MPI_MAX_FUNCTIONS) return 2;
    strcpy(funcv[funcc].name, funcname);
    funcv[funcc++].buf = (char *)string_dup(buf);
    return 0;
}

const char *
get_mfunc(const char *funcname)
{
    int i = 0;
    for (i = funcc-1; i >= 0 && string_compare(funcname, funcv[i].name); i--);
    if (i < 0) return NULL;
    return funcv[i].buf;
}

int
free_mfuncs(int downto)
{
    if (funcc < 1) return 1;
    if (downto < 0) return 1;
    while (funcc > downto)
	free(funcv[--funcc].buf);
    return 0;
}



/*** End of MFUNs. ***/

int
msg_is_macro(dbref player, dbref what, dbref perms, const char *name)
{
    const char   *ptr;
    char    buf2[BUFFER_LEN];
    dbref   obj;

    if (!*name) return 0;
    sprintf(buf2, "_msgmacs/%s", name);
    obj = what;
    ptr = get_mfunc(name);
    if (!ptr || !*ptr) ptr = safegetprop_strict(player, OWNER(obj), perms, buf2);
    if (!ptr || !*ptr) ptr = safegetprop_limited(player, obj, OWNER(obj), perms, buf2);
    if (!ptr || !*ptr) ptr = safegetprop_strict(player, 0, perms, buf2);
    if (!ptr || !*ptr) return 0;
    return 1;
}


void
msg_unparse_macro(dbref player, dbref what, dbref perms, char *name, int argc, argv_typ argv, char *rest, int maxchars)
{
    const char   *ptr;
    char *ptr2;
    char    buf[BUFFER_LEN];
    char    buf2[BUFFER_LEN];
    dbref   obj;
    int     i, p = 0;

    strcpy(buf, rest);
    sprintf(buf2, "_msgmacs/%s", name);
    obj = what;
    ptr = get_mfunc(name);
    if (!ptr || !*ptr) ptr = safegetprop_strict(player, OWNER(obj), perms, buf2);
    if (!ptr || !*ptr) ptr = safegetprop_limited(player, obj, OWNER(obj), perms, buf2);
    if (!ptr || !*ptr) ptr = safegetprop_strict(player, 0, perms, buf2);
    while (ptr && *ptr && p < (maxchars - 1)) {
        if (*ptr == '\\') {
            if (*(ptr+1) == 'r') {
                rest[p++] = '\r';
		ptr++; ptr++;
            } else {
                rest[p++] = *(ptr++);
                rest[p++] = *(ptr++);
            }
        } else if (*ptr == MFUN_LEADCHAR) {
            if (*(ptr+1) == MFUN_ARGSTART && isdigit(*(ptr + 2)) &&
                    *(ptr + 3) == MFUN_ARGEND) {
                ptr++; ptr++;
                i = *(ptr++) - '1';
                ptr++;
                if (i >= argc || i < 0) {
                    ptr2 = NULL;
                } else {
                    ptr2 = argv[i];
                }
                while (ptr2 && *ptr2 && p < (maxchars - 1)) {
                    rest[p++] = *(ptr2++);
                }
            } else {
                rest[p++] = *(ptr++);
            }
        } else {
            rest[p++] = *(ptr++);
        }
    }
    ptr2 = buf;
    while (ptr2 && *ptr2 && p < (maxchars - 1)) {
        rest[p++] = *(ptr2++);
    }
    rest[p] = '\0';
}


#ifndef MSGHASHSIZE
#define MSGHASHSIZE 256
#endif

static hash_tab msghash[MSGHASHSIZE];

int
find_mfn(const char *name)
{
    hash_data *exp = find_hash(name, msghash, MSGHASHSIZE);

    if (exp)
        return (exp->ival);
    return (0);
}


void
insert_mfn(const char *name, int i)
{
    hash_data hd;

    (void) kill_def(name);
    hd.ival = i;
    (void) add_hash(name, hd, msghash, MSGHASHSIZE);
}


void
purge_mfns(void)
{
    kill_hash(msghash, MSGHASHSIZE, 0);
}



#include "mfunlist.h"

/******** HOOK ********/
void
mesg_init(void)
{
    int i;

    for (i = 0; mfun_list[i].name; i++)
        insert_mfn(mfun_list[i].name, i + 1);
    mpi_prof_start_time = time(NULL);
}



/******** HOOK ********/
int
mesg_args(char *wbuf, argv_typ argv, char ulv, char sep, char dlv, char quot, int maxargs)
{
    int     r, lev, argc = 0;
    char    buf[BUFFER_LEN];
    char   *ptr;
    int     litflag = 0;

    /* for (ptr = wbuf; ptr && isspace(*ptr); ptr++); */
    strcpy(buf, wbuf);
    ptr = buf;
    for (lev = r = 0; (r < (BUFFER_LEN - 2)); r++) {
        if (buf[r] == '\0') {
	    return (-1);
        } else if (buf[r] == '\\') {
            r++;
	    continue;
        } else if (buf[r] == quot) {
            litflag = (!litflag);
        } else if (!litflag && buf[r] == ulv) {
            lev++;
	    continue;
        } else if (!litflag && buf[r] == dlv) {
	    lev--;
            if (lev < 0) {
		buf[r] = '\0';
		strcpy(argv[argc++], ptr);
		ptr = buf + r + 1;
                break;
            }
	    continue;
        } else if (!litflag && lev < 1 && buf[r] == sep) {
            if (argc < maxargs - 1) {
		buf[r] = '\0';
                strcpy(argv[argc++], ptr);
		ptr = buf + r + 1;
	    }
	}
    }
    buf[BUFFER_LEN - 1] = '\0';
    strcpy(wbuf, ptr);
    return argc;
}


char *
cr2slash(char *buf, const char *in)
{
    char *ptr = buf;
    const char *ptr2 = in;
    do {
        if (*ptr2 == '\r') {
            ptr2++;
            *(ptr++) = '\\';
            *(ptr++) = 'r';
        } else if (*ptr2 == '\\') {
            ptr2++;
            *(ptr++) = '\\';
            *(ptr++) = '\\';
        } else {
            *(ptr++) = *(ptr2++);
        }
    } while (*(ptr - 1) && (ptr - buf < BUFFER_LEN - 3));
    buf[BUFFER_LEN - 1] = '\0';
    return buf;
}


static int mesg_rec_cnt = 0;
static int mesg_instr_cnt = 0;


/******** HOOK ********/
char   *
mesg_parse(dbref player, dbref what, dbref perms, const char *inbuf, char *outbuf, int maxchars, int mesgtyp)
{
    char    wbuf[BUFFER_LEN];
    char    buf[BUFFER_LEN];
    char    buf2[BUFFER_LEN];
    char    dbuf[BUFFER_LEN];
    char    ebuf[BUFFER_LEN];
    char    cmdbuf[MAX_MFUN_NAME_LEN + 1];
    const char *ptr;
    char *ptr2, *pname, *dptr;
    int     p, q, r, s;
    int i;
    dbref   obj;
    char    argv[9][BUFFER_LEN];
    int     argc;
    int showtextflag = 0;
    int literalflag = 0;

    mesg_rec_cnt++;
    if (mesg_rec_cnt > 26) {
        mesg_rec_cnt--;
        strncpy(outbuf, inbuf, maxchars);
        outbuf[maxchars - 1] = '\0';
        return outbuf;
    }
    if (Typeof(player) == TYPE_GARBAGE) {
	return NULL;
    }
    if (Typeof(what) == TYPE_GARBAGE) {
	notify_nolisten(player, "MPI Error: Garbage trigger.", 1);
	return NULL;
    }
    strcpy(wbuf, inbuf);
    for (p = q = 0; wbuf[p] && (p < maxchars - 1) && q < (maxchars - 1); p++) {
        if (wbuf[p] == '\\') {
            p++;
            showtextflag = 1;
            if (wbuf[p] == 'r') {
                outbuf[q++] = '\r';
            } else {
                outbuf[q++] = wbuf[p];
            }
        } else if (wbuf[p] == MFUN_LITCHAR) {
            literalflag = (!literalflag);
        } else if (!literalflag && wbuf[p] == MFUN_LEADCHAR) {
            if (wbuf[p + 1] == MFUN_LEADCHAR) {
                showtextflag = 1;
                outbuf[q++] = wbuf[p++];
		ptr = "";
            } else {
                ptr = wbuf + (++p);
                s = 0;
                while (wbuf[p] && wbuf[p] != MFUN_LEADCHAR &&
                        !isspace(wbuf[p]) && wbuf[p] != MFUN_ARGSTART &&
                        wbuf[p] != MFUN_ARGEND && s < MAX_MFUN_NAME_LEN) {
                    p++; s++;
                }
                if (s < MAX_MFUN_NAME_LEN &&
                        (wbuf[p] == MFUN_ARGSTART || wbuf[p] == MFUN_ARGEND)) {
                    int varflag;

                    strncpy(cmdbuf,ptr,s);
                    cmdbuf[s] = '\0';

		    varflag = 0;
                    if (*cmdbuf == '&') {
			s = find_mfn("sublist");
			varflag = 1;
                    } else if (*cmdbuf) {
			s = find_mfn(cmdbuf);
                    } else {
                        s = 0;
                    }
                    if (s) {
			s--;
			if (++mesg_instr_cnt > tp_mpi_max_commands) {
			    char *zptr = get_mvar("how");
			    sprintf(dbuf, "%s %c%s%c: Instruction limit exceeded.",
				    zptr, MFUN_LEADCHAR,
				    (varflag? cmdbuf : mfun_list[s].name),
				    MFUN_ARGEND);
			    notify_nolisten(player, dbuf, 1);
			    return NULL;
			}
                        if (wbuf[p] == MFUN_ARGEND) {
                            argc = 0;
                        } else {
			    argc = mfun_list[s].maxargs;
			    if (argc < 0) {
				argc = mesg_args((wbuf+p+1),
				        &argv[(varflag? 1 : 0)],
					MFUN_LEADCHAR, MFUN_ARGSEP,
					MFUN_ARGEND, MFUN_LITCHAR,
					(-argc) + (varflag? 1 : 0));
			    } else {
				argc = mesg_args((wbuf+p+1),
				        &argv[(varflag? 1 : 0)],
					MFUN_LEADCHAR, MFUN_ARGSEP,
					MFUN_ARGEND, MFUN_LITCHAR,
					(varflag? 8 : 9));
			    }
			    if (argc == -1) {
				char *zptr = get_mvar("how");
				sprintf(ebuf, "%s %c%s%c: End brace not found.",
					zptr, MFUN_LEADCHAR, cmdbuf,
					MFUN_ARGEND);
				notify_nolisten(player, ebuf, 1);
				return NULL;
			    }
                        }
                        if (varflag) {
			    char *zptr;

			    argc++;
			    zptr = get_mvar(cmdbuf + 1);
			    if (!zptr) {
				zptr = get_mvar("how");
				sprintf(ebuf, "%s %c%s%c: Unrecognized variable.",
					zptr, MFUN_LEADCHAR, cmdbuf, MFUN_ARGEND);
				notify_nolisten(player, ebuf, 1);
				return NULL;
			    }
			    strcpy(argv[0], zptr);
                        }
                        if (mesgtyp & MPI_ISDEBUG) {
                            char *zptr = get_mvar("how");
                            sprintf(dbuf, "%s %*s%c%s%c", zptr,
                                    (mesg_rec_cnt*2-4), "", MFUN_LEADCHAR,
                                    (varflag? cmdbuf : mfun_list[s].name),
                                    MFUN_ARGSTART);
                            for (i = (varflag? 1 : 0); i < argc; i++) {
                                if (i) {
                                    sprintf(dbuf, "%.512s%c ", dbuf,
                                            MFUN_ARGSEP);
                                }
                                cr2slash(ebuf, argv[i]);
                                if (strlen(ebuf) > 512) {
                                    sprintf(dbuf, "%.512s\"%.512s...\"",
                                            dbuf, ebuf);
                                } else {
                                    sprintf(dbuf, "%.512s\"%s\"", dbuf, ebuf);
                                }
                            }
                            sprintf(dbuf, "%.512s%c", dbuf, MFUN_ARGEND);
                            notify_nolisten(player, dbuf, 1);
                        }
                        if (mfun_list[s].stripp) {
                            for (i = (varflag? 1 : 0); i < argc; i++) {
                                strcpy(argv[i], stripspaces(buf, argv[i]));
                            }
                        }
                        if (mfun_list[s].parsep) {
                            for (i = (varflag? 1 : 0); i < argc; i++) {
                                ptr = MesgParse(argv[i], argv[i]);
                                if (!ptr) {
                                    char *zptr = get_mvar("how");
                                    sprintf(dbuf, "%s %c%s%c (arg %d)", zptr,
                                            MFUN_LEADCHAR,
                                            (varflag?cmdbuf:mfun_list[s].name),
                                            MFUN_ARGEND, i+1);
                                    notify_nolisten(player, dbuf, 1);
                                    return NULL;
                                }
                            }
                        }
                        if (mesgtyp & MPI_ISDEBUG) {
                            char *zptr = get_mvar("how");
                            sprintf(dbuf, "%.512s %*s%c%.512s%c", zptr,
                                    (mesg_rec_cnt*2-4), "", MFUN_LEADCHAR,
                                    (varflag? cmdbuf : mfun_list[s].name),
                                    MFUN_ARGSTART);
                            for (i = (varflag? 1 : 0); i < argc; i++) {
                                if (i) {
                                    sprintf(dbuf, "%.512s%c ", dbuf,
                                            MFUN_ARGSEP);
                                }
                                cr2slash(ebuf, argv[i]);
                                if (strlen(ebuf) > 128) {
                                    sprintf(dbuf, "%.512s\"%.128s...\"",
                                            dbuf, ebuf);
                                } else {
                                    sprintf(dbuf, "%.512s\"%s\"", dbuf, ebuf);
                                }
                            }
                            sprintf(dbuf, "%s%c", dbuf, MFUN_ARGEND);
                        }
                        if (argc < mfun_list[s].minargs) {
                            char *zptr = get_mvar("how");
                            sprintf(ebuf, "%s %c%s%c: Too few arguments",
                                    zptr, MFUN_LEADCHAR,
                                    (varflag? cmdbuf : mfun_list[s].name),
                                    MFUN_ARGEND);
                            notify_nolisten(player, ebuf, 1);
                            return NULL;
                        } else if (mfun_list[s].maxargs > 0 &&
				    argc > mfun_list[s].maxargs) {
                            char *zptr = get_mvar("how");
                            sprintf(ebuf, "%s %c%s%c: Too many arguments",
                                    zptr, MFUN_LEADCHAR,
                                    (varflag? cmdbuf : mfun_list[s].name),
                                    MFUN_ARGEND);
                            notify_nolisten(player, ebuf, 1);
                            return NULL;
                        } else {
                            ptr = mfun_list[s].mfn(player, what, perms, argc,
                                                   argv, buf, mesgtyp);
			    if (!ptr) {
				outbuf[q] = '\0';
				return NULL;
			    }
                            if (mfun_list[s].postp) {
                                dptr = MesgParse(ptr, buf);
                                if (!dptr) {
                                    char *zptr = get_mvar("how");
                                    sprintf(ebuf, "%s %c%s%c (returned string)",
                                            zptr, MFUN_LEADCHAR,
                                            (varflag?cmdbuf:mfun_list[s].name),
                                            MFUN_ARGEND);
                                    notify_nolisten(player, ebuf, 1);
                                    return NULL;
                                }
				ptr = dptr;
                            }
                        }
                        if (mesgtyp & MPI_ISDEBUG) {
                            sprintf(dbuf, "%.512s = \"%.512s\"", dbuf,
                                    cr2slash(ebuf, ptr));
                            notify_nolisten(player, dbuf, 1);
                        }
                    } else if (msg_is_macro(player, what, perms, cmdbuf)) {
                        if (wbuf[p] == MFUN_ARGEND) {
                            argc = 0;
			    p++;
                        } else {
			    p++;
                            argc = mesg_args(wbuf+p, argv, MFUN_LEADCHAR,
				 MFUN_ARGSEP, MFUN_ARGEND, MFUN_LITCHAR, 9);
			    if (argc == -1) {
				char *zptr = get_mvar("how");
				sprintf(ebuf, "%s %c%s%c: End brace not found.",
					zptr, MFUN_LEADCHAR, cmdbuf,
					MFUN_ARGEND);
				notify_nolisten(player, ebuf, 1);
				return NULL;
			    }
                        }
                        msg_unparse_macro(player, what, perms, cmdbuf, argc,
					    argv, (wbuf+p), (BUFFER_LEN - p));
			p--;
                        ptr = NULL;
                    } else {
			/* unknown function */
			char *zptr = get_mvar("how");
			sprintf(ebuf, "%s %c%s%c: Unrecognized function.",
				zptr, MFUN_LEADCHAR, cmdbuf, MFUN_ARGEND);
			notify_nolisten(player, ebuf, 1);
			return NULL;
                    }
                } else {
                    showtextflag = 1;
                    p = (int) (ptr - wbuf);
                    if (q < (maxchars - 1))
                        outbuf[q++] = MFUN_LEADCHAR;
                    ptr = "";   /* unknown substitution type */
                }
                while (ptr && *ptr && q < (maxchars -1))
                    outbuf[q++] = *(ptr++);
            }
        } else {
            outbuf[q++] = wbuf[p];
            showtextflag = 1;
        }
    }
    outbuf[q] = '\0';
    if ((mesgtyp & MPI_ISDEBUG) && showtextflag) {
        char *zptr = get_mvar("how");
        sprintf(dbuf, "%s %*s\"%s\"", zptr, (mesg_rec_cnt*2-4), "",
                cr2slash(buf2, outbuf));
        notify_nolisten(player, dbuf, 1);
    }
    mesg_rec_cnt--;
    return (outbuf);
}

char   *
do_parse_mesg_2(dbref player, dbref what, dbref perms, const char *inbuf, const char *abuf, char *outbuf, int mesgtyp)
{

    char howvar[BUFFER_LEN];
    char cmdvar[BUFFER_LEN];
    char argvar[BUFFER_LEN];
    char tmparg[BUFFER_LEN];
    char tmpcmd[BUFFER_LEN];
    char *dptr;
    int  mvarcnt = varc;
    int  mfunccnt = funcc;
    int  tmprec_cnt = mesg_rec_cnt;
    int  tmpinst_cnt = mesg_instr_cnt;

#ifdef COMPRESS
    abuf = uncompress(abuf);
#endif                          /* COMPRESS */

    *outbuf = '\0';
    if (new_mvar("how", howvar)) return outbuf;
    strcpy(howvar, abuf);

    if (new_mvar("cmd", cmdvar)) return outbuf;
    strcpy(cmdvar, match_cmdname);
    strcpy(tmpcmd, match_cmdname);

    if (new_mvar("arg", argvar)) return outbuf;
    strcpy(argvar, match_args);
    strcpy(tmparg, match_args);

#ifdef COMPRESS
    inbuf = uncompress(inbuf);
#endif                          /* COMPRESS */

    dptr = MesgParse(inbuf, outbuf);
    if (!dptr) {
        *outbuf = '\0';
    }

    varc = mvarcnt;
    free_mfuncs(mfunccnt);
    mesg_rec_cnt = tmprec_cnt;
    mesg_instr_cnt = tmpinst_cnt;

    strcpy(match_cmdname, tmpcmd);
    strcpy(match_args, tmparg);


    return outbuf;
}


char *
do_parse_mesg(dbref player, dbref what, const char *inbuf, const char *abuf, char *outbuf, int mesgtyp)
{
    if (tp_do_mpi_parsing) {
        char *tmp = NULL;
	struct timeval st, et;

        /* Quickie additions to do rought per-object MPI profiling */
        gettimeofday(&st,NULL);
	tmp=do_parse_mesg_2(player, what, what, inbuf, abuf, outbuf, mesgtyp);
	gettimeofday(&et,NULL);
	if (strcmp(tmp,inbuf)) {
	  et.tv_usec -= st.tv_usec;
	  et.tv_sec -= st.tv_sec;
	  if (et.tv_usec < 0) {
	    et.tv_usec += 1000000;
	    et.tv_sec -= 1;
	  }
	  DBFETCH(what)->mpi_prof_sec += et.tv_sec;
	  DBFETCH(what)->mpi_prof_usec += et.tv_usec;
	  if (DBFETCH(what)->mpi_prof_usec >= 1000000) {
	    DBFETCH(what)->mpi_prof_usec -= 1000000;
	    DBFETCH(what)->mpi_prof_sec += 1;
	  }
	  DBFETCH(what)->mpi_prof_use++;
	}
	return(tmp);
    } else {
	strcpy(outbuf, inbuf);
    }
    return outbuf;
}


