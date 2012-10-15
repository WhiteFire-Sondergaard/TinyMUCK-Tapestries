/*
  Interpeter abstraction by WhiteFire Sondergaard for Tapestries MUCK.
*/

#ifndef _interpeter_h_included_
#define _interpeter_h_included_

#include <sys/time.h>


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

	virtual const char *type() = 0;

	// Do what it says
	virtual void handle_read_event(const char *command) = 0;

	// Are we backgrounded?
	virtual bool background() = 0;

	// Resume running program
	virtual void resume(const char *) = 0;

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

	/*
	 * Stuff that is global
	 */
	int get_pid() { return this->pid; };
	Interpeter(int event, dbref player) 
	{ 
		this->pid = event; 
		this->uid = player; 
	};

	/* 
	Factory for creating execution environments 
	*/
	static Interpeter *create_interp(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, const char *property);

protected:
	int pid; // process id
	dbref uid; // Player owning process
	bool backgrounded;
	time_t started;
};

class MUFInterpeter : public Interpeter
{
public:
	// What type of interpeter are we?
	const char *type();

	void handle_read_event(const char *command);

	// Are we backgrounded?
	bool background();

	// Resume
	void resume(const char *);

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

	MUFInterpeter(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, 
       const char *property);

	~MUFInterpeter();
private:
	// Things required to set this up.
	struct frame *fr;
	dbref player;
	dbref program;
};

class LuaInterpeter : public Interpeter
{
public:
	// What type of interpeter are we?
	const char *type();

	void handle_read_event(const char *command);

	// Are we backgrounded?
	bool background();

	// Resume
	void resume(const char *);

	// Get totaltime for listings
	struct timeval *get_totaltime();

	// Get start time for listings
	time_t get_started();

	// Instructions executed
	long get_instruction_count();
	
	// 
	bool has_refs(dbref program) { return FALSE; };

	// 
	bool get_number_of_references(dbref program) { return FALSE; };

	LuaInterpeter(dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int event, 
       const char *property);
private:
	void totaltime_start();
	void totaltime_stop();

	struct mlua_interp *fr;
	struct timeval totaltime;   /* profiling timing code */
	struct timeval proftime;   /* profiling timing code */
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