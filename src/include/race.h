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

#ifndef __RACE_H
#define __RACE_H

enum ability
{
  strength = 0,
  dexterity = 1,
  constitution = 2,
  intelligence = 3,
  wisdom = 4,
  charisma = 5
};

typedef struct race
{
  int32 default_abilities[6];
  int32 max_abilities[6];
  char name[MAX_USER_NAME];
  struct race *next_race;
} race;

#endif                          /* __RACE_H */
