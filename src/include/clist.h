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

#ifndef __CLIST_H
#define __CLIST_H

#include "types.h"

extern command_func say, emote, think, sing, list_commands, quit_mud,
  who_is_on, pemote, help_command, reboot_mud, kill_angel, version_func,
  pick_term, set_prompt, term_lines, term_width, term_compare, password,
  wake, wall, recap, shutdown_mud, view_log, nwho, setidle, kill_arch,
  list_addresses, mudtime;

extern command full_list[], *cmd_list[];

#endif
