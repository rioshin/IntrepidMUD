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

#ifndef __MUDCONFIG_H
#define __MUDCONFIG_H

#include "root.h"
#include "types.h"

#define INTREPID_VERSION "0.128"

#define MUD_SOCKET_PATH "files/pids/mud_alive_socket"
#define ANGEL_SOCKET_PATH "files/pids/angel_alive_socket"
#define ARCHANGEL_PID "files/pids/ARCHANGEL_PID"
#define ANGEL_PID "files/pids/ANGEL_PID"
#define MUD_PID "files/pids/MUD_PID"

/*
 * Next up are the starting locations for new characters - if you want to
 * change were a character starts in the MUD, change the following lines
 */
#define START_AREA "school"
#define START_ROOM "begin"

#ifdef NEED_CONST_DIRENT
#define DIRENT_PROTO const
#else
#define DIRENT_PROTO
#endif

#define TIMER_CLICK 	5
#define ONE_SECOND	1
#define ONE_MINUTE	60
#define	ONE_HOUR	3600
#define	ONE_DAY		86400
#define ONE_WEEK	604800
#define ONE_MONTH	2419200 /* actually only 28 days */
#define ONE_YEAR	31536000

#define null     0
#define null_ptr (void *)0
#define null_chr '\0'

#endif
