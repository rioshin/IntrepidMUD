/*
 * IntrepidMUD
 * Header file
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

#ifndef __TYPES_H
#define __TYPES_H

typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;

/* modify the int64 to int32 if you're using a 32-bit system, otherwise
 * keep it as int64 on a 64-bit system */
typedef int64 pint;

struct user;
typedef void command_func(struct user *, char *);
typedef void user_func(struct user *);
typedef unsigned char *string_func(unsigned char *);

#define CMD_INVIS	(1<<0)
#define CMD_SPACE	(1<<1)
#define CMD_MATCH	(1<<30)

typedef struct command
{
  char *cmd;
  command_func *func;
  int32 flags;
} command;

typedef struct terminal
{
  char *name;
  int32 type;
  int32 hidden;
} terminal;

#endif                          /* __TYPES_H */
