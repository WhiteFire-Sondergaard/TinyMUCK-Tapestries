/* Primitives package */

#include "copyright.h"
#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "db.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "strings.h"
#include "interp.h"

extern int force_level;


static struct inst *oper1, *oper2, *oper3, *oper4;
//static struct inst temp1, temp2, temp3;
static int tmp, result;
//static dbref ref;


int 
arith_type(struct inst * op1, struct inst * op2)
{
    return ((op1->type == PROG_INTEGER && op2->type == PROG_INTEGER)	/* real stuff */
	    ||(op1->type == PROG_OBJECT && op2->type == PROG_INTEGER)	/* inc. dbref */
	    ||(op1->type == PROG_VAR && op2->type == PROG_INTEGER)	/* offset array */
	    ||(op1->type == PROG_LVAR && op2->type == PROG_INTEGER));
}


void 
prim_add(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!arith_type(oper2, oper1))
	abort_interp("Invalid argument type.");
    result = oper1->data.number + oper2->data.number;
    tmp = oper2->type;
    CLEAR(oper1);
    CLEAR(oper2);
    push(arg, top, tmp, MIPSCAST & result);
}


void 
prim_subtract(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!arith_type(oper2, oper1))
	abort_interp("Invalid argument type.");
    result = oper2->data.number - oper1->data.number;
    tmp = oper2->type;
    CLEAR(oper1);
    CLEAR(oper2);
    push(arg, top, tmp, MIPSCAST & result);
}


void 
prim_multiply(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!arith_type(oper2, oper1))
	abort_interp("Invalid argument type.");
    result = oper1->data.number * oper2->data.number;
    tmp = oper2->type;
    CLEAR(oper1);
    CLEAR(oper2);
    push(arg, top, tmp, MIPSCAST & result);
}


void 
prim_divide(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!arith_type(oper2, oper1))
	abort_interp("Invalid argument type.");
    if (oper1->data.number)
	result = oper2->data.number / oper1->data.number;
    else
	result = 0;
    tmp = oper2->type;
    CLEAR(oper1);
    CLEAR(oper2);
    push(arg, top, tmp, MIPSCAST & result);
}


void 
prim_mod(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!arith_type(oper2, oper1))
	abort_interp("Invalid argument type.");
    if (oper1->data.number)
	result = oper2->data.number % oper1->data.number;
    else
	result = 0;
    tmp = oper2->type;
    CLEAR(oper1);
    CLEAR(oper2);
    push(arg, top, tmp, MIPSCAST & result);
}


void 
prim_bitor(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!arith_type(oper2, oper1))
	abort_interp("Invalid argument type.");
    result = oper2->data.number | oper1->data.number;
    tmp = oper2->type;
    CLEAR(oper1);
    CLEAR(oper2);
    push(arg, top, tmp, MIPSCAST & result);
}


void 
prim_bitxor(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!arith_type(oper2, oper1))
	abort_interp("Invalid argument type.");
    result = oper2->data.number ^ oper1->data.number;
    tmp = oper2->type;
    CLEAR(oper1);
    CLEAR(oper2);
    push(arg, top, tmp, MIPSCAST & result);
}


void 
prim_bitand(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!arith_type(oper2, oper1))
	abort_interp("Invalid argument type.");
    result = oper2->data.number & oper1->data.number;
    tmp = oper2->type;
    CLEAR(oper1);
    CLEAR(oper2);
    push(arg, top, tmp, MIPSCAST & result);
}


void 
prim_bitshift(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!arith_type(oper2, oper1))
	abort_interp("Invalid argument type.");
    if (oper1->data.number > 0)
	result = oper2->data.number << oper1->data.number;
    else if (oper1->data.number < 0)
	result = oper2->data.number >> (-(oper1->data.number));
    else
	result = oper2->data.number;
    tmp = oper2->type;
    CLEAR(oper1);
    CLEAR(oper2);
    push(arg, top, tmp, MIPSCAST & result);
}


void 
prim_and(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    result = !false(oper1) && !false(oper2);
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void 
prim_or(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    result = !false(oper1) || !false(oper2);
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void 
prim_not(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    result = false(oper1);
    CLEAR(oper1);
    PushInt(result);
}

void 
prim_lessthan(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!(oper1->type == PROG_INTEGER && oper2->type == PROG_INTEGER))
	abort_interp("Invalid argument type.");
    result = oper2->data.number < oper1->data.number;
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}


void 
prim_greathan(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!(oper1->type == PROG_INTEGER && oper2->type == PROG_INTEGER))
	abort_interp("Invalid argument type.");
    result = oper2->data.number > oper1->data.number;
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}


void 
prim_equal(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!(oper1->type == PROG_INTEGER && oper2->type == PROG_INTEGER))
	abort_interp("Invalid argument type.");
    result = oper1->data.number == oper2->data.number;
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}


void 
prim_lesseq(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!(oper1->type == PROG_INTEGER && oper2->type == PROG_INTEGER))
	abort_interp("Invalid argument type.");
    result = oper2->data.number <= oper1->data.number;
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}


void 
prim_greateq(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!(oper1->type == PROG_INTEGER && oper2->type == PROG_INTEGER))
	abort_interp("Invalid argument type.");
    result = oper2->data.number >= oper1->data.number;
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void 
prim_random(PRIM_PROTOTYPE)
{
    result = RANDOM();
    CHECKOFLOW(1);
    PushInt(result);
}


void 
prim_int(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (!(oper1->type == PROG_OBJECT || oper1->type == PROG_VAR ||
	  oper1->type == PROG_LVAR))
	abort_interp("Invalid argument type.");
    result = (int) ((oper1->type == PROG_OBJECT) ?
		    oper1->data.objref : oper1->data.number);
    CLEAR(oper1);
    PushInt(result);
}
