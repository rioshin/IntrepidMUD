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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/telnet.h>

#include "../include/proto.h"

user *users_list, *current_user;

void tell_all_users(char *msg)
{
  user *scan;

  for (scan = users_list; scan; scan = scan->next)
    if (scan->logged_in != 0)
      tell_user(scan, msg);
}

void vtell_all_users(char *fmt, ...)
{
  va_list argum;
  unsigned char *oldstack = stack;

  va_start(argum, fmt);
  vsprintf((char *)stack, fmt, argum);
  va_end(argum);

  stack = end_string(stack);
  tell_all_users((char *)oldstack);
  stack = oldstack;
}

void tell_all_users_but(user * u, char *msg)
{
  user *scan;

  for (scan = users_list; scan; scan = scan->next)
  {
    if (scan->logged_in == 0)
      continue;
    if (scan == u)
      continue;

    tell_user(scan, msg);
  }
}

void vtell_all_users_but(user * u, char *fmt, ...)
{
  va_list argum;
  unsigned char *oldstack;

  oldstack = stack;
  va_start(argum, fmt);
  vsprintf((char *)stack, fmt, argum);
  va_end(argum);

  stack = end_string(stack);
  tell_all_users_but(u, (char *)oldstack);
  stack = oldstack;
}

void tell_user(user * u, char *msg)
{
  char *scan;
  int32 lines = 0, orig_lines = u->term_lines;

  if (u->term_lines == 0)
    u->term_lines = u->default_term_lines;

  if (u->pager)
  {
    tell_user_normal(u, msg);
    u->term_lines = orig_lines;
    return;
  }

  for (scan = msg; *scan; scan++)
    if (*scan == '\n')
      lines++;

  if (lines > u->term_lines)
    tell_user_paged(u, msg);
  else
    tell_user_normal(u, msg);

  u->term_lines = orig_lines;
}

void vtell_user(user * u, char *fmt, ...)
{
  va_list argum;
  unsigned char *oldstack = stack;

  va_start(argum, fmt);
  vsprintf((char *)stack, fmt, argum);
  va_end(argum);

  stack = end_string(stack);
  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void tell_user_normal(user * u, char *msg)
{
  user *old_current = current_user;

  current_user = u;
  send_to_user(u, msg);
  do_prompt(u);
  current_user = old_current;
}

void tell_user_paged(user * u, char *msg)
{
  char *scan;
  int32 length = 0, lines = 0;
  pager *p;

  if (u->pager)
  {
    tell_user(u, " Eeek, can't enter pager right now.\n"
              " (try quitting the current pager using the 'q' command)\n");
    return;
  }

  for (scan = msg; *scan; scan++, length++)
  {
    if (*scan == '\n')
      lines++;
    length++;
  }

  if (lines > u->term_lines)
  {
    p = (pager *) malloc(sizeof(pager));
    memset(p, null, sizeof(pager));
    u->pager = p;
    p->buffer = (char *)malloc(length + 1);
    memset(p->buffer, null_chr, length);
    memcpy(p->buffer, msg, strlen(msg));
    p->buffer[strlen(msg)] = null_chr;
    p->current = p->buffer;
    p->max_size = lines;
    p->size = 0;
    p->input_copy = u->fn_input;
    u->fn_input = pager_fn;

    draw_page(u, msg);
  }
  else
    tell_user_normal(u, msg);
}

int32 draw_page(user * u, char *msg)
{
  int32 end_line = 0, n;
  pager *p;
  unsigned char *oldstack = stack;
  float pdone;

  for (n = (u->term_lines - 1); n; n--, end_line++)
  {
    while (*msg && *msg != '\n')
      *stack++ = *msg++;
    if (!*msg)
      break;
    *stack++ = *msg++;
  }
  *stack++ = null_chr;

  memset(u->special_prompt, null_chr, MAX_PROMPT);
  if (*msg && u->pager)
  {
    p = u->pager;
    end_line += p->size;
    pdone = ((float)end_line / (float)p->max_size) * 100.0;
    snprintf(u->special_prompt, MAX_PROMPT - 1,
             "[Pager: %d-%d (%d) [%.0f%%] <command>/[ENTER]/b/t/q] ",
             p->size, end_line, p->max_size, pdone);
    strcpy(u->pager_prompt, u->special_prompt);
  }

  tell_user_normal(u, (char *)oldstack);
  stack = oldstack;

  return *msg;
}

void pager_fn(user * u, char *str)
{
  pager *p;

  memset(u->special_prompt, null_chr, MAX_PROMPT);

  p = u->pager;

  switch (tolower(*str))
  {
  case 'b':
  case 'p':
    back_page(u, p);
    break;

  case null_chr:
  case 'n':
    forward_page(u, p);
    break;

  case 't':
    p->current = p->buffer;
    p->size = 0;
    break;

  case 'q':
    quit_pager(u, p);
    return;
    break;
  }

  if (!draw_page(u, p->current))
    quit_pager(u, p);
}

void quit_pager(user * u, pager * p)
{
  u->fn_input = p->input_copy;

  if (p->buffer)
    free(p->buffer);
  free(p);
  u->pager = null_ptr;

  memset(u->pager_prompt, null_chr, MAX_PROMPT);
}

void back_page(user * u, pager * p)
{
  char *scan;
  int32 n;

  scan = p->current;
  for (n = u->term_lines; n; n--)
  {
    while (scan != p->buffer && *scan != '\n')
      scan--;
    if (scan == p->buffer)
      break;
    p->size--;
    scan--;
  }

  p->current = scan;
}

void forward_page(user * u, pager * p)
{
  char *scan;
  int32 n;

  scan = p->current;
  for (n = u->term_lines; n; n--)
  {
    while (*scan && *scan != '\n')
      scan++;
    if (!*scan)
      break;
    p->size++;
    scan++;
  }

  p->current = scan;
}

void backspace(user * u)
{
  u->input_buffer[u->input_pointer] = null_chr;
  if (u->input_pointer > 0)
    u->input_pointer--;
}

void do_prompt(user * u)
{
  unsigned char *oldstack = stack;

  if (u->special_prompt[0] != null_chr)
    stack =
      (unsigned char *)strprintf((char *)stack, add_string, "^N%s^N",
                                 u->special_prompt);
  else
    stack =
      (unsigned char *)strprintf((char *)stack, add_string, "^N%s^N",
                                 u->normal_prompt);
  if (u->conn_flags & CONN_IAC_GA)
  {
    *stack++ = IAC;
    *stack++ = GA;
  }
  else if (u->conn_flags & CONN_IAC_EOR)
  {
    *stack++ = IAC;
    *stack++ = TELOPT_EOR;
  }
  *stack++ = null_chr;
  send_to_user(u, (char *)oldstack);
  stack = oldstack;
}

void tell_character(character * c, char *str)
{
  tell_user(c->the_user, str);
}

void vtell_character(character * c, char *fmt, ...)
{
  va_list argum;
  unsigned char *oldstack = stack;

  va_start(argum, fmt);
  vsprintf((char *)stack, fmt, argum);
  va_end(argum);

  stack = end_string(stack);
  tell_user(c->the_user, (char *)oldstack);
  stack = oldstack;
}
