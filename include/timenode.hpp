
#ifndef _timenode_h_included_
#define _timenode_h_included_

class TimeNode {
public:
    static const unsigned int MUF_TYP = 0;
    static const unsigned int MPI_TYP = 1;
    static const unsigned int LUA_TYP = 2;

    static const unsigned int QUEUE   = 0x0;
    static const unsigned int DELAY   = 0x1;
    static const unsigned int LISTEN  = 0x2;
    static const unsigned int READ    = 0x3;
    static const unsigned int SUBMASK = 0x7;
    static const unsigned int OMESG   = 0x8;

//private:
    TimeNode *next;         // Linked list
    int     typ;            // Script type
    int     subtyp;         // State
    time_t  when;           // When to next execute
    dbref   called_prog;    // Dbref of called program
    char   *called_data;    // str1
    char   *command;        // cmdstr
    char   *str3;           // str3
    dbref   uid;            // user or "speaker"
    dbref   loc;            // Location of event
    dbref   trig;           // triggering object
    struct frame *fr;       // Muf interp
    struct inst *where;     // Instruction pointer
    int     eventnum;       // event ID

    static TimeNode *head;
};

#endif // !_timenode_h_included_
