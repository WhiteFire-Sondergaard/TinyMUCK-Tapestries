/*
  Interpreter abstraction by WhiteFire Sondergaard for Tapestries MUCK.
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
#include "interpreter.h"

#include <stdio.h>
#include <sys/types.h>
#include <time.h>

/* --------------------------------------------------------------------------------------
    Interpreter stuff
*/
/* Factory to create subclasses */
std::tr1::shared_ptr<Interpreter> Interpreter::create_interp(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, const char *property)
{
#ifdef COMPRESS
    const char *lang = uncompress(get_property_class(program, "~language"));
#else
    const char *lang = get_property_class(program, "~language");
#endif
    std::tr1::shared_ptr<Interpreter> ret_interp;

    if (!MLevel(program) || !MLevel(OWNER(program)) ||
        ((source != NOTHING) && !TrueWizard(OWNER(source)) &&
            !can_link_to(OWNER(source), TYPE_EXIT, program))) 
    {
        notify_nolisten(player, "Program call: Permission denied.", 1);
        return ret_interp;
    }

    if (lang && (strcmp(lang, "Lua") == 0)) // Lua
    {
        ret_interp = std::tr1::shared_ptr<Interpreter>(new LuaInterpreter(player, location, program, source, 
            nosleeps, whichperms, event, property));
    }
    else // MUF
    {
        ret_interp = std::tr1::shared_ptr<Interpreter>(new MUFInterpreter(player, location, program, source, 
            nosleeps, whichperms, event, property));
    }

    ret_interp->set_weak_pointer();

    return ret_interp;
}

std::tr1::shared_ptr<InterpreterReturnValue> Interpreter::create_and_run_interp(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, const char *property, const char *arg)
{
    std::tr1::shared_ptr<InterpreterReturnValue> irv;

    std::tr1::shared_ptr<Interpreter> i = 
        Interpreter::create_interp(player, location, program, source, nosleeps, whichperms, event, property);

    if (i)
        irv = i->resume(arg);

    return irv;
}

/* --------------------------------------------------------------------------------------
    Lua
*/
LuaInterpreter::LuaInterpreter(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, const char *property)
    : Interpreter(event, player)
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
}

// Why? because this may not be done during the constructor.
void LuaInterpreter::set_weak_pointer()
{
    if (this->fr)
        this->fr->interpreter = shared_from_this();
}

LuaInterpreter::~LuaInterpreter()
{
    if (this->fr)
        mlua_free_interp(this->fr);
}

const char *LuaInterpreter::type()
{
    return "Lua";
}

bool LuaInterpreter::background()
{
    return this->fr->mode == MLUA_BACKGROUND;
}

void LuaInterpreter::handle_read_event(const char *command)
{
    this->resume(command);
    // Um. What do we do now? Are we done? How do we know?
}

std::tr1::shared_ptr<InterpreterReturnValue> LuaInterpreter::resume(const char *str)
{
    std::tr1::shared_ptr<InterpreterReturnValue> irv;

    this->totaltime_start();
    irv = mlua_resume(this->fr, str);
    this->totaltime_stop();

    return irv;
}

void LuaInterpreter::totaltime_start()
{
    gettimeofday(&this->proftime, NULL);
    if (!this->started)
        this->started = time(NULL);
}

time_t LuaInterpreter::get_started()
{
    return this->started;
}

long LuaInterpreter::get_instruction_count()
{
    return 0L;
}

void LuaInterpreter::totaltime_stop()
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

struct timeval *LuaInterpreter::get_totaltime()
{
    return &this->totaltime;
}

/* --------------------------------------------------------------------------------------
    MUF
*/
MUFInterpreter::MUFInterpreter(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, const char *property)
    : Interpreter(event, player)
{
    this->fr = create_interp_frame(player, location, program, source, nosleeps, whichperms);
    this->program = program;
//    this->fr->interpreter = shared_from_this();
}

MUFInterpreter::MUFInterpreter(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, const char *property, 
       struct frame *new_frame)
    : Interpreter(event, player)
{
    this->program = program;
    this->fr = new_frame;
}

// Why? because this may not be done during the constructor.
void MUFInterpreter::set_weak_pointer()
{
    this->fr->interpreter = shared_from_this();
}

const char *MUFInterpreter::type()
{
    return "MUF";
}

struct timeval *MUFInterpreter::get_totaltime()
{
    return &this->fr->totaltime;
}

bool MUFInterpreter::background()
{
    return this->fr->multitask == BACKGROUND;
}

time_t MUFInterpreter::get_started()
{
    return (time_t)this->fr->started;
}

long MUFInterpreter::get_instruction_count()
{
    return this->fr->instcnt;
}

/* Checks the MUF timequeue for address references on the stack or */
/* dbref references on the callstack */
bool MUFInterpreter::has_refs(dbref program)
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

bool MUFInterpreter::get_number_of_references(dbref program)
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

MUFInterpreter::~MUFInterpreter()
{
    // Release people from blocking mode...
    if (this->fr->multitask != BACKGROUND)
        DBFETCH(this->uid)->sp.player.block = 0;

    // Delete interpreter
    prog_clean(this->fr);
}

void MUFInterpreter::handle_read_event(const char *command)
{
    if (this->fr->brkpt.debugging && !this->fr->brkpt.isread) {

        /* We're in the MUF debugger!  Call it with the input line. */
        if(muf_debugger(this->uid, this->program, command, this->fr)) {

            /* MUF Debugger exited.  Free up the program frame & exit */
            prog_clean(this->fr);
            return;
        }

        /*
         * When using the MUF Debugger, the debugger will set the
         * INTERACTIVE bit on the user, if it does NOT want the MUF
         * program to resume executing.
         */
        if (!(FLAGS(this->uid) & INTERACTIVE)) {
            this->resume(NULL);
        }

    } else {
        this->resume(command);
    }
}

// From interp.c
void RCLEAR(struct inst * oper, char *file, int line);
#define CLEAR(oper) RCLEAR(oper, __FILE__, __LINE__)

std::tr1::shared_ptr<InterpreterReturnValue> MUFInterpreter::resume(const char *str)
{
    struct inst *rv;
    std::tr1::shared_ptr<InterpreterReturnValue> irv;

    if (str)
    {
        if (this->fr->argument.top >= STACK_SIZE) {

            /*
             * Uh oh! That MUF program's stack is full!
             * Print an error, free the frame, and exit.
             */
            notify_nolisten(this->uid, "Program stack overflow.", 1);
            //prog_clean(this->fr);
            irv = std::tr1::shared_ptr<InterpreterReturnValue>(new InterpreterReturnValue(
                    InterpreterReturnValue::NIL,
                    0
                    ));
            return irv;
        }
        
        /*
         * Place the string on the stack
         */
        this->fr->argument.st[this->fr->argument.top].type = PROG_STRING;
        this->fr->argument.st[this->fr->argument.top++].data.string =
            alloc_prog_string(str);
    }

    rv = interp_loop(this->uid, this->program, this->fr, 0);

    if (rv == NULL)
    {
        return std::tr1::shared_ptr<InterpreterReturnValue>(new InterpreterReturnValue(
                    InterpreterReturnValue::BOOL,
                    FALSE
                    ));
    }

    if ((int)rv == 1)
    {
        return std::tr1::shared_ptr<InterpreterReturnValue>(new InterpreterReturnValue(
                    InterpreterReturnValue::BOOL,
                    TRUE
                    ));
    }

    switch(rv->type) {
        case PROG_STRING:
            if (rv->data.string) {
                irv = std::tr1::shared_ptr<InterpreterReturnValue>(new InterpreterReturnValue(
                    InterpreterReturnValue::STRING,
                    rv->data.string->data
                    ));
            } else {
                irv = std::tr1::shared_ptr<InterpreterReturnValue>(new InterpreterReturnValue(
                    InterpreterReturnValue::STRING,
                    ""
                    ));
            }
            break;

        case PROG_INTEGER:
            irv = std::tr1::shared_ptr<InterpreterReturnValue>(new InterpreterReturnValue(
                    InterpreterReturnValue::INTEGER,
                    rv->data.number
                    ));
            break;

        case PROG_OBJECT:
            irv = std::tr1::shared_ptr<InterpreterReturnValue>(new InterpreterReturnValue(
                    InterpreterReturnValue::DBREF,
                    rv->data.objref
                    ));

            // Note: MUF primitive returns a string, that needs to be handled
            // in the muf primitive, not here. Though, it would be nice if 
            // InterpreterReturnValue had a function to do this.
            //ptr = ref2str(rv->data.objref, buf);
            break;

        default:
            irv = std::tr1::shared_ptr<InterpreterReturnValue>(new InterpreterReturnValue(
                    InterpreterReturnValue::NIL,
                    0
                    ));
            break;
    }
    CLEAR(rv);
    return irv;
}

/* --------------------------------------------------------------------------------------
    InterpreterReturnValue stuff
*/
InterpreterReturnValue::InterpreterReturnValue(const int type, const int num)
{
    return_type = type;
    v_num = num;
    v_str = NULL;
}

InterpreterReturnValue::InterpreterReturnValue(const int type, const char *str)
{
    return_type = type;
    v_num = 0;
    v_str = alloc_string(str);
}

InterpreterReturnValue::~InterpreterReturnValue()
{
    if (v_str) free(v_str);
}

const char *InterpreterReturnValue::String()
{
    return v_str;
}

const int InterpreterReturnValue::Type()
{
    return return_type;
}

const int InterpreterReturnValue::Bool()
{
    if (return_type == InterpreterReturnValue::STRING)
        return v_str[0] != 0;
    else if (return_type == InterpreterReturnValue::NIL)
        return FALSE;
    else
        return v_num != 0;
}

const int InterpreterReturnValue::Dbref()
{
    return v_num;
}

const int InterpreterReturnValue::Number()
{
    return v_num;
}



