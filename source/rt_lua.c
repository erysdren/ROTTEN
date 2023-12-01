/*
Copyright (C) 1994-1995 Apogee Software, Ltd.
Copyright (C) 2002-2015 Steven Fuller, Ryan C. Gordon, John Hall, Dan Olson
Copyright (C) 2023 Fabian Greffrath
Copyright (C) 2023 erysdren (it/she/they)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

//****************************************************************************
//
// RT_LUA.C
//
//****************************************************************************

#include "rt_def.h"
#include "rt_datadir.h"
#include "rt_util.h"
#include "rt_build.h"
#include "rt_menu.h"
#include "w_wad.h"
#include "z_zone.h"

#include "rt_lua.h"

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

//****************************************************************************
//
// IO LIBRARY
//
//****************************************************************************

int _lua_io_print(lua_State *L)
{
	const char *s = luaL_checkstring(L, 1);
	printf("%s\n", s);
	return 0;
}

int _lua_io_error(lua_State *L)
{
	const char *s = luaL_checkstring(L, 1);
	Error("%s", s);
	return 0;
}

static const struct luaL_Reg _lua_io[] = {
	{"print", _lua_io_print},
	{"error", _lua_io_error},
	{NULL, NULL}
};

//****************************************************************************
//
// MENU LIBRARY
//
//****************************************************************************

int _lua_menu_drawstring(lua_State *L)
{
	int x = luaL_checkinteger(L, 1);
	int y = luaL_checkinteger(L, 2);
	const char *s = luaL_checkstring(L, 3);
	DrawMenuBufPropString(x, y, s);
	return 0;
}

int _lua_menu_drawpic(lua_State *L)
{
	int x = luaL_checkinteger(L, 1);
	int y = luaL_checkinteger(L, 2);
	const char *s = luaL_checkstring(L, 3);
	DrawMenuBufPic(x, y, W_GetNumForName((char *)s));
	return 0;
}

int _lua_io_setfont(lua_State *L)
{
	int f = luaL_checkinteger(L, 1);
	if (f < mn_tinyfont || f > mn_largefont) return 0;
	IFont = (cfont_t *)W_CacheLumpName(FontNames[f], PU_CACHE, Cvt_cfont_t, 1);
	return 0;
}

int _lua_io_settitle(lua_State *L)
{
	const char *s = luaL_checkstring(L, 1);
	SetMenuTitle(s);
	return 0;
}

static const struct luaL_Reg _lua_menu[] = {
	{"drawstring", _lua_menu_drawstring},
	{"drawpic", _lua_menu_drawpic},
	{"setfont", _lua_io_setfont},
	{"settitle", _lua_io_settitle},
	{NULL, NULL}
};

//****************************************************************************
//
// LOCAL VARIABLES
//
//****************************************************************************

/* lua state for menus */
static lua_State *lua_menu_state = NULL;

//****************************************************************************
//
// PUBLIC FUNCTIONS
//
//****************************************************************************

boolean lua_init(void)
{
	/* create lua state */
	lua_menu_state = luaL_newstate();
	if (lua_menu_state == NULL)
		return false;

	/* link io libraries */
	luaL_newlib(lua_menu_state, _lua_io);
	lua_setglobal(lua_menu_state, "io");
	lua_settop(lua_menu_state, 0);

	/* link menu library */
	luaL_newlib(lua_menu_state, _lua_menu);
	lua_setglobal(lua_menu_state, "menu");
	lua_settop(lua_menu_state, 0);

	return true;
}

void lua_quit(void)
{
	lua_close(lua_menu_state);
}

void lua_menu_init(const char *menu)
{
	char *temp;
	char *filename;

	/* get lua path */
	temp = M_StringJoin("scripts/menu/", menu, ".lua", NULL);
	filename = FindFileByName(temp);

	if (filename == NULL)
		Error("Invalid menu \"%s\" specified!", menu);

	/* run file */
	luaL_loadfile(lua_menu_state, filename);
	if (lua_pcall(lua_menu_state, 0, LUA_MULTRET, 0))
		Error("Lua Error: \"%s\"", lua_tostring(lua_menu_state, -1));

	/* make module global */
	lua_setglobal(lua_menu_state, menu);
	lua_settop(lua_menu_state, 0);

	/* free tempstrings */
	free(temp);
	free(filename);
}

void lua_menu_call(const char *menu, const char *field)
{
	/* get function */
	lua_getglobal(lua_menu_state, menu);
	lua_getfield(lua_menu_state, -1, field);

	/* do the call */
	lua_pcall(lua_menu_state, 0, 0, 0);
}