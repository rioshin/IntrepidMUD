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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../include/proto.h"
#include "../include/user.h"

int32 get_line_width(user * u)
{
  if (!u)
    return DEFAULT_TERM_WIDTH;
  else if (u->detected_term_width)
    return u->term_width;
  else
    return u->default_term_width;
}

unsigned char *add_line(user * u, char *where, string_func * fn)
{
  unsigned char *orig = (unsigned char *)where;
  int32 width;

  width = get_line_width(u);

  where = strprintf(where, add_string, "^B>");
  width--;
  while (width > 1)
  {
    *where++ = '-';
    width--;
  }
  sprintf(where, "<^N\n");

  if (fn)
    orig = (*fn) (orig);

  return (unsigned char *)orig;
}

unsigned char *add_line_text(user * u, char *where, string_func * fn,
                             char *text)
{
  char buf[1024];
  unsigned char *orig = (unsigned char *)where;
  int32 len, half, count;
  int32 width;

  width = get_line_width(u);

  memset(buf, null_chr, 1024);
  sprintf(buf, "<^N %s ^B>", text);

  len = mud_strlen(buf);
  if (len > width && len < DEFAULT_TERM_WIDTH)
    width = DEFAULT_TERM_WIDTH;

  half = (width - len) / 2;

  count = 2;
  where = strprintf(where, add_string, "^B>-");
  while (count < half)
  {
    *where++ = '-';
    count++;
  }
  where = strprintf(where, add_string, buf);
  count += len;
  while (count < (width - 2))
  {
    *where++ = '-';
    count++;
  }
  sprintf(where, "-<^N\n");

  if (fn)
    orig = (*fn) (orig);

  return (unsigned char *)orig;
}

unsigned char *vadd_line_text(user * u, char *where, string_func * fn,
                              char *fmt, ...)
{
  va_list argum;
  char buf[1024];

  memset(buf, null_chr, 1024);
  va_start(argum, fmt);
  vsnprintf(buf, 1023, fmt, argum);
  va_end(argum);

  return add_line_text(u, where, fn, buf);
}
