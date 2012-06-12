#include "config.h"
#include "params.h"
#include "db.h"
#include "defaults.h"
#include "externs.h"


const char *tp_dumpwarn_mesg	= DUMPWARN_MESG;
const char *tp_deltawarn_mesg	= DELTAWARN_MESG;
const char *tp_dumpdeltas_mesg	= DUMPDELTAS_MESG;
const char *tp_dumping_mesg	= DUMPING_MESG;
const char *tp_dumpdone_mesg	= DUMPDONE_MESG;

const char *tp_penny		= PENNY;
const char *tp_pennies		= PENNIES;
const char *tp_cpenny		= CPENNY;
const char *tp_cpennies		= CPENNIES;

const char *tp_muckname 	= MUCKNAME;
const char *tp_rwho_passwd	= RWHO_PASSWD;
const char *tp_rwho_server	= RWHO_SERVER;

const char *tp_huh_mesg         = HUH_MESSAGE;
const char *tp_leave_mesg       = LEAVE_MESSAGE;
const char *tp_idle_mesg        = IDLEBOOT_MESSAGE;
const char *tp_register_mesg    = REG_MSG;

const char *tp_playermax_warnmesg   = PLAYERMAX_WARNMESG;
const char *tp_playermax_bootmesg   = PLAYERMAX_BOOTMESG;


struct tune_str_entry {
    const char *name;
    const char **str;
    int security;
    int isdefault;
};

struct tune_str_entry tune_str_list[] =
{
    {"dumpwarn_mesg",       &tp_dumpwarn_mesg,       0, 1},
    {"deltawarn_mesg",      &tp_deltawarn_mesg,      0, 1},
    {"dumpdeltas_mesg",     &tp_dumpdeltas_mesg,     0, 1},
    {"dumping_mesg",        &tp_dumping_mesg,        0, 1},
    {"dumpdone_mesg",       &tp_dumpdone_mesg,       0, 1},
    {"penny",               &tp_penny,               0, 1},
    {"pennies",             &tp_pennies,             0, 1},
    {"cpenny",              &tp_cpenny,              0, 1},
    {"cpennies",            &tp_cpennies,            0, 1},
    {"muckname",            &tp_muckname,            0, 1},
    {"rwho_passwd",         &tp_rwho_passwd,         4, 1},
    {"rwho_server",         &tp_rwho_server,         4, 1},
    {"huh_mesg",            &tp_huh_mesg,            0, 1},
    {"leave_mesg",          &tp_leave_mesg,          0, 1},
    {"idle_boot_mesg",      &tp_idle_mesg,           0, 1},
    {"register_mesg",       &tp_register_mesg,       0, 1},
    {"playermax_warnmesg",  &tp_playermax_warnmesg,  0, 1},
    {"playermax_bootmesg",  &tp_playermax_bootmesg,  0, 1},
    {NULL, NULL, 0, 0}
};



/* times */
int tp_rwho_interval		= RWHO_INTERVAL;
int tp_dump_interval		= DUMP_INTERVAL;
int tp_dump_warntime		= DUMP_WARNTIME;
int tp_monolithic_interval	= MONOLITHIC_INTERVAL;
int tp_clean_interval		= CLEAN_INTERVAL;
int tp_aging_time		= AGING_TIME;
int tp_maxidle			= MAXIDLE;


struct tune_time_entry {
    const char *name;
    int *tim;
    int security;
};

struct tune_time_entry tune_time_list[] =
{
    {"rwho_interval",       &tp_rwho_interval,		0},
    {"dump_interval",       &tp_dump_interval,		0},
    {"dump_warntime",       &tp_dump_warntime,		0},
    {"monolithic_interval", &tp_monolithic_interval,	0},
    {"clean_interval",      &tp_clean_interval,		0},
    {"aging_time",          &tp_aging_time,		0},
    {"maxidle",             &tp_maxidle,		0},
    {NULL, NULL, 0}
};



/* integers */
int tp_max_object_endowment	= MAX_OBJECT_ENDOWMENT;
int tp_object_cost		= OBJECT_COST;
int tp_exit_cost		= EXIT_COST;
int tp_link_cost		= LINK_COST;
int tp_room_cost		= ROOM_COST;
int tp_lookup_cost		= LOOKUP_COST;
int tp_max_pennies		= MAX_PENNIES;
int tp_penny_rate		= PENNY_RATE;
int tp_start_pennies		= START_PENNIES;

int tp_kill_base_cost		= KILL_BASE_COST;
int tp_kill_min_cost		= KILL_MIN_COST;
int tp_kill_bonus		= KILL_BONUS;

int tp_command_burst_size	= COMMAND_BURST_SIZE;
int tp_commands_per_time	= COMMANDS_PER_TIME;
int tp_command_time_msec	= COMMAND_TIME_MSEC;

int tp_max_delta_objs		= MAX_DELTA_OBJS;
int tp_max_loaded_objs		= MAX_LOADED_OBJS;
int tp_max_process_limit	= MAX_PROCESS_LIMIT;
int tp_max_plyr_processes	= MAX_PLYR_PROCESSES;
int tp_max_instr_count		= MAX_INSTR_COUNT;
int tp_instr_slice		= INSTR_SLICE;
int tp_mpi_max_commands         = MPI_MAX_COMMANDS;
int tp_pause_min		= PAUSE_MIN;
int tp_free_frames_pool		= FREE_FRAMES_POOL;

int tp_listen_mlev		= LISTEN_MLEV;
int tp_playermax_limit          = PLAYERMAX_LIMIT;


struct tune_val_entry {
    const char *name;
    int *val;
    int security;
};

struct tune_val_entry tune_val_list[] =
{
    {"max_object_endowment", &tp_max_object_endowment,	0},
    {"object_cost",          &tp_object_cost,		0},
    {"exit_cost",            &tp_exit_cost,		0},
    {"link_cost",            &tp_link_cost,		0},
    {"room_cost",            &tp_room_cost,		0},
    {"lookup_cost",          &tp_lookup_cost,		0},
    {"max_pennies",          &tp_max_pennies,		0},
    {"penny_rate",           &tp_penny_rate,		0},
    {"start_pennies",        &tp_start_pennies,		0},

    {"kill_base_cost",       &tp_kill_base_cost,	0},
    {"kill_min_cost",        &tp_kill_min_cost,		0},
    {"kill_bonus",           &tp_kill_bonus,		0},

    {"command_burst_size",   &tp_command_burst_size,	0},
    {"commands_per_time",    &tp_commands_per_time,	0},
    {"command_time_msec",    &tp_command_time_msec,	0},

    {"max_delta_objs",       &tp_max_delta_objs,	0},
    {"max_loaded_objs",      &tp_max_loaded_objs,	0},
    {"max_process_limit",    &tp_max_process_limit,	0},
    {"max_plyr_processes",   &tp_max_plyr_processes,	0},
    {"max_instr_count",      &tp_max_instr_count,	0},
    {"instr_slice",          &tp_instr_slice,		0},
    {"mpi_max_commands",     &tp_mpi_max_commands,	0},
    {"pause_min",            &tp_pause_min,		0},
    {"free_frames_pool",     &tp_free_frames_pool,	0},

    {"listen_mlev",          &tp_listen_mlev,		0},
    {"playermax_limit",      &tp_playermax_limit,		0},

    {NULL, NULL, 0}
};




/* dbrefs */
dbref tp_player_start	= PLAYER_START;


struct tune_ref_entry {
    const char *name;
    int typ;
    dbref *ref;
    int security;
};

struct tune_ref_entry tune_ref_list[] =
{
    {"player_start", TYPE_ROOM, &tp_player_start,	0},
    {NULL, 0, NULL, 0}
};


/* booleans */
int tp_hostnames 		= HOSTNAMES;
int tp_log_commands 		= LOG_COMMANDS;
int tp_log_interactive 		= LOG_INTERACTIVE;
int tp_log_failed_commands 	= LOG_FAILED_COMMANDS;
int tp_log_programs 		= LOG_PROGRAMS;
int tp_dbdump_warning 		= DBDUMP_WARNING;
int tp_deltadump_warning 	= DELTADUMP_WARNING;
int tp_periodic_program_purge 	= PERIODIC_PROGRAM_PURGE;
int tp_rwho 			= RWHO;
int tp_secure_who 		= SECURE_WHO;
int tp_who_doing 		= WHO_DOING;
int tp_realms_control 		= REALMS_CONTROL;
int tp_listeners 		= LISTENERS;
int tp_listeners_obj 		= LISTENERS_OBJ;
int tp_listeners_env 		= LISTENERS_ENV;
int tp_zombies 			= ZOMBIES;
int tp_wiz_vehicles 		= WIZ_VEHICLES;
int tp_force_mlev1_name_notify	= FORCE_MLEV1_NAME_NOTIFY;
int tp_restrict_kill 		= RESTRICT_KILL;
int tp_registration 		= REGISTRATION;
int tp_teleport_to_player 	= TELEPORT_TO_PLAYER;
int tp_secure_teleport 		= SECURE_TELEPORT;
int tp_exit_darking 		= EXIT_DARKING;
int tp_thing_darking 		= THING_DARKING;
int tp_dark_sleepers 		= DARK_SLEEPERS;
int tp_who_hides_dark 		= WHO_HIDES_DARK;
int tp_compatible_priorities 	= COMPATIBLE_PRIORITIES;
int tp_do_mpi_parsing 		= DO_MPI_PARSING;
int tp_mpi_strict_permissions   = MPI_STRICT_PERMISSIONS;
int tp_look_propqueues          = LOOK_PROPQUEUES;
int tp_lock_envcheck            = LOCK_ENVCHECK;
int tp_diskbase_propvals        = DISKBASE_PROPVALS;
int tp_idleboot                 = IDLEBOOT;
int tp_playermax                = PLAYERMAX;


struct tune_bool_entry {
    const char *name;
    int *tbool;
    int security;
};

struct tune_bool_entry tune_bool_list[] =
{
    {"use_hostnames",		&tp_hostnames,			0},
    {"log_commands",		&tp_log_commands,		4},
    {"log_interactive",		&tp_log_interactive,		4},
    {"log_failed_commands",	&tp_log_failed_commands,	4},
    {"log_programs",		&tp_log_programs,		4},
    {"dbdump_warning",		&tp_dbdump_warning,		0},
    {"deltadump_warning",	&tp_deltadump_warning,		0},
    {"periodic_program_purge",	&tp_periodic_program_purge,	0},
    {"support_rwho",		&tp_rwho,			0},
    {"secure_who",		&tp_secure_who,			0},
    {"who_doing",		&tp_who_doing,			0},
    {"realms_control",		&tp_realms_control,		0},
    {"allow_listeners",		&tp_listeners,			0},
    {"allow_listeners_obj",	&tp_listeners_obj,		0},
    {"allow_listeners_env",	&tp_listeners_env,		0},
    {"allow_zombies",		&tp_zombies,			0},
    {"wiz_vehicles",		&tp_wiz_vehicles,		0},
    {"force_mlev1_name_notify",	&tp_force_mlev1_name_notify,	0},
    {"restrict_kill",		&tp_restrict_kill,		0},
    {"registration",		&tp_registration,		0},
    {"teleport_to_player",	&tp_teleport_to_player,		0},
    {"secure_teleport",		&tp_secure_teleport,		0},
    {"exit_darking",		&tp_exit_darking,		0},
    {"thing_darking",		&tp_thing_darking,		0},
    {"dark_sleepers",		&tp_dark_sleepers,		0},
    {"who_hides_dark",		&tp_who_hides_dark,		4},
    {"compatible_priorities",	&tp_compatible_priorities,	0},
    {"do_mpi_parsing",		&tp_do_mpi_parsing,		0},
    {"mpi_strict_permissions",     &tp_mpi_strict_permissions,	0},
    {"look_propqueues",		&tp_look_propqueues,		0},
    {"lock_envcheck",		&tp_lock_envcheck,		0},
    {"diskbase_propvals",	&tp_diskbase_propvals,		0},
    {"idleboot",		&tp_idleboot,			0},
    {"playermax",		&tp_playermax,			0},

    {NULL, NULL, 0}
};


static const char *
timestr_full(long dtime)
{
    static char buf[32];
    int days, hours, minutes;

    days = dtime / 86400;
    dtime %= 86400;
    hours = dtime / 3600;
    dtime %= 3600;
    minutes = dtime / 60;
    dtime %= 60;

    sprintf(buf, "%3dd %2d:%02d:%02d", days, hours, minutes, (int)dtime);

    return buf;
}


int
tune_count_parms()
{
    int total = 0;
    struct tune_str_entry *tstr = tune_str_list;
    struct tune_time_entry *ttim = tune_time_list;
    struct tune_val_entry *tval = tune_val_list;
    struct tune_ref_entry *tref = tune_ref_list;
    struct tune_bool_entry *tbool = tune_bool_list;

    while ((tstr++)->name) total++;
    while ((ttim++)->name) total++;
    while ((tval++)->name) total++;
    while ((tref++)->name) total++;
    while ((tbool++)->name) total++;

    return total;
}


void
tune_display_parms(dbref player, char *name)
{
    char buf[BUFFER_LEN + 50];
    struct tune_str_entry *tstr = tune_str_list;
    struct tune_time_entry *ttim = tune_time_list;
    struct tune_val_entry *tval = tune_val_list;
    struct tune_ref_entry *tref = tune_ref_list;
    struct tune_bool_entry *tbool = tune_bool_list;

    while (tstr->name) {
	strcpy(buf, tstr->name);
	if (!*name || equalstr(name, buf)) {
	    sprintf(buf, "(str)  %-20s = %.4096s", tstr->name, *tstr->str);
	    notify(player, buf);
	}
	tstr++;
    }

    while (ttim->name) {
	strcpy(buf, ttim->name);
	if (!*name || equalstr(name, buf)) {
	    sprintf(buf, "(time) %-20s = %s", ttim->name,
		    timestr_full(*ttim->tim));
	    notify(player, buf);
	}
	ttim++;
    }

    while (tval->name) {
	strcpy(buf, tval->name);
	if (!*name || equalstr(name, buf)) {
	    sprintf(buf, "(int)  %-20s = %d", tval->name, *tval->val);
	    notify(player, buf);
	}
	tval++;
    }

    while (tref->name) {
	strcpy(buf, tref->name);
	if (!*name || equalstr(name, buf)) {
	    sprintf(buf, "(ref)  %-20s = %s", tref->name,
		    unparse_object(player, *tref->ref));
	    notify(player, buf);
	}
	tref++;
    }

    while (tbool->name) {
	strcpy(buf, tbool->name);
	if (!*name || equalstr(name, buf)) {
	    sprintf(buf, "(bool) %-20s = %s", tbool->name,
		    ((*tbool->tbool)? "yes" : "no"));
	    notify(player, buf);
	}
	tbool++;
    }
    notify(player, "*done*");
}


void
tune_save_parms_to_file(FILE *f)
{
    struct tune_str_entry *tstr = tune_str_list;
    struct tune_time_entry *ttim = tune_time_list;
    struct tune_val_entry *tval = tune_val_list;
    struct tune_ref_entry *tref = tune_ref_list;
    struct tune_bool_entry *tbool = tune_bool_list;

    while (tstr->name) {
	fprintf(f, "%s=%.4096s\n", tstr->name, *tstr->str);
	tstr++;
    }

    while (ttim->name) {
	fprintf(f, "%s=%s\n", ttim->name, timestr_full(*ttim->tim));
	ttim++;
    }

    while (tval->name) {
	fprintf(f, "%s=%d\n", tval->name, *tval->val);
	tval++;
    }

    while (tref->name) {
	fprintf(f, "%s=#%d\n", tref->name, *tref->ref);
	tref++;
    }

    while (tbool->name) {
	fprintf(f, "%s=%s\n", tbool->name, (*tbool->tbool)? "yes" : "no");
	tbool++;
    }
}

void
tune_save_parmsfile()
{
    FILE   *f;

    f = fopen(PARMFILE_NAME, "w");
    if (!f) {
	log_status("Couldn't open file %s!\n", PARMFILE_NAME);
	return;
    }

    tune_save_parms_to_file(f);

    fclose(f);
}



const char *
tune_get_parmstring(const char *name, int mlev)
{
    static char buf[BUFFER_LEN + 50];
    struct tune_str_entry *tstr = tune_str_list;
    struct tune_time_entry *ttim = tune_time_list;
    struct tune_val_entry *tval = tune_val_list;
    struct tune_ref_entry *tref = tune_ref_list;
    struct tune_bool_entry *tbool = tune_bool_list;

    while (tstr->name) {
	if (!string_compare(name, tstr->name)) {
	    if (tstr->security > mlev) return "";
	    return (*tstr->str);
	}
	tstr++;
    }

    while (ttim->name) {
	if (!string_compare(name, ttim->name)) {
	    if (ttim->security > mlev) return "";
	    sprintf(buf, "%d", *ttim->tim);
	    return (buf);
	}
	ttim++;
    }

    while (tval->name) {
	if (!string_compare(name, tval->name)) {
	    if (tval->security > mlev) return "";
	    sprintf(buf, "%d", *tval->val);
	    return (buf);
	}
	tval++;
    }

    while (tref->name) {
	if (!string_compare(name, tref->name)) {
	    if (tref->security > mlev) return "";
	    sprintf(buf, "#%d", *tref->ref);
	    return (buf);
	}
	tref++;
    }

    while (tbool->name) {
	if (!string_compare(name, tbool->name)) {
	    if (tbool->security > mlev) return "";
	    sprintf(buf, "%s", ((*tbool->tbool)? "yes" : "no"));
	    return (buf);
	}
	tbool++;
    }

    /* Parameter was not found.  Return null string. */
    strcpy(buf, "");
    return (buf);
}



/* return values:
 *  0 for success.
 *  1 for unrecognized parameter name.
 *  2 for bad parameter value syntax.
 *  3 for bad parameter value.
 */

#define TUNESET_SUCCESS 0
#define TUNESET_UNKNOWN 1
#define TUNESET_SYNTAX 2
#define TUNESET_BADVAL 3

int
tune_setparm(const char *parmname, const char *val)
{
    struct tune_str_entry *tstr = tune_str_list;
    struct tune_time_entry *ttim = tune_time_list;
    struct tune_val_entry *tval = tune_val_list;
    struct tune_ref_entry *tref = tune_ref_list;
    struct tune_bool_entry *tbool = tune_bool_list;
    char buf[BUFFER_LEN];
    char *parmval;

    strcpy(buf, val);
    parmval = buf;

    while (tstr->name) {
	if (!string_compare(parmname, tstr->name)) {
	    if (!tstr->isdefault) free((char *) *tstr->str);
	    if (*parmval == '-') parmval++;
	    *tstr->str = string_dup(parmval);
	    tstr->isdefault = 0;
	    return 0;
	}
	tstr++;
    }

    while (ttim->name) {
	if (!string_compare(parmname, ttim->name)) {
	    int days, hrs, mins, secs, result;
	    char *end;

	    days = hrs = mins = secs = 0;
	    end = parmval + strlen(parmval) - 1;
	    switch (*end) {
		case 's':
		case 'S':
		    *end = '\0';
		    if (!number(parmval)) return 2;
		    secs = atoi(parmval);
		    break;
		case 'm':
		case 'M':
		    *end = '\0';
		    if (!number(parmval)) return 2;
		    mins = atoi(parmval);
		    break;
		case 'h':
		case 'H':
		    *end = '\0';
		    if (!number(parmval)) return 2;
		    hrs = atoi(parmval);
		    break;
		case 'd':
		case 'D':
		    *end = '\0';
		    if (!number(parmval)) return 2;
		    days = atoi(parmval);
		    break;
		default:
		    result = sscanf(parmval, "%dd %2d:%2d:%2d",
				    &days, &hrs, &mins, &secs);
		    if (result != 4) return 2;
		    break;
	    }
	    *ttim->tim = (days * 86400) + (3600 * hrs) + (60 * mins) + secs;
	    return 0;
	}
	ttim++;
    }

    while (tval->name) {
	if (!string_compare(parmname, tval->name)) {
	    if (!number(parmval)) return 2;
	    *tval->val = atoi(parmval);
	    return 0;
	}
	tval++;
    }

    while (tref->name) {
	if (!string_compare(parmname, tref->name)) {
	    dbref obj;

	    if (*parmval != '#') return 2;
	    if (!number(parmval+1)) return 2;
	    obj = (dbref) atoi(parmval+1);
	    if (obj < 0 || obj >= db_top) return 2;
	    if (tref->typ != NOTYPE && Typeof(obj) != tref->typ) return 3;
	    *tref->ref = obj;
	    return 0;
	}
	tref++;
    }

    while (tbool->name) {
	if (!string_compare(parmname, tbool->name)) {
	    if (*parmval == 'y' || *parmval == 'Y') {
		*tbool->tbool = 1;
	    } else if (*parmval == 'n' || *parmval == 'N') {
		*tbool->tbool = 0;
	    } else {
		return 2;
	    }
	    return 0;
	}
	tbool++;
    }

    return 1;
}

void
tune_load_parms_from_file(FILE *f, dbref player, int cnt)
{
    char buf[BUFFER_LEN];
    char *c, *p;
    int result;

    while (!feof(f) && (cnt<0 || cnt--)) {
	fgets(buf, sizeof(buf), f);
	if (*buf != '#') {
	    p = c = index(buf, '=');
	    if (c) {
		*c++ = '\0';
		while (p > buf && isspace(*(--p))) *p = '\0';
		while(isspace(*c)) c++;
		for (p = c; *p && *p != '\n'; p++);
		*p = '\0';
		for (p = buf; isspace(*p); p++);
		if (*p) {
		    result = tune_setparm(p, c);

                    switch (result) {
		        case TUNESET_SUCCESS:
			    strcat(p, ": Parameter set.");
			    break;
		        case TUNESET_UNKNOWN:
			    strcat(p, ": Unknown parameter.");
			    break;
		        case TUNESET_SYNTAX:
			    strcat(p, ": Bad parameter syntax.");
			    break;
		        case TUNESET_BADVAL:
			    strcat(p, ": Bad parameter value.");
			    break;
		    }
		    if (result && player != NOTHING) notify(player, p);
		}
	    }
	}
    }
}

void
tune_load_parmsfile(dbref player)
{
    FILE   *f;

    f = fopen(PARMFILE_NAME, "r");
    if (!f) {
	log_status("Couldn't open file %s!\n", PARMFILE_NAME);
	return;
    }

    tune_load_parms_from_file(f, player, -1);

    fclose(f);
}


void
do_tune(dbref player, char *parmname, char *parmval)
{
    int result;

    if (!Wizard(player)) {
	notify(player, "You pull out a harmonica and play a short tune.");
	return;
    }

    if (*parmname && *parmval) {
	result = tune_setparm(parmname, parmval);
	switch (result) {
	    case TUNESET_SUCCESS:
		log_status("TUNED: %s(%d) tuned %s to %s\n",
			    NAME(player), player, parmname, parmval);
		notify(player, "Parameter set.");
		tune_display_parms(player, parmname);
		break;
	    case TUNESET_UNKNOWN:
		notify(player, "Unknown parameter.");
		break;
	    case TUNESET_SYNTAX:
		notify(player, "Bad parameter syntax.");
		break;
	    case TUNESET_BADVAL:
		notify(player, "Bad parameter value.");
		break;
	}
	return;
    } else if (*parmname) {
	if (!string_compare(parmname, "save")) {
	    tune_save_parmsfile();
	    notify(player, "Saved parameters to configuration file.");
	} else if (!string_compare(parmname, "load")) {
	    tune_load_parmsfile(player);
	    notify(player, "Restored parameters from configuration file.");
	} else {
	    tune_display_parms(player, parmname);
	}
	return;
    } else if (!*parmval && !*parmname) {
	tune_display_parms(player, parmname);
    } else {
	notify(player, "But what do you want to tune?");
	return;
    }
}


