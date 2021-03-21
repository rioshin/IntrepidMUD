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

#include "../include/proto.h"
#include "../include/clist.h"

command full_list[] = {
  /* non-alphabetical */
  {"'", say, 0}
  ,
  {"`", say, CMD_INVIS}
  ,
  {"\"", say, CMD_INVIS}
  ,
  {"::", pemote, 0}
  ,
  {":;", pemote, CMD_INVIS}
  ,
  {";;", pemote, CMD_INVIS}
  ,
  {";:", pemote, CMD_INVIS}
  ,
  {":", emote, 0}
  ,
  {";", emote, CMD_INVIS}
  ,
  {"~", think, 0}
  ,
  {")", sing, 0}
  ,
  {"?", help_command, CMD_INVIS}
  ,
  {null_ptr, null_ptr, 0}
  ,

  /* a */
  {null_ptr, null_ptr, 0}
  ,

  /* b */
  {null_ptr, null_ptr, 0}
  ,

  /* c */
  {"c", list_commands, CMD_INVIS}
  ,
  {"commands", list_commands, 0}
  ,
  {null_ptr, null_ptr, 0}
  ,

  /* d */
  {null_ptr, null_ptr, 0}
  ,

  /* e */
  {"emote", emote, 0}
  ,
  {null_ptr, null_ptr, 0}
  ,

  /* f */
  {null_ptr, null_ptr, 0}
  ,

  /* g */
  {null_ptr, null_ptr, 0}
  ,

  /* h */
  {"help", help_command, 0}
  ,
  {null_ptr, null_ptr, 0}
  ,

  /* i */
  {null_ptr, null_ptr, 0}
  ,

  /* j */
  {null_ptr, null_ptr, 0}
  ,

  /* k */
  {"kill_angel", kill_angel, CMD_INVIS}
  ,
  {"kill_arch", kill_arch, CMD_INVIS}
  ,
  {null_ptr, null_ptr, 0}
  ,

  /* l */
  {"lines", term_lines, 0}
  ,
  {"lsi", list_addresses, CMD_INVIS}
  ,
  {null_ptr, null_ptr, 0}
  ,

  /* m */
  {null_ptr, null_ptr, 0}
  ,

  /* n */
  {null_ptr, null_ptr, 0}
  ,

  /* o */
  {"ow", who_is_on, 0}
  ,
  {null_ptr, null_ptr, 0}
  ,

  /* p */
  {"password", password, 0}
  ,
  {"pemote", pemote, 0}
  ,
  {"prompt", set_prompt, 0}
  ,
  {null_ptr, null_ptr, 0}
  ,

  /* q */
  {"quit", quit_mud, 0}
  ,
  {null_ptr, null_ptr, 0}
  ,

  /* r */
  {"reboot", reboot_mud, 0}
  ,
  {"recap", recap, 0}
  ,
  {null_ptr, null_ptr, 0}
  ,

  /* s */
  {"say", say, 0}
  ,
  {"set_idle", setidle, CMD_INVIS}
  ,
  {"shutdown", shutdown_mud, 0}
  ,
  {"sing", sing, 0}
  ,
  {null_ptr, null_ptr, 0}
  ,

  /* t */
  {"tc", term_compare, 0}
  ,
  {"term", pick_term, 0}
  ,
  {"think", think, 0}
  ,
  {"time", mudtime, 0}
  ,
  {null_ptr, null_ptr, 0}
  ,

  /* u */
  {null_ptr, null_ptr, 0}
  ,

  /* v */
  {"version", version_func, 0}
  ,
  {"vlog", view_log, 0}
  ,
  {null_ptr, null_ptr, 0}
  ,

  /* w */
  {"w", nwho, CMD_INVIS}
  ,
  {"wake", wake, 0}
  ,
  {"wall", wall, 0}
  ,
  {"who", nwho, 0}
  ,
  {"width", term_width, 0}
  ,
  {null_ptr, null_ptr, 0}
  ,

  /* x */
  {null_ptr, null_ptr, 0}
  ,

  /* y */
  {null_ptr, null_ptr, 0}
  ,

  /* z */
  {null_ptr, null_ptr, 0}
  ,
};

command *cmd_list[27];
