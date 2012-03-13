#define DUMPWARN_MESG   "## Game will pause to save the database in a few minutes. ##"
#define DELTAWARN_MESG  "## Game will pause to save changed objects in a few minutes. ##"
#define DUMPDELTAS_MESG "## Saving changed objects ##"
#define DUMPING_MESG    "## Pausing to save database. This may take a while. ##"
#define DUMPDONE_MESG   "## Save complete. ##"


/* Change this to the name of your muck.  ie: FurryMUCK, or Tapestries */
#define MUCKNAME "NoNameMUCK"

/* name of the monetary unit being used */
#define PENNY "penny"
#define PENNIES "pennies"
#define CPENNY "Penny"
#define CPENNIES "Pennies"

/* message seen when a player enters a line the server doesn't understand */
#define HUH_MESSAGE "Huh?  (Type \"help\" for help.)"

/*  Goodbye message.  */
#define LEAVE_MESSAGE "Come back later!"

/*  Idleboot message.  */
#define IDLEBOOT_MESSAGE "Autodisonnecting for inactivity."

/*  How long someone can idle for.  */
#define MAXIDLE TIME_HOUR(2)

/*  Boot idle players?  */
#define IDLEBOOT 1


/* Limit max number of players to allow connected?  (wizards are immune) */
#define PLAYERMAX 0

/* How many connections before blocking? */
#define PLAYERMAX_LIMIT 56

/* The mesg to warn users that nonwizzes can't connect due to connect limits */
#define PLAYERMAX_WARNMESG "You likely won't be able to connect right now, since too many players are online."

/* The mesg to display when booting a connection because of connect limit */
#define PLAYERMAX_BOOTMESG "Sorry, but there are too many players online.  Please try reconnecting in a few minutes."

/*
 * Message if someone trys the create command and your system is
 * setup for registration.
 */
#define REG_MSG "Sorry, you can get a character by e-mailing XXXX@machine.net.address with a charname and password."

/* various times */
#define AGING_TIME TIME_DAY(90) /* Unused time before obj shows as old. */
#define DUMP_INTERVAL TIME_HOUR(4) /* time between dumps (or deltas) */
#define DUMP_WARNTIME TIME_MINUTE(2) /* warning time before a dump */
#define MONOLITHIC_INTERVAL TIME_DAY(1) /* max time between full dumps */
#define CLEAN_INTERVAL TIME_MINUTE(15) /* time between unused obj purges */


/* Enables sending of updates to an RWHO server. */
#define RWHO 0

/* Information needed for RWHO. */
#define RWHO_INTERVAL TIME_MINUTE(4)
#define RWHO_PASSWD "potrzebie"
#define RWHO_SERVER "riemann.math.okstate.edu"

/* amount of object endowment, based on cost */
#define MAX_OBJECT_ENDOWMENT 100

/* minimum costs for various things */
#define OBJECT_COST 10          /* Amount it costs to make an object    */
#define EXIT_COST 1             /* Amount it costs to make an exit      */
#define LINK_COST 1             /* Amount it costs to make a link       */
#define ROOM_COST 10            /* Amount it costs to dig a room        */
#define LOOKUP_COST 0           /* cost to do a scan                    */
#define MAX_PENNIES 10000       /* amount at which temple gets cheap    */
#define PENNY_RATE 8            /* 1/chance of getting a penny per room */
#define START_PENNIES 50        /* # of pennies a player's created with */

/* costs of kill command */
#define KILL_BASE_COST 100      /* prob = expenditure/KILL_BASE_COST    */
#define KILL_MIN_COST 10        /* minimum amount needed to kill        */
#define KILL_BONUS 50           /* amount of "insurance" paid to victim */


#define MAX_DELTA_OBJS 20  /* max %age of objs changed before a full dump */

/* player spam input limiters */
#define COMMAND_BURST_SIZE 500  /* commands allowed per user in a burst */
#define COMMANDS_PER_TIME 2     /* commands per time slice after burst  */
#define COMMAND_TIME_MSEC 1000  /* time slice length in milliseconds    */


/* Max %of db in unchanged objects allowed to be loaded.  Generally 5% */
/* This is only needed if you defined DISKBASED in config.h */
#define MAX_LOADED_OBJS 5

/* Max # of timequeue events allowed. */
#define MAX_PROCESS_LIMIT 400

/* Max # of timequeue events allowed for any one player. */
#define MAX_PLYR_PROCESSES 32

/* Max # of instrs in uninterruptable muf programs before timeout. */
#define MAX_INSTR_COUNT 20000


/* INSTR_SLICE is the number of instructions to run before forcing a
 * context switch to the next waiting muf program.  This is for the
 * multitasking interpreter.
 */
#define INSTR_SLICE 2000


/* Max # of instrs in uninterruptable programs before timeout. */
#define MPI_MAX_COMMANDS 2048


/* PAUSE_MIN is the minimum time in milliseconds the server will pause
 * in select() between player input/output servicings.  Larger numbers
 * slow FOREGROUND and BACKGROUND MUF programs, but reduce CPU usage.
 */
#define PAUSE_MIN 100

/* FREE_FRAMES_POOL is the number of program frames that are always
 *  available without having to allocate them.  Helps prevent memory
 *  fragmentation.
 */
#define FREE_FRAMES_POOL 8




#define PLAYER_START ((dbref) 0)  /* room number of player start location */




/* Use gethostbyaddr() for hostnames in logs and the wizard WHO list. */
#define HOSTNAMES 0

/* Server support of @doing (reads the _/do prop on WHOs) */
#define WHO_DOING 1

/* To enable logging of all regular commands */
#define LOG_COMMANDS 1

/* To enable logging of all INTERACTIVE commands, (muf editor, muf READ, etc) */
#define LOG_INTERACTIVE 1

/* Log failed commands ( HUH'S ) to status log */
#define LOG_FAILED_COMMANDS 0

/* Log the text of changed programs when they are saved.  This is helpful
 * for catching people who upload scanners, use them, then recycle them. */
#define LOG_PROGRAMS 1

/* give a bit of warning before a database dump. */
#define DBDUMP_WARNING 1

/* give a bit of warning before a delta dump. */
/* only warns if DBDUMP_WARNING is also 1 */
#define DELTADUMP_WARNING 1

/* clear out unused programs every so often */
#define PERIODIC_PROGRAM_PURGE 1

/* Makes WHO unavailable before connecting to a player, or when Interactive.
 * This lets you prevent bypassing of a global @who program. */
#define SECURE_WHO 0

/* Makes all items under the environment of a room set Wizard, be controlled
 * by the owner of that room, as well as by the object owner, and Wizards. */
#define REALMS_CONTROL 0

/* Allows 'listeners' (see CHANGES file) */
#define LISTENERS 1

/* Forbid listener programs of less than given mucker level. 4 = wiz */
#define LISTEN_MLEV 3

/* Allows listeners to be placed on objects. */
#define LISTENERS_OBJ 1

/* Searches up the room environments for listeners */
#define LISTENERS_ENV 1

/* Allow mortal players to @force around objects that are set ZOMBIE. */
#define ZOMBIES 1

/* Allow only wizards to @set the VEHICLE flag on Thing objects. */
#define WIZ_VEHICLES 0

/* Force Mucker Level 1 muf progs to prepend names on notify's to players
 * other than the using player, and on notify_except's and notify_excludes. */
#define FORCE_MLEV1_NAME_NOTIFY 1

/* Define these to let players set TYPE_THING and TYPE_EXIT objects dark. */
#define EXIT_DARKING 1
#define THING_DARKING 1

/* Define this to 1 to make sleeping players effectively DARK */
#define DARK_SLEEPERS 0

/* Define this to 1 if you want DARK players to not show up on the WHO list
 * for mortals, in addition to not showing them in the room contents. */
#define WHO_HIDES_DARK 1

/* Allow a player to use teleport-to-player destinations for exits */
#define TELEPORT_TO_PLAYER 1

/* Make using a personal action require that the source room
 * be controlled by the player, or else be set JUMP_OK */
#define SECURE_TELEPORT 0

/* With this defined to 1, exits that aren't on TYPE_THING objects will */
/* always act as if they have at least a Priority Level of 1.  */
/* Define this if you want to use this server with an old db, and don't want */
/* to re-set the Levels of all the LOOK, DISCONNECT, and CONNECT actions. */
#define COMPATIBLE_PRIORITIES 1

/* With this defined to 1, Messages and Omessages run thru the MPI parser. */
/* This means that whatever imbedded MPI commands are in the strings get */
/* interpreted and evaluated.  MPI commands are sort of like MUSH functions, */
/* only possibly more bizzarre. The users will probably love it. */
#define DO_MPI_PARSING 1

/* If defined to 1, disallows local and executer MPI permissions. */
/* This prevents people from accessing the looker's properties from */
/* their @desc, etc. */
/* */
/* Used by Tapestries. */
#define MPI_STRICT_PERMISSIONS 0

/* Only allow killing people with the Kill_OK bit. */
#define RESTRICT_KILL 1

/* To have a private MUCK, this restricts player
 * creation to Wizards using the @pcreate command */
#define REGISTRATION 1

/* Define to 1 to allow _look/ propqueue hooks. */
#define LOOK_PROPQUEUES 0

/* Define to 1 to allow locks to check down the environment for props. */
#define LOCK_ENVCHECK 0

/* Define to 0 to prevent diskbasing of property values, or to 1 to allow. */
#define DISKBASE_PROPVALS 1

