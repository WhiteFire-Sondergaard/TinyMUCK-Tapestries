/*
  Interpeter abstraction by WhiteFire Sondergaard for Tapestries MUCK.
*/

#ifndef _interpeter_h_included_
#define _interpeter_h_included_

class InterpeterReturnValue; // forward declration

/* 	The interpeter class itself is an abstract class that defines an API
	for the various language classess to fill.
*/
class Interpeter
{
public:
	/* 
	Stuff to be defined by the various subclasses 
	*/

	// terminate and clean up the program
//	virtual void clean() =0;
	// resume a running interpeter
//	virtual void resume() =0;
	// start an interpeter
//	virtual InterpeterReturnValue *run() =0;
	// are we debugging?
//	virtual int debugging() =0;
	// execute a debugger command.
//	virtual int debugger(const char *command) =0;

	/* 
	Factory for creating execution environments 
	*/
	static Interpeter *create_interp(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, const char *property);

private:

};

class MUFInterpeter : public Interpeter
{
public:
//	virtual void clean();
	// resume a running interpeter
//	virtual void resume();
	// start an interpeter
//	virtual InterpeterReturnValue *run();
	// are we debugging?
//	virtual int debugging();
	// execute a debugger command.
//	virtual int debugger(const char *command);

	MUFInterpeter(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, const char *property);
private:
	// Things required to set this up.
	struct frame *fr;
	dbref player;
	dbref program;
};

class LuaInterpeter : public Interpeter
{
public:
//	virtual void clean();
	// resume a running interpeter
//	virtual void resume();
	// start an interpeter
//	virtual InterpeterReturnValue *run();
	// are we debugging?
//	virtual int debugging();
	// execute a debugger command.
//	virtual int debugger(const char *command);

	LuaInterpeter(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, const char *property);
private:
	struct mlua_interp *fr;
};

class InterpeterReturnValue
{
public:
	static const unsigned int NIL = 0;
	static const unsigned int STRING = 1;
	static const unsigned int INTEGER = 2;
	static const unsigned int BOOL = 3;
	static const unsigned int DBREF = 4;

	InterpeterReturnValue(const int type, const int num);
	InterpeterReturnValue(const int type, const char *str);
	~InterpeterReturnValue();
	const char *String();
	const int Type();
	const int Bool();
	const int Dbref();
	const int Number();

private:
	int return_type;
	int v_num;
	char *v_str;
};

#endif // !_interpeter_h_included_