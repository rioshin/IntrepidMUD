/*
 * IntrepidMUD
 * Header file
 * ---------------------------------------------------------------------------
 * Copyright 2012-2019 by Mikael Segercrantz, Dan Griffiths and Dave Etheridge
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

#ifndef __CHARACTER_H
#define __CHARACTER_H

#define MAX_CHAR_NAME	20

#include "types.h"
#include "room.h"
#include "race.h"
#include "class.h"
#include "user.h"

typedef struct user u_struct;

typedef struct character
{
  /* Information following this needs to be saved */
  char firstname[MAX_CHAR_NAME];
  char familyname[MAX_CHAR_NAME];
  char user[MAX_USER_NAME];
  room *location;
  int32 align;
  int32 ethos;
  int32 remorted;
  int32 level;
  int32 gender;
  struct race *race;
  struct class *class;

  /* Information following this doesn't need to be saved */
  struct character *user_next;
  struct character *room_next;
  struct character *next;
  u_struct *the_user;
} character;

#endif
