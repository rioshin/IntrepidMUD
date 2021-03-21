/*
 * IntrepidMUD
 * Common Library
 * Common features between the different MUD executables
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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/telnet.h>
#include <unistd.h>
#include <sys/select.h>
#include <linux/errno.h>

#include "../include/proto.h"
#include "../include/user.h"
#include "../include/un.h"

void send_to_user(user * u, char *str)
{
  file output;

  if (u->fd < 0)
    return;
  if (current_user != u)
    return;

  output = process_output(u, str);

  if (!write_socket(&u->fd, output.where, output.length))
  {
    //quit_user(u);
    ;
  }
}

file process_output(user * u, char *str)
{
  file out;
  char *scan;
  int32 length, spaces, column, orig_width = u->term_width;

  out.where = (char *)stack;
  column = 0;

  if (u->term_width == 0)
    u->term_width = u->default_term_width;

  while (*str)
  {
    /* handle new line */
    if (*str == '\n')
    {
      *stack++ = '\r';
      *stack++ = *str++;
      column = 0;
      continue;
    }
    else
    {
      length = 0;
      spaces = 0;
      scan = str;

      /* count spaces */
      while (*scan && isspace(*scan))
      {
        if (*scan == '\n')
          break;
        scan++;
        spaces++;
        length++;
      }

      /* only spaces followed by a new-line? */
      if (*scan && *scan == '\n')
      {
        /* let's have the new-line next */
        str = scan;
        continue;
      }

      /* count characters following the spaces */
      while (*scan && !isspace(*scan))
      {
        if (*scan == '^')
        {
          scan++;
          if (*scan && (*scan == '^'))
            length++;
        }
        else
          length++;
        scan++;
      }

      if (column + length > u->term_width)
      {
        /* we go too wide, add a new-line */
        *stack++ = '\r';
        *stack++ = '\n';
        column = 0;

        /* skip the spaces */
        while (*str && isspace(*str))
          str++;
      }

      /* copy to output */
      while (str < scan)
      {
        if (*str == '^')
        {
          if (*(str + 1) && (*(str + 1) == '^'))
          {
            *stack++ = *str++;
            str++;
            column++;
          }
          else if (!(*(str + 1)))
            str++;
          else
          {
            str++;
            stack =
              (unsigned char *)strprintf((char *)stack, add_string,
                                         get_color_code(u, *str++));
          }
        }
        else
        {
          *stack++ = *str++;
          column++;
        }
      }
    }
  }
  *stack++ = null_chr;

  u->term_width = orig_width;

  out.length = strlen(out.where);
  stack = (unsigned char *)out.where;

  return out;
}

char *get_color_code(user * u, char c)
{
  int32 rnd1, rnd2;
  static char ret[16];

  if (u->term_type == TERM_NONE)
    return "";

  rnd1 = (rand() % 8) + 1;
  rnd2 = (rand() % 2);

  switch (c)
  {
  case 'X':
    if (rnd1 < 8)
      sprintf(ret, "\033[1;3%dm", rnd1);
    else
      return "\033[0;1m";
    return ret;
    break;

  case 'x':
    if (rnd1 < 8)
      sprintf(ret, "\033[0;3%dm", rnd1);
    else
      return "\033[0;1m";
    return ret;
    break;

  case 'q':
  case 'Q':
    if (rnd1 < 8)
      sprintf(ret, "\033[%d;3%dm", rnd2, rnd1);
    else
      return "\033[0;1m";
    return ret;
    break;

  case 'S':
  case 's':
    return "\033[3m";
    break;

  case 'K':
  case 'k':
    return "\033[5m";
    break;

  case 'U':
  case 'u':
    return "\033[4m";
    break;

  case 'I':
  case 'i':
    return "\033[7m";
    break;

  case 'H':
  case 'h':
    return "\033[0;1m";
    break;

  case 'r':
    return "\033[0;31m";
    break;

  case 'R':
    return "\033[1;31m";
    break;

  case 'y':
    return "\033[0;33m";
    break;

  case 'Y':
    return "\033[1;33m";
    break;

  case 'g':
    return "\033[0;32m";
    break;

  case 'G':
    return "\033[0;32m";
    break;

  case 'p':
    return "\033[0;35m";
    break;

  case 'P':
    return "\033[1;35m";
    break;

  case 'c':
    return "\033[0;36m";
    break;

  case 'C':
    return "\033[1;36m";
    break;

  case 'b':
    return "\033[0;34m";
    break;

  case 'B':
    return "\033[1;34m";
    break;

  case 'a':
    return "\033[1;30m";
    break;

  case 'A':
    return "\033[0;37m";
    break;

  case 'n':
  case 'N':
    switch (u->term_type)
    {
    case TERM_XTERM:
    case TERM_VT220:
    case TERM_VT100:
    case TERM_VT102:
    case TERM_SUN:
      return "\033[m";
      break;

    case TERM_ANSI:
      return "\033[0m";
      break;

    case TERM_TVI912:
      return "\033m";
      break;

    case TERM_ADM:
      return "\033(";
      break;

    case TERM_HP2392:
      return "\033&d@";
      break;

    default:
      return "";
      break;
    }
    break;

  case 'W':
    return "\007";
    break;

  default:
    return "";
    break;
  }

  return "";
}

ssize_t write_socket(int32 * fd, const void *where, size_t length)
{
  ssize_t output_length = 0;

  if (!length)
    return 1;

  if (*fd > -1)
  {
    if ((output_length = write(*fd, where, length)) == -1)
    {
      switch (errno)
      {
      case EINTR:
      case EAGAIN:
      case ENOSPC:
        return output_length;
        break;

      default:
        close(*fd);
        *fd = -1;
        return 0;
        break;
      }
    }

    return output_length;
  }

  return 0;
}
