/*
  Interpeter abstraction by WhiteFire Sondergaard for Tapestries MUCK.
*/

#include "copyright.h"
#include "config.h"
#include "params.h"
#include "match.h"

#include "db.h"
#include "tune.h"
#include "mpi.h"
#include "mlua.h"
#include "props.h"
#include "interface.h"
#include "externs.h"
#include "interpeter.h"

#include <stdio.h>
#include <sys/types.h>
#include <time.h>

/* --------------------------------------------------------------------------------------
    Interpeter stuff
*/
/* Factory to create subclasses */
std::tr1::shared_ptr<Interpeter> Interpeter::create_interp(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, const char *property)
{
#ifdef COMPRESS
    const char *lang = uncompress(get_property_class(program, "~language"));
#else
    const char *lang = get_property_class(program, "~language");
#endif
    std::tr1::shared_ptr<Interpeter> ret_interp;

    if (!MLevel(program) || !MLevel(OWNER(program)) ||
        ((source != NOTHING) && !TrueWizard(OWNER(source)) &&
            !can_link_to(OWNER(source), TYPE_EXIT, program))) 
    {
        notify_nolisten(player, "Program call: Permission denied.", 1);
        return ret_interp;
    }

    if (lang && (strcmp(lang, "Lua") == 0)) // Lua
    {
        ret_interp = std::tr1::shared_ptr<Interpeter>(new LuaInterpeter(player, location, program, source, 
            nosleeps, whichperms, event, property));
    }
    else // MUF
    {
        ret_interp = std::tr1::shared_ptr<Interpeter>(new MUFInterpeter(player, location, program, source, 
            nosleeps, whichperms, event, property));
    }

    return ret_interp;
}



/* --------------------------------------------------------------------------------------
    Lua
*/
LuaInterpeter::LuaInterpeter(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, const char *property)
    : Interpeter(event, player)
{
    int mode;
    dbref euid = OWNER(program);

    switch(nosleeps)
    {
    case PREEMPT:
        mode = MLUA_CALLED;
        break;

    case FOREGROUND:
        mode = MLUA_FOREGROUND;
        break;

    case BACKGROUND:
        mode = MLUA_BACKGROUND;
        break;

    default:
        mode = MLUA_CALLED;
        break;
    }

    this->totaltime.tv_sec = 0;
    this->totaltime.tv_usec = 0;

    this->started = 0;

    this->fr = mlua_create_interp(program, property, location, player, 
            source, euid, mode, event);
    this->fr->interpeter = shared_from_this();
}

LuaInterpeter::~LuaInterpeter()
{
    mlua_free_interp(this->fr);
}

const char *LuaInterpeter::type()
{
    return "Lua";
}

bool LuaInterpeter::background()
{
    return this->fr->mode == MLUA_BACKGROUND;
}

void LuaInterpeter::handle_read_event(const char *command)
{
    this->resume(command);
    // Um. What do we do now? Are we done? How do we know?
}

void LuaInterpeter::resume(const char *str)
{
    this->totaltime_start();
    mlua_resume(this->fr, str);
    this->totaltime_stop();
}

void LuaInterpeter::totaltime_start()
{
    gettimeofday(&this->proftime, NULL);
    if (!this->started)
        this->started = time(NULL);
}

time_t LuaInterpeter::get_started()
{
    return this->started;
}

long LuaInterpeter::get_instruction_count()
{
    return 0L;
}

void LuaInterpeter::totaltime_stop()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    tv.tv_usec -= this->proftime.tv_usec;
    tv.tv_sec -= this->proftime.tv_sec;
    if (tv.tv_usec < 0) {
        tv.tv_usec += 1000000;
        tv.tv_sec -= 1;
    }

    this->totaltime.tv_sec += tv.tv_sec;
    this->totaltime.tv_usec += tv.tv_usec;
    if (this->totaltime.tv_usec > 1000000) {
        this->totaltime.tv_usec -= 1000000;
        this->totaltime.tv_sec += 1;
    }
}

struct timeval *LuaInterpeter::get_totaltime()
{
    return &this->totaltime;
}

/* --------------------------------------------------------------------------------------
    MUF
*/
MUFInterpeter::MUFInterpeter(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, const char *property)
    : Interpeter(event, player)
{
    this->fr = create_interp_frame(player, location, program, source, nosleeps, whichperms);
    this->player = player;
    this->program = program;
    this->fr->interpeter = shared_from_this();
}

MUFInterpeter::MUFInterpeter(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, const char *property, 
       struct frame *new_frame)
    : Interpeter(event, player)
{
    this->fr = new_frame;
    this->fr->interpeter = shared_from_this();
}

const char *MUFInterpeter::type()
{
    return "MUF";
}

struct timeval *MUFInterpeter::get_totaltime()
{
    return &this->fr->totaltime;
}

bool MUFInterpeter::background()
{
    return this->fr->multitask == BACKGROUND;
}

time_t MUFInterpeter::get_started()
{
    return (time_t)this->fr->started;
}

long MUFInterpeter::get_instruction_count()
{
    return this->fr->instcnt;
}

/* Checks the MUF timequeue for address references on the stack or */
/* dbref references on the callstack */
bool MUFInterpeter::has_refs(dbref program)
{
    int loop;
    if (!(this->fr) ||
            Typeof(program) != TYPE_PROGRAM ||
            !(DBFETCH(program)->sp.program.instances))
        return 0;

    for (loop = 1; loop < this->fr->caller.top; loop++) {
        if (this->fr->caller.st[loop] == program)
            return 1;
    }

    for (loop = 0; loop < this->fr->argument.top; loop++) {
        if (this->fr->argument.st[loop].type == PROG_ADD &&
                this->fr->argument.st[loop].data.addr->progref == program)
            return 1;
    }

    return 0;
}

bool MUFInterpeter::get_number_of_references(dbref program)
{
    int refs = 0, loop;

    if (!this->fr)
        return 0;

    for (loop = 1; loop < this->fr->caller.top; loop++) {
        if (this->fr->caller.st[loop] == program)
        refs++;
    }
    for (loop = 0; loop < this->fr->argument.top; loop++) {
        if (this->fr->argument.st[loop].type == PROG_ADD &&
                this->fr->argument.st[loop].data.addr->progref == program)
            refs++;
    }

    return refs;
}

MUFInterpeter::~MUFInterpeter()
{
    // Release people from blocking mode...
    if (this->fr->multitask != BACKGROUND)
        DBFETCH(this->uid)->sp.player.block = 0;

    // Delete interpeter
    prog_clean(this->fr);
}

void MUFInterpeter::handle_read_event(const char *command)
{
    if (this->fr->brkpt.debugging && !this->fr->brkpt.isread) {

        /* We're in the MUF debugger!  Call it with the input line. */
        if(muf_debugger(this->player, this->program, command, this->fr)) {

            /* MUF Debugger exited.  Free up the program frame & exit */
            prog_clean(this->fr);
            return;
        }

        /*
         * When using the MUF Debugger, the debugger will set the
         * INTERACTIVE bit on the user, if it does NOT want the MUF
         * program to resume executing.
         */
        if (!(FLAGS(player) & INTERACTIVE)) {
            this->resume(NULL);
        }

    } else {
        this->resume(command);
    }
}

void MUFInterpeter::resume(const char *str)
{
    if (str)
    {
        if (this->fr->argument.top >= STACK_SIZE) {

            /*
             * Uh oh! That MUF program's stack is full!
             * Print an error, free the frame, and exit.
             */
            notify_nolisten(this->player, "Program stack overflow.", 1);
            prog_clean(this->fr);
            return;
        }
        
        /*
         * Place the string on the stack
         */
        this->fr->argument.st[this->fr->argument.top].type = PROG_STRING;
        this->fr->argument.st[this->fr->argument.top++].data.string =
            alloc_prog_string(str);
    }

    interp_loop(this->player, this->program, this->fr, 0);
}

/* --------------------------------------------------------------------------------------
    InterpeterReturnValue stuff
*/
InterpeterReturnValue::InterpeterReturnValue(const int type, const int num)
{
    return_type = type;
    v_num = num;
    v_str = NULL;
}

InterpeterReturnValue::InterpeterReturnValue(const int type, const char *str)
{
    return_type = type;
    v_num = 0;
    v_str = alloc_string(str);
}

InterpeterReturnValue::~InterpeterReturnValue()
{
    if (v_str) free(v_str);
}

const char *InterpeterReturnValue::String()
{
    return v_str;
}

const int InterpeterReturnValue::Type()
{
    return return_type;
}

const int InterpeterReturnValue::Bool()
{
    return v_num != 0;
}

const int InterpeterReturnValue::Dbref()
{
    return v_num;
}

const int InterpeterReturnValue::Number()
{
    return v_num;
}



