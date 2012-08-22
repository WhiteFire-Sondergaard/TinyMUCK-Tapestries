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
Interpeter *Interpeter::create_interp(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, const char *property)
{
	const char *lang = get_property_class(program, "~language");


	if (!MLevel(program) || !MLevel(OWNER(program)) ||
		((source != NOTHING) && !TrueWizard(OWNER(source)) &&
			!can_link_to(OWNER(source), TYPE_EXIT, program))) 
	{
		notify_nolisten(player, "Program call: Permission denied.", 1);
		return 0;
	}

	if (lang && (strcmp(lang, "Lua") == 0)) // Lua
	{
		return new LuaInterpeter(player, location, program, source, 
			nosleeps, whichperms, event, property);
	}
	else // MUF
	{
		return new MUFInterpeter(player, location, program, source, 
			nosleeps, whichperms, event, property);
	}
}

/* --------------------------------------------------------------------------------------
	Lua
*/
LuaInterpeter::LuaInterpeter(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, const char *property)
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

	fr = mlua_create_interp(program, property, location, player, 
			source, euid, mode, event);
}

/* --------------------------------------------------------------------------------------
	MUF
*/
MUFInterpeter::MUFInterpeter(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, const char *property)
{
	fr = create_interp_frame(player, location, program, source, nosleeps, whichperms);
	this->player = player;
	this->program = program;
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



