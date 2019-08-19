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

#ifndef __REALM_H
#define __REALM_H

typedef struct realm_data
{
  int32 min;                    /* 0-59 */
  int32 hour;                   /* 0-23 */
  int32 day;                    /* 1-31 */
  int32 month;                  /* 1-12 */
  int32 year;                   /* 0-999 */
  int32 epoch;                  /* 0-25 */
} realm_data;

#endif                          /* __REALM_H */
