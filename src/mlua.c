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

#include "mlua.h"
#include "timenode.hpp"

/* Stuff from timequeue.c */
int add_lua_read_event(dbref player, dbref prog, struct mlua_interp *interp);
int add_lua_delay_event(int delay, dbref player, dbref loc, dbref trig,
                dbref prog, struct mlua_interp *interp, const char *mode);

/*
 * PID stuff. Should be a functioncall to somewhere. This is kinda lame.
 */
extern int top_pid;

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
static int mlua_lib_print (lua_State *L) {
  int n = lua_gettop(L);  /* number of arguments */
  int i, sl, bl;
  dbref player;
  char output[BUFFER_LEN+1];
  struct mlua_interp *interp;

  output[0] = 0;
  bl = 0;

  /* get the player dbref from registry */
  lua_pushstring(L, "mlua_interp");
  lua_gettable(L, LUA_REGISTRYINDEX);
  interp = (mlua_interp *)lua_touserdata(L, -1);
  lua_pop(L, 1);
  if (!interp)
  {
      lua_pushstring(L, "lua_lib_print(): mlua_interp missing from registry.");
      lua_error(L);
  }
  player = interp->player;

  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    lua_pushvalue(L, -1);  /* function to be called */
    lua_pushvalue(L, i);   /* value to print */
    lua_call(L, 1, 1);
    s = lua_tostring(L, -1);  /* get result */
    if (s == NULL)
      return luaL_error(L, "`tostring' must return a string to `print'");
    if (i>1 && (bl + 1 < BUFFER_LEN)) {
        strcat(output, " ");
        bl++;
    }
    sl = strlen(s);
    if (bl + sl < BUFFER_LEN) {
        strcat(output, s);
    }    
    lua_pop(L, 1);  /* pop result */
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
    struct mlua_interp *interp;

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
            lua_close(L);
            return NULL;
        }
    }

    /* Setup environment */

    /* lua builtin libraries */
    luaopen_base(L);
    /* need to overwrite some dangerous functions provide by base... */
    lua_pushnil(L); lua_setglobal(L, "loadfile");
    lua_pushnil(L); lua_setglobal(L, "dofile");
    lua_pushnil(L); lua_setglobal(L, "require");
    lua_pushnil(L); lua_setglobal(L, "print");
    lua_pushnil(L); lua_setglobal(L, "coroutine");
    luaopen_table(L);
    luaopen_string(L);
    luaopen_math(L);

    /* open the mud lua libs */ 
    mlua_base_open(L);
    mlua_open_mdb(L);

    /* create a table to store hidded data in the registery */
    lua_pushstring(L, "mlua_interp");
    interp = (mlua_interp *)lua_newuserdata(L, sizeof(struct mlua_interp));
    lua_settable(L, LUA_REGISTRYINDEX);

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
    if (!*match_cmdname)
    {
        lua_pushstring(interp->L, match_cmdname);
        lua_setglobal(interp->L, "command");
    }

    if (!*match_args)
    {
        lua_pushstring(interp->L, match_args);
        lua_setglobal(interp->L, "args");
    }

    return interp;
}

/* Resum or start a lua program. */
int mlua_resume(struct mlua_interp *interp, const char *resume_arg)
{
    int error;
    int nargs = 0;
    lua_State *L;
    lua_State *Lrunning = interp->L;

    /* The program may be passed an arg when it starts or when resuming
     * from a READ event.
     */
    if (resume_arg)
    {
        lua_pushstring(Lrunning, resume_arg);
        nargs = 1;
    }

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
        L = interp->L;
        error = lua_pcall(Lrunning, nargs, /*nresults*/0, /*errfunc*/0);
    }

    /* handle any errors */
    if (error == LUA_ERRMEM || error == LUA_ERRERR)
    {
        abort();
    }
    if (error == LUA_ERRRUN)
    {
        notify_nolisten(interp->player, lua_tostring(Lrunning, -1), 1);
        mlua_free_interp(interp); /* Interpeter now self-destructs */
        return FALSE;
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
            /* We are done. */
            mlua_free_interp(interp);
            break;
    }

    return TRUE;
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
    return mlua_resume(interp, NULL);
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
        DBFETCH(program)->sp.program.first =
                (struct line *) read_program(program);
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

