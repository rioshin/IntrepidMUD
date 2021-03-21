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

#define __need_timespec

#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "../include/proto.h"
#include "../include/clist.h"
#include "../include/realm.h"

#undef __need_timespec

realm_data the_realm = { 0, 0, 1, 1, 0, 0 };

void timing_function(int32 c)
{
  static char hit_time[20];
  char now[20];
  time_t t;

  t = time(0);
  (void)strftime(now, 20, "%H:%M:%S", localtime(&t));
  if (!strcasecmp(hit_time, now))
    return;
  else
    strcpy(hit_time, now);

  actual_timer(c);
}

void actual_timer(int32 c)
{
  user *scan;

  if (c)
    c = 0;

  for (scan = users_list; scan; scan = scan->next)
    scan->time_idle++;

#if !defined(hpux) && !defined(linux)
  if ((pint) signal(SIGALRM, actual_timer) < 0)
    handle_error("Can't set timer signal.");
#endif                          /* !hpux && !linux */

  increase_time(&the_realm);
}

void increase_time(realm_data * realm)
{
  realm->min++;

  if (realm->min == 60)
  {
    realm->min = 0;
    realm->hour++;

    if (realm->hour == 24)
    {
      realm->hour = 0;
      realm->day++;

      if (realm->day == 32)
      {
        realm->day = 1;
        realm->month++;

        if (realm->month == 13)
        {
          realm->month = 1;
          realm->year++;

          if (realm->year == 1000)
          {
            realm->year = 0;
            realm->epoch++;

            if (realm->epoch == 26)
              realm->epoch = 0;
          }
        }
      }
    }
  }
}

void timer_function(void)
{
}

void process_users(void)
{
  user *scan;

  for (scan = users_list; scan; scan = scan->next)
  {
    if (scan->input_flags & INPUT_READY)
      input_for_one(scan);

    do_prompt(scan);
  }
}

void input_for_one(user * u)
{
  u->input_flags &= ~INPUT_READY;

  memset(u->special_prompt, null_chr, MAX_PROMPT);

  if (u->fn_input)
    (*u->fn_input) (u, u->input_buffer);
  else if (u->input_buffer[0] != null_chr)
    match_command(u, u->input_buffer);
  else
    tell_user(u, "");
}

char *list_matched_commands(char *ptr, command * com_entry)
{
  command *com_scan = com_entry;

  for (; com_scan->func != null_ptr; com_scan++)
  {
    if (com_scan->flags & CMD_MATCH)
    {
      ptr = strprintf(ptr, add_string, "%s ", com_scan->cmd);
      com_scan->flags &= ~CMD_MATCH;
    }
  }

  return ptr;
}

void match_command(user * u, char *str)
{
  char *space;
  command *cmdlist;

  if (!str || !*str)
    return;

  while (*str && isspace(*str))
    str++;

  space = (char *)add_string((unsigned char *)str);
  space--;
  while (isspace(*space))
    *space-- = null_chr;

  if (!str || !*str)
    return;

  if (isalpha(*str))
    cmdlist = cmd_list[((int32) (tolower(*str)) - (int32) 'a' + 1)];
  else
    cmdlist = cmd_list[0];

  match_command_real(u, str, cmdlist);
}

void match_command_real(user * u, char *str, command * cmdlist)
{
  int32 matched = 0, hits = 0;
  command *matchcmd, *scancmd;

  matchcmd = null_ptr;

  for (scancmd = cmdlist; scancmd->func != null_ptr; scancmd++)
  {
    matched = do_match_exact(str, scancmd);
    if (matched)
    {
      matchcmd = scancmd;
      break;
    }
  }

  if (!matched)
  {
    for (scancmd = cmdlist; scancmd->func != null_ptr; scancmd++)
    {
      matched = do_match(str, scancmd);
      if (matched)
      {
        scancmd->flags |= CMD_MATCH;
        matchcmd = scancmd;
        hits++;
      }
    }

    if (!hits)
    {
      tell_user(u, "Error! No command could be matched!\n");
      return;
    }
    else if (hits > 1)
    {
      unsigned char *oldstack = stack;

      matchcmd = null_ptr;
      strcpy((char *)stack, "Error: Multiple commands matched: ");
      stack = add_string(stack);
      stack =
        (unsigned char *)list_matched_commands((char *)stack, cmdlist);
      vtell_user(u, "%s\n", (char *)oldstack);
      stack = oldstack;
      return;
    }

    for (scancmd = cmdlist; scancmd->func != null_ptr; scancmd++)
      scancmd->flags &= ~CMD_MATCH;
  }

  if (matchcmd != null_ptr)
  {
    u->time_idle = 0;
    str = do_match_modify(str, matchcmd);
    (*matchcmd->func) (u, str);
  }
  else
    tell_user(u, "Error! Command matching failed for some reason!\n");
}

int32 do_match_exact(char *str, command * com_entry)
{
  char *scan;

  for (scan = com_entry->cmd; *scan && *str && !isspace(*str);
       scan++, str++)
    if (tolower(*str) != tolower(*scan))
      return 0;

  if ((com_entry->flags & CMD_SPACE) && (*str) && (!isspace(*str)))
    return 0;

  if (*scan)
    return 0;

  return 1;
}

int32 do_match(char *str, command * com_entry)
{
  char *scan;

  for (scan = com_entry->cmd; *scan && *str && !isspace(*str);
       scan++, str++)
    if (tolower(*str) != tolower(*scan))
      return 0;

  if ((com_entry->flags & CMD_SPACE) && (*str) && (!isspace(*str)))
    return 0;

  if (*scan && (com_entry->flags & CMD_INVIS))
    return 0;

  return 1;
}

char *do_match_modify(char *str, command * com_entry)
{
  char *str_back = str, *scan;

  for (scan = com_entry->cmd; *scan && *str && !isspace(*str);
       scan++, str++)
    if (tolower(*str) != tolower(*scan))
      return str_back;

  if ((com_entry->flags & CMD_SPACE) && (*str) && (!isspace(*str)))
    return str_back;

  while (*str && isspace(*str))
    str++;

  return str;
}

void init_commands(void)
{
  int32 i;
  command *scan;

  scan = full_list;

  for (i = 0; i < 27; i++)
  {
    cmd_list[i] = scan;

    while (scan->cmd)
    {
      if (isalpha(scan->cmd[strlen(scan->cmd) - 1]))
        scan->flags |= CMD_SPACE;

      scan++;
    }

    scan++;
  }
}

char *next_space(char *str)
{
  while (*str && *str != ' ')
    str++;

  if (*str == ' ')
  {
    while (*str == ' ')
      str++;
    str--;
  }

  return str;
}

void mudtime(user * u, char *str)
{
  TESTVARP(u);
  TESTVARP(str);
}
