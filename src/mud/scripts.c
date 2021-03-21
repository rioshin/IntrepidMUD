/*
 * IntrepidMUD
 * MUD Server
 * The main MUD server itself
 * ---------------------------------------------------------------------------
 * Copyright 2012-2021 by Mikael Segercrantz, Dan Griffiths and Dave Etheridge
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the license, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../include/mudconfig.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../include/proto.h"

void lua_runscript(user * u, char *str)
{
  lua_State *L;

  if (!str || !*str)
  {
    vtell_user(u, "Please tell the staff that lua_runscript needs to be "
               "called with the name of the lua script.");
    return;
  }

  L = luaL_newstate();
  luaL_openlibs(L);

  if (luaL_dofile(L, str))
  {
    vtell_user(u, "The script %s was not found.", lua_tostring(L, -1));
    lua_close(L);
    return;
  }

  lua_close(L);
}
