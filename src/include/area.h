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

#ifndef __AREA_H
#define __AREA_H

#define MAX_AREA_NAME	40

typedef struct room r_struct;

typedef struct area
{
  /* The following information needs to be saved */
  char areaname[MAX_AREA_NAME];

  /* The following information does not need to be saved */
  r_struct *first_room;
  struct area *next;
  struct area *previous;
} area;

#endif                          /* __AREA_H */
