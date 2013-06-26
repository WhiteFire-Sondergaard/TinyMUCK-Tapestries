/*
 * Functions to interface LUA in place of the MUF interpeter.
 *
 * (c) Peter Torkelson 2005
 */

#include "config.h"
#include "db.h"
#include "props.h"
#include "params.h"
#include "tune.h"
#include "interface.h"
#include "match.h"
#include "externs.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "mlua.h"
#include "timenode.hpp"

/*
 * PID stuff. Should be a functioncall to somewhere. This is kinda lame.
 */
extern int top_pid;

/*
 * Because it's not in db.h? :P
 */
struct line *read_program(dbref i);


/*
 * Forward declerations
 */
static int load_program(lua_State *L, dbref program, dbref player);

/*
 * Lua stack dump
 */
void mlua_dump_stack(lua_State *L, dbref player, const char *title)
{
    char buf[BUFFER_LEN]; 
    int i;
    sprintf(buf, "Dumping Lua Stack at: %s", title);
    notify_nolisten(player, buf, 1);
    for (i = 1; i <= lua_gettop(L); i++)
    {
        sprintf(buf, "%2d: %-10s: %s", i, lua_typename(L, lua_type(L, i)), lua_tostring(L, i));
        notify_nolisten(player, buf, 1);
    }
}

/*
 * State to Interpeter
 */
mlua_interp *mlua_get_interp(lua_State *L)
{
    struct mlua_interp *interp;

    /* Extract the interpeter from the state */
    lua_pushstring(L, "mlua_interp");
    lua_gettable(L, LUA_REGISTRYINDEX);
    interp = (mlua_interp *)lua_touserdata(L, -1);
    lua_pop(L, 1);

    return interp;
}

/*
 * These are some C functions callable from LUA. These will be moved
 * to their own file at some point, probably mlua_api.c. Right now
 * the few that exist are hacked into here for testing.
 */

 /* Almost copied verbatum from the lua base library, but
    load_aux was merged, and the mode is forced to "t".
    */

#define RESERVEDSLOT 5
/*
** Reader for generic `load' function: `lua_load' uses the
** stack for internal stuff, so the reader cannot change the
** stack top. Instead, it keeps its resulting string in a
** reserved slot inside the stack.
*/
static const char *generic_reader (lua_State *L, void *ud, size_t *size) {
  (void)(ud);  /* not used */
  luaL_checkstack(L, 2, "too many nested functions");
  lua_pushvalue(L, 1);  /* get function */
  lua_call(L, 0, 1);  /* call it */
  if (lua_isnil(L, -1)) {
    *size = 0;
    return NULL;
  }
  else if (!lua_isstring(L, -1))
    luaL_error(L, "reader function must return a string");
  lua_replace(L, RESERVEDSLOT);  /* save string in reserved slot */
  return lua_tolstring(L, RESERVEDSLOT, size);
}

static int mlua_lib_package_searcher(lua_State *L)
{
    int n = lua_gettop(L);
    size_t name_length;
    const char *name = lua_tolstring(L, 1, &name_length);
    dbref player;
    dbref prog;
    const char *lang;
    struct mlua_interp *interp;

    if (n != 1)
        return luaL_error(L, "mlua_lib_package_searcher(): takes 1 argument");

    /* get the player dbref from registry */
    lua_pushstring(L, "mlua_interp");
    lua_gettable(L, LUA_REGISTRYINDEX);
    interp = (mlua_interp *)lua_touserdata(L, -1);
    lua_pop(L, 1);
    if (!interp)
    {
      return luaL_error(L, "mlua_lib_package_searcher(): mlua_interp missing from registry.");
    }
    player = interp->player;

    prog = find_registered_obj(player, name);

    if (prog == NOTHING || Typeof(prog) != TYPE_PROGRAM)
        return 0;

 #ifdef COMPRESS
    lang = uncompress(get_property_class(prog, "~language"));
#else
    lang = get_property_class(prog, "~language");
#endif

    if (!lang || (strcmp(lang, "Lua") != 0))
        return luaL_error(L, "mlua_lib_package_searcher(): program is not Lua");

    if (load_program(L, prog, player))
        return 1;
    else
        return luaL_error(L, "mlua_lib_package_searcher(): module unloadable");
}

static int mlua_lib_load(lua_State *L)
{
    int status;
    size_t l;
    int top = lua_gettop(L);
    const char *s = lua_tolstring(L, 1, &l);
    const char *mode = "t";
    if (s != NULL) {  /* loading a string? */
        const char *chunkname = luaL_optstring(L, 2, s);
        status = luaL_loadbufferx(L, s, l, chunkname, mode);
    }
    else {  /* loading from a reader function */
        const char *chunkname = luaL_optstring(L, 2, "=(load)");
        luaL_checktype(L, 1, LUA_TFUNCTION);
        lua_settop(L, RESERVEDSLOT);  /* create reserved slot */
        status = lua_load(L, generic_reader, NULL, chunkname, mode);
    }
    if (status == LUA_OK && top >= 4) {  /* is there an 'env' argument */
        lua_pushvalue(L, 4);  /* environment for loaded function */
        lua_setupvalue(L, -2, 1);  /* set it as 1st upvalue */
    }

    if (status == LUA_OK)
        return 1;
    else {
        lua_pushnil(L);
        lua_insert(L, -2);  /* put before error message */
        return 2;  /* return nil plus error message */
    }
}

static int mlua_lib_print (lua_State *L) {
  int n = lua_gettop(L);  /* number of arguments */
  int i, sl, bl;
  dbref player;
  char output[BUFFER_LEN+1];
  struct mlua_interp *interp;
  int tostring;

  output[0] = 0;
  bl = 0;

  /* get the player dbref from registry */
  lua_pushstring(L, "mlua_interp");
  lua_gettable(L, LUA_REGISTRYINDEX);
  interp = (mlua_interp *)lua_touserdata(L, -1);
  lua_pop(L, 1);
  if (!interp)
  {
      // lua_pushstring(L, "lua_lib_print(): mlua_interp missing from registry.");
      // lua_error(L);
      return luaL_error(L, "lua_lib_print(): mlua_interp missing from registry.");
  }
  player = interp->player;
  assert(player >= 0 && player <= db_top && Typeof(player) == TYPE_PLAYER);

  lua_getglobal(L, "tostring");
  tostring = lua_gettop(L);

  for (i=1; i<=n; i++) {
    const char *s;
    bool need_pop = false;

    if (lua_isstring(L, i))
    {
        s = lua_tostring(L, i);
        if (s == NULL)
            return luaL_error(L, "lua_lib_print(): lua_tostring() returned NULL");
    }
    else
    {
        //mlua_dump_stack(L, player, "Before tostring.");
        lua_checkstack(L, 5);
        lua_pushvalue(L, /* -1 */ tostring);  /* function to be called */
        lua_pushvalue(L, i);   /* value to print */
        //mlua_dump_stack(L, player, "Before call.");
        lua_call(L, 1, 1);
        //mlua_dump_stack(L, player, "After call.");
        s = lua_tostring(L, -1);  /* get result */
        if (s == NULL)
          return luaL_error(L, "lua_lib_print(): `tostring' must return a string to `print'");
        need_pop = true;
    } 

    if (*s)
    {
       if (i>1 && (bl + 1 < BUFFER_LEN)) {
            strcat(output, " ");
            bl++;
        }
        sl = strlen(s);
        if (bl + sl < BUFFER_LEN) {
            strcat(output, s);
            bl += sl;
        }
        else
            return luaL_error(L, "lua_lib_print(): buffer overflow.");
    }
    if (need_pop) lua_pop(L, 1);  /* pop result */
  }

  notify_nolisten(player, output, 1);

  return 0;
}

static int mlua_lib_read (lua_State *L) {
    struct mlua_interp *interp;

    /* get the player dbref from registry */
    lua_pushstring(L, "mlua_interp");
    lua_gettable(L, LUA_REGISTRYINDEX);
    interp = (mlua_interp *)lua_touserdata(L, -1);
    lua_pop(L, 1);

    /* See if we can yield at this point */
//    if (L->nCcalls > 0)
//        return luaL_error(L, "Can not yield inside a call.");

    if (!mlua_is_foreground(interp))
        return luaL_error(L, "Can only read() in a foreground process.");

    /* Set reason for yield */
    interp->why_sleep = MLUA_READ;

    return lua_yield(L, 0);    
}

static int mlua_lib_yield (lua_State *L) {
    struct mlua_interp *interp;

    /* get the player dbref from registry */
    lua_pushstring(L, "mlua_interp");
    lua_gettable(L, LUA_REGISTRYINDEX);
    interp = (mlua_interp *)lua_touserdata(L, -1);
    lua_pop(L, 1);

    /* See if we can yield at this point */
//    if (L->nCcalls > 0)
//        return luaL_error(L, "Can not yield inside a call.");

    if (interp->mode == MLUA_CALLED)
        return luaL_error(L, "Can not yield() in a called program.");

    /* Set reason for yield */
    interp->why_sleep = MLUA_YIELD;

    return lua_yield(L, 0);    
}

static int mlua_lib_sleep (lua_State *L) {
    struct mlua_interp *interp;
    int d;

    /* get the player dbref from registry */
    lua_pushstring(L, "mlua_interp");
    lua_gettable(L, LUA_REGISTRYINDEX);
    interp = (mlua_interp *)lua_touserdata(L, -1);
    lua_pop(L, 1);

    /* See if we can yield at this point */
//    if (L->nCcalls > 0)
//        return luaL_error(L, "Can not yield inside a call.");

    if (interp->mode == MLUA_CALLED)
        return luaL_error(L, "Can not sleep() in a called program.");

    /* A little error checking... */
    d = (int)lua_tonumber(L, 1);
    luaL_argcheck(L, d > 0, 1, "positive numeric value expected");

    /* Set reason for yield */
    interp->why_sleep = MLUA_SLEEP;

    return lua_yield(L, 1);    
}


static void mlua_base_open(lua_State *l)
{
    /* remove some functions from the base library. */
    /* loadfile, dofile, require, print */

    /* Load mud global functions */
    lua_pushcfunction(l, mlua_lib_print);
    lua_setglobal(l, "print");
    lua_pushcfunction(l, mlua_lib_print);
    lua_setglobal(l, "notify");
    lua_pushcfunction(l, mlua_lib_read);
    lua_setglobal(l, "read");
    lua_pushcfunction(l, mlua_lib_yield);
    lua_setglobal(l, "yield");
    lua_pushcfunction(l, mlua_lib_sleep);
    lua_setglobal(l, "sleep");
    lua_pushcfunction(l, mlua_lib_load);
    lua_setglobal(l, "load");
    lua_pushcfunction(l, mlua_lib_load);
    lua_setglobal(l, "loadstring");

    // Need a mlua_lib_load here...
}

static int load_program(lua_State *L, dbref program, dbref player)
{
    char progname[BUFFER_LEN+1];
    int error;

    /* Create the program name for Lua */
    strcpy(progname, "@"); /* This tells lua it's a "filename". */
    strncat(progname, unparse_object(player, program), BUFFER_LEN-1);
    progname[BUFFER_LEN] = 0;

    /* oops, not compiled yet... */
    if (!DBFETCH(program)->sp.program.code)
    {
        if (!mlua_compile(player, program, TRUE))
        {
            sprintf(progname, "Program %s uncompilable!",
                unparse_object(player, program));
            notify_nolisten(player, progname, 1);
            return FALSE;
        }
    }

    /* load code */
    error = luaL_loadbuffer(L, 
                      (char *)DBFETCH(program)->sp.program.code,
                      DBFETCH(program)->sp.program.siz,
                      progname);

    /* handle any errors */
    if (error == LUA_ERRMEM || error == LUA_ERRERR)
    {
        abort();
    }
    if (error == LUA_ERRRUN)
    {
        notify_nolisten(player, "Lua Load Error!", 1);
        notify_nolisten(player, lua_tostring(L, -1), 1);
        return FALSE;
    }

    //mlua_dump_stack(L, player, "post-load");

    return TRUE;
}

/* Called to create a new interpeter and load code into it. */
/* Yea, that is a lot of arguments. :) */
struct mlua_interp *mlua_create_interp(
    dbref program,
    const char *property,
    dbref location,
    dbref player,
//    dbref output,
    dbref trigger,
    dbref euid,
    int mode,
    int event)
{
    lua_State *L;
    struct mlua_interp *interp = new struct mlua_interp;
    interp->interpeter.reset(); // redundant, but just to be sure.

    /* Do some error checking */
    if (!property && Typeof(program) != TYPE_PROGRAM) return NULL;

    /* Create a LUA engine */
    L = luaL_newstate();
    if (L == NULL)
    {
        abort();
    }

    /* Do some error checking */
    if (property)
    {
        /* loading code from properties is not supported yet */
        lua_close(L);
        return NULL;
    } else {
        if (!load_program(L, program, player))
        {
            //notify_nolisten(player, "Program not compilable.", 1);
            lua_close(L);
            return NULL;
        }
    }

    /* Setup environment */
    //mlua_dump_stack(L, player, "Before libs...");

    /* lua builtin libraries */
    luaopen_base(L); lua_remove(L, -1);
    //mlua_dump_stack(L, player, "Post base...");

    /* need to overwrite some dangerous functions provide by base... */
    lua_pushnil(L); lua_setglobal(L, "loadfile");
    lua_pushnil(L); lua_setglobal(L, "loadstring");
    lua_pushnil(L); lua_setglobal(L, "load");
    lua_pushnil(L); lua_setglobal(L, "dofile");
    lua_pushnil(L); lua_setglobal(L, "require");
    lua_pushnil(L); lua_setglobal(L, "print");
    lua_pushnil(L); lua_setglobal(L, "coroutine");
    lua_pushnil(L); lua_setglobal(L, "collectgarbage");
    luaopen_table(L); lua_remove(L, -1);
    luaopen_string(L); lua_remove(L, -1);
    luaopen_math(L); lua_remove(L, -1);

    //mlua_dump_stack(L, player, "Post lua math lib...");

    // Has to be loaded differently...
    luaL_requiref(L, "package", luaopen_package, 1);
    lua_pushnil(L); lua_setfield(L, -2, "loaders");
    lua_pushnil(L); lua_setfield(L, -2, "path");
    lua_pushnil(L); lua_setfield(L, -2, "cpath");
    lua_pushnil(L); lua_setfield(L, -2, "loaders");
    lua_pushnil(L); lua_setfield(L, -2, "searchpath");
    lua_createtable(L, 1, 0);
    lua_pushvalue(L, -2);
    lua_pushcclosure(L, mlua_lib_package_searcher, 1);
    lua_rawseti(L, -2, 1);
    lua_setfield(L, -2, "searchers");  /* put it in field 'searchers' */
    lua_pop(L, 1);
    lua_pushnil(L); lua_setglobal(L, "module"); // So broken. QQ

    //mlua_dump_stack(L, player, "Post lua package lib...");

    luaL_requiref(L, "os", luaopen_os, 1);
    lua_pushnil(L); lua_setfield(L, -2, "execute");
    lua_pushnil(L); lua_setfield(L, -2, "getenv");
    lua_pushnil(L); lua_setfield(L, -2, "exit");
    lua_pushnil(L); lua_setfield(L, -2, "remove");
    lua_pushnil(L); lua_setfield(L, -2, "rename");
    lua_pushnil(L); lua_setfield(L, -2, "setlocale");
    lua_pushnil(L); lua_setfield(L, -2, "tmpname");
    lua_pop(L, 1);

    luaL_requiref(L, "math", luaopen_math, 1);
    lua_pushnil(L); lua_setfield(L, -2, "randomseed"); // Not safe
    lua_pop(L, 1);

    luaL_requiref(L, "string", luaopen_string, 1);
    lua_pushnil(L); lua_setfield(L, -2, "dump"); // Not safe
    lua_pop(L, 1);

    luaL_requiref(L, "table", luaopen_table, 1);
    lua_pop(L, 1);

    //mlua_dump_stack(L, player, "Post lua libs...");

    /* open the mud lua libs */ 
    mlua_base_open(L);

    //mlua_dump_stack(L, player, "Post mlua base...");

    mlua_open_mdb(L); lua_remove(L, -1);

    //mlua_dump_stack(L, player, "Post libs...");

    /* create a table to store hidded data in the registery */
    lua_pushstring(L, "mlua_interp");
    lua_pushlightuserdata(L, interp);
    //interp = (mlua_interp *)lua_newuserdata(L, sizeof(struct mlua_interp));
    lua_settable(L, LUA_REGISTRYINDEX);

    //mlua_dump_stack(L, player, "Post interp...");

    /* Fill the interp table */
    interp->L = L;
    interp->pid = top_pid++;
    interp->caller = NULL;
    interp->event = event;
    interp->mode = mode;
    interp->why_sleep = MLUA_FIRSTRUN;
    interp->player = player;
    interp->euid = euid;
    interp->program = program;
    interp->trigger = trigger;
    interp->inst_count = 0;
    interp->inst_since_yield = 0;
    interp->inst_max = MLUA_INST_MAX;
    interp->inst_try_yield = MLUA_INST_TRY_YIELD;
    interp->inst_max_noyield = MLUA_INST_MAX_NOYIELD;
    /* Add a _gc handler at some point so we can fill these. */
    interp->command = NULL;
    interp->prop = NULL;

    /* Provide arguments if the program is running as a command */
    if (*match_cmdname)
    {
        lua_pushstring(interp->L, match_cmdname);
        lua_setglobal(interp->L, "command");
    }

    if (*match_args)
    {
        lua_pushstring(interp->L, match_args);
        lua_setglobal(interp->L, "args");
    }

    return interp;
}

/* Resum or start a lua program. */
std::tr1::shared_ptr<InterpeterReturnValue> mlua_resume(struct mlua_interp *interp, const char *resume_arg)
{
    int error;
    int nargs = 0;
    //lua_State *L;
    std::tr1::shared_ptr<InterpeterReturnValue> ret_val;

    if (!interp) return ret_val;

    lua_State *Lrunning = interp->L;

    /* The program may be passed an arg when it starts or when resuming
     * from a READ event.
     */
    if (resume_arg)
    {
        lua_pushstring(Lrunning, resume_arg);
        nargs = 1;
    }

    //mlua_dump_stack(Lrunning, interp->player, "in resume()");

    /* reset sleeping flag */
    interp->why_sleep = MLUA_NOTSLEEPING;

    /* Resume execution */
    if (interp->mode != MLUA_CALLED)
    {
        /* It it was not a call, then it's a thread. */
        error = lua_resume(Lrunning, NULL, nargs);
    }
    else
    {
        /* Calls are not threads because they can not suspend. */
        //L = interp->L;
        error = lua_pcall(Lrunning, nargs, /*nresults*/1, /*errfunc*/0);
    }

    /* handle any errors */
    if (error == LUA_ERRMEM || error == LUA_ERRERR)
    {
        abort();
    }
    if (error == LUA_ERRRUN)
    {
        notify_nolisten(interp->player, "Lua Runtime Error:", 1);
        notify_nolisten(interp->player, lua_tostring(Lrunning, -1), 1);
        //mlua_free_interp(interp); /* Interpeter now self-destructs */
        return ret_val;
    }

    /* Now, do we suspend again, or are we done? */
    switch (interp->why_sleep)
    {
        case MLUA_READ:
            interp->inst_since_yield = 0;
            DBSTORE(interp->player, sp.player.curr_prog, interp->program);
            DBSTORE(interp->player, sp.player.block, 0);
            add_prog_read_event(interp->player, interp->program, 
                interp->interpeter.lock(), interp->trigger);
            break;

        case MLUA_YIELD:
        case MLUA_FYIELD:
            interp->inst_since_yield = 0;
            DBSTORE(interp->player, sp.player.block, 
                            (mlua_is_foreground(interp)));
            add_prog_delay_event(0, interp->player, NOTHING, NOTHING,
                  interp->program, interp->interpeter.lock(),
                  (mlua_is_foreground(interp)) ? "FOREGROUND" : "BACKGROUND");
            break;

        case MLUA_SLEEP:
            interp->inst_since_yield = 0;
            DBSTORE(interp->player, sp.player.block, 
                            (mlua_is_foreground(interp)));
            add_prog_delay_event(lua_tonumber(Lrunning, 1),
                interp->player, NOTHING, NOTHING, interp->program, interp->interpeter.lock(),
                "SLEEPING");
            break;

        case MLUA_NOTSLEEPING:
        case MLUA_FIRSTRUN:
        default:
            // ret_val = 
            /* We are done. */
            //mlua_free_interp(interp);
            break;
    }

    // LUA_TNIL, LUA_TNUMBER, LUA_TBOOLEAN, LUA_TSTRING, LUA_TTABLE, 
    // LUA_TFUNCTION, LUA_TUSERDATA, LUA_TTHREAD, and LUA_TLIGHTUSERDATA.
    switch (lua_type(Lrunning, -1))
    {
        case LUA_TBOOLEAN:
            ret_val = std::tr1::shared_ptr<InterpeterReturnValue>(new InterpeterReturnValue(
                    InterpeterReturnValue::BOOL,
                    lua_toboolean(Lrunning, -1)
                    ));
            break;

        case LUA_TNUMBER:
            ret_val = std::tr1::shared_ptr<InterpeterReturnValue>(new InterpeterReturnValue(
                    InterpeterReturnValue::INTEGER,
                    lua_tointeger(Lrunning, -1)
                    ));
            break;

        case LUA_TSTRING:
            ret_val = std::tr1::shared_ptr<InterpeterReturnValue>(new InterpeterReturnValue(
                    InterpeterReturnValue::STRING,
                    lua_tostring(Lrunning, -1)
                    ));
            break;
    }


    return ret_val;
}

/* Called to start interpetation */
int mlua_run(
    dbref program,
    const char *property,
    dbref location,
    dbref player,
//    dbref output,
    dbref trigger,
    dbref euid,
    int mode,
    int event)
{
    struct mlua_interp *interp;

    /* Create the interpeter */
    interp = mlua_create_interp(
        program,
        property,
        location,
        player,
//        output,
        trigger,
        euid,
        mode,
        event);

    /* Handle failure */
    if (!interp) return FALSE;

    /* execute code */
    return mlua_resume(interp, NULL) ? TRUE : FALSE;
}

/*
 * Free a compiled block from the DB...
 */
void mlua_decompile(dbref program)
{
    void *code;

    /* WTF */
    if (Typeof(program) != TYPE_PROGRAM) return;

    /* See if there is anything to free, and free it */    
    code = DBFETCH(program)->sp.program.code;
    if (code) free(code);

    /* Clear db referece */
    DBFETCH(program)->sp.program.code = NULL;
    DBFETCH(program)->sp.program.siz = 0;
}

/*
 * Clean up an unused interpeter
 */
void mlua_free_interp(struct mlua_interp *interp)
{
    /* UnBlock player */
    if (interp->mode == MLUA_FOREGROUND)
        DBFETCH(interp->player)->sp.player.block = 0;

    /* Free strings (properly, this should be done as a _gc method)  */
    if (interp->command) free(interp->command);
    if (interp->prop) free(interp->prop);

    /* Free up our pointer to the interpeter wrapper */
    interp->interpeter.reset();

    /* The interpeter will garbage collect the structure itself */
    lua_close(interp->L);
}

/*
 * Structure required by the following function to keep state in the
 * chunk loader.
 */

/* for reading */
struct chunk_state {
    struct line *curr_line; /* Muck linked list line structure. */
    int fed_line;          /* have we fed this line yet? */
};

/* for writing */
struct bytecode_chunk {
    void *block;
    size_t size;
};

/* function prototype for chunk reader and writer */
static const char *chunk_reader(lua_State *l, void *data, size_t *size);
static int chunk_writer(lua_State *l, const void *p, size_t size, void *ud);

/*
 * Function to bytecode compule a LUA program.
 */
int mlua_compile(dbref player, dbref program, int silent)
{
    lua_State *l;
    struct chunk_state state;    /* for reader */
    struct bytecode_chunk chunk; /* for writer */
    int error;                
    char msg_buff[1024];         /* For formatted messages */
    char program_text_loaded = FALSE; /* so we remeber to clear it */
    char progname[BUFFER_LEN];

    /* Create the program name for Lua */
    strcpy(progname, "@"); /* This tells lua it's a "filename". */
    strcat(progname, unparse_object(player, program));

    /* load the muf file if it's not already loaded... */
    if (!DBFETCH(program)->sp.program.first)
    {
        DBFETCH(program)->sp.program.first = read_program(program);
        program_text_loaded = TRUE;
    }

    /* Seed the first line into the state structure. */
    state.curr_line = DBFETCH(program)->sp.program.first;
    state.fed_line = FALSE;

    /* If there is no first line, the program has no text to compile. */
    if (!state.curr_line)
    {
        if (!silent) notify_nolisten(player, "No program text!", 1);
        return FALSE;
    }

    /* Create a LUA engine */
    l = luaL_newstate();
    if (l == NULL)
    {
        /* failure here means something very bad happened. */
        abort();
    } 

    /* Attempt to bytecompile program */
    error = lua_load(l, chunk_reader, &state, progname, "t");

    if (!error)
    {
        if (!silent) notify_nolisten(player, "Successful Compile.", 1);
    }
    else if (error == LUA_ERRSYNTAX)
    {
        if (!silent) notify_nolisten(player, lua_tostring(l, -1), 1);
    }
    else
    {
        /* lua_load returned something unexpected */
        abort();
    }

    /* Save the bytecode to the DB engine. */
    if (!error)
    {
        /* Setup userdata for creating block */
        chunk.size = 0;
        chunk.block = NULL;

        /* dump to new block */ 
        lua_dump(l, chunk_writer, &chunk);

        /* Free up the old code, if any. */
        mlua_decompile(program);

        /* Save it ... */
        DBFETCH(program)->sp.program.code = (inst *)chunk.block;
        DBFETCH(program)->sp.program.siz = chunk.size;

        /* Debug stuff */
        if (!silent)
        {
            sprintf(msg_buff, "Bytecode is %d bytes.", chunk.size);
            notify_nolisten(player, msg_buff, 1);
        }
    }

    /* Destroy a the engine */    
    lua_close(l);

    /* If we loaded the program text, delete it. */
    if (program_text_loaded)
    {
        free_prog_text(DBFETCH(program)->sp.program.first);
        DBSTORE(program, sp.program.first, NULL);
    }

    /* return status */
    return !error;
}

/*
 * This is code to implement the functions necisary to feed a MUF program
 * into the Lua compiler.
 */
static const char *chunk_reader(lua_State *l, void *data, size_t *size)
{
    struct chunk_state *state = (struct chunk_state *)data;

    if (state->fed_line)
    {
        /* We have already fed this line to Lua, feed a CR and advance
           to next line */
        state->curr_line = state->curr_line->next;
        state->fed_line = FALSE;
        *size = 1;
        return "\n";
    }
    else if (state->curr_line == NULL)
    {
        /* out of text */
        *size = 0;
        return NULL;
    }
    else
    {
        /* Feed line and note that we have done so. */
        state->fed_line = TRUE;
        *size = strlen(state->curr_line->this_line);
        return state->curr_line->this_line;
    }
}

/* Create a btyecode block */
static int chunk_writer(lua_State *l, const void *p, size_t size, void *ud)
{
    struct bytecode_chunk *chunk = (struct bytecode_chunk *)ud;

    /* Size block to chunk */
    if (!chunk)
    {
        chunk->block = malloc(size);
    } else {
        chunk->block = realloc(chunk->block, chunk->size + size);
    }

    /* Copy data */
    memcpy((char *)chunk->block + chunk->size, p, size);

    /* Set size */
    chunk->size += size;

    /* Hell if I know what they want as a return, it's not documented */
    return 0;
}

