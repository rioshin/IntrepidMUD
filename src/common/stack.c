/*
 * IntrepidMUD
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

#include <stdlib.h>
#include <string.h>

#include "../include/proto.h"

unsigned char *create_stack(int32 size)
{
  /* Allocate the memory we were asked for */
  unsigned char *cur_stack = (unsigned char *)malloc(size);

  /* And clear it */
  if (cur_stack)
    memset(cur_stack, 0, size);

  /* Return the stack */
  return cur_stack;
}
