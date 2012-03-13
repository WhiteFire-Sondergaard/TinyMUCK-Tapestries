/* $Header: /home/foxen/CR/FB/include/RCS/externs.h,v 1.1 1996/06/17 17:29:45 foxen Exp foxen $
 * 
 * $Log: externs.h,v $
 * Revision 1.1  1996/06/17 17:29:45  foxen
 * Initial revision
 *
 * Revision 5.15  1994/03/19  06:56:39  foxen
 * strdup -> string_dup
 *
 * Revision 5.14  1994/02/13  13:58:53  foxen
 * Bugfix for compiling with MALLOC_PROFILING, with Strdup.
 *
 * Revision 5.13  1994/02/11  05:55:59  foxen
 * memory monitoring code and memory cleanup mods.
 *
 * Revision 5.12  1994/01/06  03:16:30  foxen
 * Version 5.12
 *
 * Revision 5.1  1993/12/17  00:35:54  foxen
 * initial revision.
 *
 * Revision 1.12  90/09/18  08:06:03  rearl
 * Added hash tables for players and primitives.
 * 
 * Revision 1.11  90/09/13  06:33:47  rearl
 * do_dump() and do_toad() prototypes changed.
 * 
 * Revision 1.10  90/09/10  02:24:17  rearl
 * Changed NL line termination to CR/LF pairs.
 * 
 * Revision 1.9  90/09/01  06:03:56  rearl
 * Put do_drop() back like it was.
 * 
 * Revision 1.8  90/08/27  14:06:53  rearl
 * Added support for environments: @trace, mods to drop and @dig commands.
 * 
 * Revision 1.7  90/08/15  03:11:00  rearl
 * Messed around with pronoun_substitute, just like in stringutil.c.
 * 
 * Revision 1.6  90/08/06  02:47:24  rearl
 * Put can_link() back, added restricted() from predicates.c.
 * 
 * Revision 1.5  90/07/30  00:10:47  rearl
 * Added @owned command.
 * 
 * Revision 1.4  90/07/29  17:18:22  rearl
 * Fixed the functions containing the = bug.
 * 
 * Revision 1.3  90/07/20  11:43:06  casie
 * Added new function declarations for log.c
 * 
 * Revision 1.2  90/07/19  23:14:18  casie
 * Removed log comments from top.
 * 
 */
#include "copyright.h"

#include "db.h"

/* Prototypes for externs not defined elsewhere */

extern char match_args[];
extern char match_cmdname[];

/* from event.c */
extern long next_muckevent_time();
extern void next_muckevent();

/* from timequeue.c */
extern void handle_read_event(dbref player, const char *command);
extern int add_muf_read_event(dbref player, dbref prog, struct frame *fr);
extern int add_muf_queue_event(dbref player, dbref loc, dbref trig, dbref prog,
                    const char *argstr, const char *cmdstr, int listen_p);
extern int add_event(int event_type, int subtyp, int dtime, dbref player,
		    dbref loc, dbref trig, dbref program, struct frame * fr,
		    const char *strdata, const char *strcmd, const char *str3);
extern void next_timequeue_event();
extern int  in_timequeue(int pid);
extern long next_event_time();
extern void list_events(dbref program);
extern int  dequeue_prog(dbref program, int sleeponly);
extern int  dequeue_process(int procnum);
extern int  control_process(dbref player, int procnum);
extern void do_dequeue(dbref player, const char *arg1);
extern void propqueue(dbref player, dbref where, dbref trigger, dbref what,
	    dbref xclude, const char *propname, const char *toparg,
	    int mlev, int mt);
extern void envpropqueue(dbref player, dbref where, dbref trigger, dbref what,
	    dbref xclude, const char *propname, const char *toparg,
	    int mlev, int mt);

/* from db.c */
extern int  number(const char *s);
extern void putproperties(FILE *f, int obj);
extern void getproperties(FILE *f, int obj);
extern void free_line(struct line *l);
extern void db_free_object(dbref i);
extern void db_clear_object(dbref i);
extern void macrodump(struct macrotable *node, FILE *f);
extern void macroload();
extern void free_prog_text(struct line * l);
extern struct line *read_program(dbref i);
extern void write_program(struct line * first, dbref i);

/* From create.c */
extern void do_open(dbref player, const char *direction, const char *linkto);
extern void do_link(dbref player, const char *name, const char *room_name);
extern void do_dig(dbref player, const char *name, const char *pname);
extern void do_create(dbref player, char *name, char *cost);
extern void do_prog(dbref player, const char *name);
extern int unset_source(dbref player, dbref loc, dbref action);
extern int link_exit(dbref player, dbref exit, char *dest_name, dbref * dest_list);

/* from edit.c */
extern struct macrotable
    *new_macro(const char *name, const char *definition, dbref player);
extern char *macro_expansion(struct macrotable *node, const char *match);
extern void match_and_list(dbref player, const char *name, char *linespec);
extern void do_list(dbref player, dbref program, int oarg[], int argc);

/* From game.c */
extern void do_dump(dbref player, const char *newfile);
extern void do_shutdown(dbref player);

/* From hashtab.c */
extern hash_data *find_hash(const char *s, hash_tab *table, unsigned size);
extern hash_entry *add_hash(const char *name, hash_data data, hash_tab *table,
                            unsigned size);
extern int free_hash(const char *name, hash_tab *table, unsigned size);
extern void kill_hash(hash_tab *table, unsigned size, int freeptrs);

/* From help.c */
extern void spit_file(dbref player, const char *filename);
extern void do_help(dbref player, char *topic, char *seg);
extern void do_mpihelp(dbref player, char *topic, char *seg);
extern void do_news(dbref player, char *topic, char *seg);
extern void do_man(dbref player, char *topic, char *seg);
extern void do_motd(dbref player, char *text);
extern void do_info(dbref player, const char *topic, const char *seg);

/* From look.c */
extern void look_room(dbref player, dbref room, int verbose);
/* extern void look_room_simple(dbref player, dbref room); */
extern void do_look_around(dbref player);
extern void do_look_at(dbref player, const char *name, const char *detail);
extern void do_examine(dbref player, const char *name, const char *dir);
extern void do_inventory(dbref player);
extern void do_find(dbref player, const char *name, const char *flags);
extern void do_owned(dbref player, const char *name, const char *flags);
extern void do_trace(dbref player, const char *name, int depth);
extern void do_entrances(dbref player, const char *name, const char *flags);
extern void do_contents(dbref player, const char *name, const char *flags);
extern void exec_or_notify(dbref player, dbref thing, const char *message,
			   const char *whatcalled);

/* From move.c */
extern void moveto(dbref what, dbref where);
extern void enter_room(dbref player, dbref loc, dbref exit);
extern void send_home(dbref thing, int homepuppet);
extern int parent_loop_check(dbref source, dbref dest);
extern int can_move(dbref player, const char *direction, int lev);
extern void do_move(dbref player, const char *direction, int lev);
extern void do_get(dbref player, const char *what, const char *obj);
extern void do_drop(dbref player, const char *name, const char *obj);
extern void do_recycle(dbref player, const char *name);
extern void recycle(dbref player, dbref thing);

/* From player.c */
extern dbref lookup_player(const char *name);
extern void do_password(dbref player, const char *old, const char *newobj);
extern void add_player(dbref who);
extern void delete_player(dbref who);
extern void clear_players(void);

/* From predicates.c */
extern int can_link_to(dbref who, object_flag_type what_type, dbref where);
extern int can_link(dbref who, dbref what);
extern int could_doit(dbref player, dbref thing);
extern int can_doit(dbref player, dbref thing, const char *default_fail_msg);
extern int can_see(dbref player, dbref thing, int can_see_location);
extern int controls(dbref who, dbref what);
extern int controls_nowizperm(dbref who, dbref what);
extern int controls_link(dbref who, dbref what);
extern int may_read(dbref who, dbref what);
extern int may_read_nowizperm(dbref who, dbref what);
extern int restricted(dbref player, dbref thing, object_flag_type flag);
extern int payfor(dbref who, int cost);
extern int ok_name(const char *name);

/* From rob.c */
extern void do_kill(dbref player, const char *what, int cost);
extern void do_give(dbref player, const char *recipient, int amount);
extern void do_rob(dbref player, const char *what);

/* From set.c */
extern void do_name(dbref player, const char *name, char *newname);
extern void do_describe(dbref player, const char *name, const char *description);
extern void do_idescribe(dbref player, const char *name, const char *description);
extern void do_fail(dbref player, const char *name, const char *message);
extern void do_success(dbref player, const char *name, const char *message);
extern void do_drop_message(dbref player, const char *name, const char *message);
extern void do_osuccess(dbref player, const char *name, const char *message);
extern void do_ofail(dbref player, const char *name, const char *message);
extern void do_odrop(dbref player, const char *name, const char *message);
extern int setlockstr(dbref player, dbref thing, const char *keyname);
extern void do_lock(dbref player, const char *name, const char *keyname);
extern void do_unlock(dbref player, const char *name);
extern void do_unlink(dbref player, const char *name);
extern void do_chown(dbref player, const char *name, const char *newobj);
extern void do_set(dbref player, const char *name, const char *flag);

/* From speech.c */
extern void do_wall(dbref player, const char *message);
extern void do_gripe(dbref player, const char *message);
extern void do_say(dbref player, const char *message);
extern void do_page(dbref player, const char *arg1, const char *arg2);
extern void notify_listeners(dbref who, dbref xprog, dbref obj, dbref room, const char *msg, int isprivate);
extern void notify_except(dbref first, dbref exception, const char *msg, dbref who);

/* From stringutil.c */
extern int alphanum_compare(const char *s1, const char *s2);
extern int string_compare(const char *s1, const char *s2);
extern int string_prefix(const char *string, const char *prefix);
extern const char *string_match(const char *src, const char *sub);
extern char *pronoun_substitute(dbref player, const char *str);
extern char *intostr(int i);
extern const char *unmangle(dbref player, char *s);

#if !defined(MALLOC_PROFILING)
extern char *string_dup(const char *s);
#endif


/* From utils.c */
extern int member(dbref thing, dbref list);
extern dbref remove_first(dbref first, dbref what);

/* From wiz.c */
extern void do_teleport(dbref player, const char *arg1, const char *arg2);
extern void do_force(dbref player, const char *what, char *command);
extern void do_stats(dbref player, const char *name);
extern void do_toad(dbref player, const char *name, const char *recip);
extern void do_boot(dbref player, const char *name);
extern void do_pcreate(dbref player, const char *arg1, const char *arg2);
extern void do_usage(dbref player);

/* From boolexp.c */
extern int eval_boolexp(dbref player, struct boolexp *b, dbref thing);
extern struct boolexp *parse_boolexp(dbref player, const char *string, int dbloadp);
extern struct boolexp *copy_bool(struct boolexp *old);
extern struct boolexp *getboolexp(FILE *f);
extern struct boolexp *negate_boolexp(struct boolexp *b);
extern void free_boolexp(struct boolexp *b);

/* From unparse.c */
extern const char *unparse_object(dbref player, dbref object);
extern const char *unparse_boolexp(dbref player, struct boolexp *b, int fullname);
extern const char *unparse_flags(dbref thing);


/* From compress.c */
#ifdef COMPRESS
extern void save_compress_words_to_file(FILE *f);
extern void init_compress_from_file(FILE *dicto);
extern const char *compress(const char *);
extern const char *uncompress(const char *);
extern void init_compress(void);
#endif /* COMPRESS */

/* From edit.c */
extern void interactive(dbref player, const char *command);

/* From compile.c */
extern void uncompile_program(dbref i);
extern void do_uncompile(dbref player);
extern void free_unused_programs(void);
extern void do_compile(dbref in_player, dbref in_program);
extern void clear_primitives(void);
extern void init_primitives(void);

/* From interp.c */
extern struct inst *interp_loop(dbref player, dbref program,
				struct frame *fr, int rettyp);
extern struct inst *interp(dbref player, dbref location, dbref program,
			   dbref source, int nosleeping, int whichperms,
			   int rettyp);

/* declared in log.c */
extern void log2file(char *myfilename, char *format, ...);
extern void log_error(char *format, ...);
extern void log_gripe(char *format, ...);
extern void log_muf(char *format, ...);
extern void log_conc(char *format, ...);
extern void log_status(char *format, ...);
extern void log_other(char *format, ...);
extern void notify_fmt(dbref player, char *format, ...);
extern void log_program_text(struct line * first, dbref player, dbref i);

/* From timestamp.c */
extern void ts_newobject(struct object *thing);
extern void ts_useobject(dbref thing);
extern void ts_lastuseobject(dbref thing);
extern void ts_modifyobject(dbref thing);

/* From smatch.c */
extern int  equalstr(char *s, char *t);

extern void CrT_summarize( dbref player );

extern long get_tz_offset();

extern int force_level;

extern void do_credits(dbref player);

extern void disassemble(dbref player, dbref program);

/* For MPI profiling */
extern time_t mpi_prof_start_time;
/* For select idle profiling */
extern time_t sel_prof_start_time;
extern long sel_prof_idle_sec;
extern long sel_prof_idle_usec;
extern unsigned long sel_prof_idle_use;


