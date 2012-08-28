/*
 * Mlua bindings to the Muck DB.
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
#include "mlua.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// extern "C" {
// #include "lua.h"
// #include "lauxlib.h"
// #include "lualib.h"
// }

#include "lua.hpp"

/*
 * Stupid utilities that should be in db.c
 */
static int dbref_valid(dbref d)
{
    return (!(d >= db_top || (d < 0) || Typeof(d) == TYPE_GARBAGE));
}

/*
 * Create a meta table that contains the methods for a dbref object.
 */
static int dbref_name (lua_State *L) {
    dbref *d = (dbref *)lua_touserdata(L, 1);
    luaL_argcheck(L, d != NULL, 1, "`dbref' expected");
    lua_pushstring(L, dbref_valid(*d) ? NAME(*d) : "(invalid)");
    return 1;
}

static const luaL_Reg dbref_meta[] = {
  {"name", 		dbref_name},
//  {"unparse",		dbref_unparse},
//  {"__tostring", 	dbref_unparse},
  {NULL, NULL}
};

static void createmeta (lua_State *L) {
  luaL_newmetatable(L, "mlua_dbref");  
  lua_pushliteral(L, "__index");
  lua_pushvalue(L, -2);  /* push metatable */
  lua_rawset(L, -3);  /* metatable.__index = metatable */
  luaL_setfuncs(L, dbref_meta, 0);
}

/*
 * Create the mdb library.
 */

/* Create a new dbref object */
void mlua_push_dbref(lua_State *L, dbref d)
{
    dbref *p = (dbref *)lua_newuserdata(L, sizeof(dbref));
    *p = d;
    luaL_getmetatable(L, "mlua_dbref");
    lua_setmetatable(L, -2);
}

static int mdb_new (lua_State *L) {
    dbref d = luaL_checkint(L, 1);
    mlua_push_dbref(L, d);
    return 1;  /* new userdatum is already on the stack */
}

static const luaL_Reg lib_mdb[] = {
  {"new", 		mdb_new},
  {NULL, NULL}
};

int mlua_open_mdb (lua_State *L) {
    createmeta(L);
    luaL_newlib(L, lib_mdb);
    return 1;
}

