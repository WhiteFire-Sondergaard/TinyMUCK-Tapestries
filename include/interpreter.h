/*
  Interpreter abstraction by WhiteFire Sondergaard for Tapestries MUCK.
*/

#ifndef _interpreter_h_included_
#define _interpreter_h_included_

#include <sys/time.h>
#include <tr1/memory>

class Interpreter; // more forward declration
class InterpreterReturnValue; // forward declration

#include "mlua.h"

/* 	The interpreter class itself is an abstract class that defines an API
	for the various language classess to fill.
*/
class Interpreter : public std::tr1::enable_shared_from_this<Interpreter>
{
public:
	/* 
	Stuff to be defined by the various subclasses 
	*/

	virtual const char *type() = 0;

	// Do what it says
	virtual void handle_read_event(const char *command) = 0;

	// Are we backgrounded?
	virtual bool background() = 0;

	// Resume running program
	virtual std::tr1::shared_ptr<InterpreterReturnValue> resume(const char *) = 0;

	// Get totaltime for listings
	virtual struct timeval *get_totaltime() = 0;

	// Get start time for listings
	virtual time_t get_started() = 0;

	// Instructions executed
	virtual long get_instruction_count() = 0;

	// 
	virtual bool has_refs(dbref program) = 0;

	// 
	virtual bool get_number_of_references(dbref program) = 0;

	//
	virtual void set_weak_pointer() = 0;

	/*
	 * Stuff that is global
	 */
	int get_pid() { return this->pid; };
	Interpreter(int event, dbref player) 
	{ 
		this->pid = event; 
		this->uid = player; 
	};

	/* 
	Factory for creating execution environments 
	*/
	static std::tr1::shared_ptr<Interpreter> create_interp(
		  dbref player, dbref location, dbref program,
      dbref source, int nosleeps, int whichperms, 
      int event, const char *property);

	/* 
	Factory for creating execution environments 
	*/
	static std::tr1::shared_ptr<InterpreterReturnValue> create_and_run_interp(
		  dbref player, dbref location, dbref program,
      dbref source, int nosleeps, int whichperms, 
      int event, const char *property, const char *args);

protected:
	int pid; // process id
	dbref uid; // Player owning process
	bool backgrounded;
	time_t started;
};

class MUFInterpreter : public Interpreter
{
public:
	// What type of interpreter are we?
	const char *type();

	void handle_read_event(const char *command);

	// Are we backgrounded?
	bool background();

	// Resume
	std::tr1::shared_ptr<InterpreterReturnValue> resume(const char *);

	// Get totaltime for listings
	struct timeval *get_totaltime();

	// Get start time for listings
	time_t get_started();

	// Instructions executed
	long get_instruction_count();
	
	// 
	bool has_refs(dbref program);

	// 
	bool get_number_of_references(dbref program);

	//
	void set_weak_pointer();

	MUFInterpreter(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, 
       const char *property);

	MUFInterpreter(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, 
       const char *property, struct frame *new_frame);

	~MUFInterpreter();
private:
	// Things required to set this up.
	struct frame *fr;
	dbref program;
};

class LuaInterpreter : public Interpreter
{
public:
	// What type of interpreter are we?
	const char *type();

	void handle_read_event(const char *command);

	// Are we backgrounded?
	bool background();

	// Resume
	std::tr1::shared_ptr<InterpreterReturnValue> resume(const char *);

	// Get totaltime for listings
	struct timeval *get_totaltime();

	// Get start time for listings
	time_t get_started();

	// Instructions executed
	long get_instruction_count();
	
	// 
	bool has_refs(dbref program) { return false; };

	// 
	bool get_number_of_references(dbref program) { return false; };

	//
	void set_weak_pointer();

	LuaInterpreter(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, 
       const char *property);

	~LuaInterpreter();

private:
	void totaltime_start();
	void totaltime_stop();

	struct mlua_interp *fr;
	struct timeval totaltime;   /* profiling timing code */
	struct timeval proftime;   /* profiling timing code */
};

class InterpreterReturnValue
{
public:
	static const int NIL = 0;
	static const int STRING = 1;
	static const int INTEGER = 2;
	static const int BOOL = 3;
	static const int DBREF = 4;

	InterpreterReturnValue(const int type, const int num);
	InterpreterReturnValue(const int type, const char *str);
	~InterpreterReturnValue();
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

#endif // !_interpreter_h_included_
