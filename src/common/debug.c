/* IntrepidMUD
 * Common Library
 * Common features between the different MUD executables
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/proto.h"

void send_to_debug(char *str)
{
  unsigned char *oldstack = stack;
  static char *temp = null_ptr;
  user *scan;

  sprintf((char *)stack, "^H[DEBUG] %s^N", str);
  for (scan = users_list; scan; scan = scan->next)
    if (scan->residency & HCADMIN)
    {
      temp = malloc(strlen((char *)oldstack) + 1);
      if (temp)
      {
        memset(temp, 0, strlen((char *)oldstack) + 1);
        strcpy(temp, (char *)oldstack);
      }
      free(temp);
      temp = null_ptr;
      vtell_user(scan, (char *)oldstack);
    }

  stack = oldstack;
}

void vsend_to_debug(char *fmt, ...)
{
  unsigned char *oldstack = stack;
  va_list argum;

  va_start(argum, fmt);
  vsprintf((char *)stack, fmt, argum);
  va_end(argum);
  stack = end_string(stack);

  send_to_debug((char *)oldstack);
  stack = oldstack;
}
