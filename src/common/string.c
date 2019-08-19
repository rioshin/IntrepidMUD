/*
 * IntrepidMUD
 * Common Library
 * Common features between the different MUD executables
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

#include "../include/mudconfig.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "../include/proto.h"

unsigned char *end_string(unsigned char *str)
{
  /* Find the terminating null chracter */
  str = (unsigned char *)strchr((char *)str, null_chr);

  /* And skip it */
  str++;

  /* This position is where further things can be written to this patch of
     memory, without overriding whatever has been written there already */
  return str;
}

unsigned char *add_string(unsigned char *str)
{
  /* Find the terminating null character */
  str = (unsigned char *)strchr((char *)str, null_chr);

  /* Here we can continue writing to the current memory buffer */
  return str;
}

char *linestr(char *haystack, char *needle)
{
  char *ptr;

  /* If we don't even have anything to look in, return the null pointer */
  if (!haystack || !*haystack)
    return null_ptr;

  /* Find the string "needle" in the buffer "haystack". Note that the "needle"
     has to be at the start of the line */
  ptr = haystack;
  while ((ptr = strcasestr(ptr, needle)))
    if (ptr == haystack || *(ptr - 1) == '\n')
      return ptr;
    else
      ptr++;

  return null_ptr;
}

char *lineval(char *haystack, char *needle)
{
  unsigned char *oldstack = stack;
  char *line, *bufptr;
  char tmp_buf[1024];
  static char buf[1024];
  int32 multiline = 0;

  /* Add a ':' character to the needle value */
  sprintf((char *)stack, "%s:", needle);
  stack = end_string(stack);

  /* Find it at the beginning of a line */
  line = linestr(haystack, (char *)oldstack);
  stack = oldstack;

  /* Did we find anything? */
  if (!line || !*line)
    return null_ptr;

  /* Yes, scan for the ':' we added above, and then scan spaces and tabs
     away, since we're looking for whatever comes after them */
  memset(buf, null_chr, 1024);
  memset(tmp_buf, null_chr, 1024);
  while (*line && *line++ != ':');
  while (*line && (*line == ' ' || *line == '\t'))
    line++;

  /* Set the temporary buffer in shape */
  strncpy(tmp_buf, line, 1023);
  line = tmp_buf;
  bufptr = buf;

  while (!null)
  {
    /* Find the next linebreak */
    while (*line && *line != '\n')
      *bufptr++ = *line++;

    /* Ok, linebreak (or end of file) found, does the buffer continue with
       a tab character? */
    if (*line && *(line + 1) == '\t')
    {
      /* Yes, convert it to a space (and mark us as a multi-line entry) */
      *bufptr++ = ' ';
      line++;
      multiline = 1;

      /* Skip further tabs at the start of this line */
      while (*line == '\t')
        line++;
    }
    else
      break;
  }

  /* If we're a multi-line entry, we need to add a linebreak at the end */
  if (multiline)
    *bufptr++ = '\n';
  *bufptr = null_chr;

  /* Remove trailing spaces */
  if (*(bufptr - 1) == ' ')
  {
    bufptr--;
    while (*bufptr == ' ')
      bufptr--;

    /* And convert the final terminating linebreak (if we had spaces following
       it) to a null character */
    if (*bufptr == '\n')
      *++bufptr = null_chr;
  }

  /* And return the buffer */
  return buf;
}

char *strcasestr(char *haystack, char *needle)
{
  char *hptr;
  int32 nlen;

  /* Do we even have anything to look for? */
  /* If we don't, return the haystack */
  if (!needle || !*needle)
    return haystack;

  /* Or anything to look in? */
  if (!haystack || !*haystack)
    return null_ptr;

  /* Find the needle in the haystack */
  hptr = haystack;
  nlen = strlen(needle);
  while (*hptr)
  {
    /* Do we find it? */
    if (!strncasecmp(hptr, needle, nlen))
      return hptr;
    hptr++;
  }

  return null_ptr;
}

char *sys_time(void)
{
  time_t t;
  static char time_string[40];

  /* Get the current time */
  t = time(0);
  memset(time_string, null_chr, 40);

  /* And format it according to whatever is in our configuration message */
  if (!strcasecmp(get_config_message(config_message, "time_format"), "us"))
    strftime(time_string, 39, "%H:%M:%S - %m/%d/%Y", localtime(&t));
  else if (!strcasecmp
           (get_config_message(config_message, "time_format"), "uk"))
    strftime(time_string, 39, "%H:%M:%S - %m/%d/%Y", localtime(&t));
  else
    strftime(time_string, 39, "%H:%M:%S - %d/%m/%Y", localtime(&t));

  /* Return the nice time string */
  return time_string;
}

char *lower_case(char *str)
{
  static char copy[1024];
  char *tmp = copy;

  memset(copy, null_chr, 1024);
  strncpy(copy, str, 1023);

  /* Modify each character to its corresponding lower case equivalent */
  while (tmp && *tmp)
  {
    *tmp = tolower(*tmp);
    tmp++;
  }

  return copy;
}

char *get_number(int num)
{
  static char newnum[20];
  int i;

  for (i = 0; i < 20; i++)
    newnum[i] = '\0';

  sprintf(newnum, "%d", num);
  while (newnum[0] == ' ')
  {
    for (i = 0; i < 19; i++)
      newnum[i] = newnum[i + 1];
  }

  return newnum;
}

char *strprintf(char *where, string_func * fn, char *fmt, ...)
{
  va_list argum;
  char buf[65536];

  memset(buf, null_chr, 65536);

  va_start(argum, fmt);
  vsnprintf(buf, 65535, fmt, argum);
  va_end(argum);

  sprintf(where, "%s", buf);

  if (fn)
    where = (char *)(*fn) ((unsigned char *)where);

  return where;
}

int mud_strcmp(char *str1, char *str2)
{
  int retval = 0;

  while (*str1++ == *str2++)
  {
    ;
  }

  if ((*str1 == null_chr) && (*str2 != null_chr))
    retval = -(int)*str2;
  else if ((*str2 == null_chr) && (*str1 != null_chr))
    retval = (int)*str1;
  else
    retval = (int)*str1 - (int)*str2;

  return retval;
}

int mud_strncmp(char *str1, char *str2, int amount)
{
  int retval = 0;
  int current = 0;

  while ((current++ < amount) && (*str1++ == *str2++))
  {
    ;
  }

  if (current < amount)
  {
    if ((*str1 == null_chr) && (*str2 != null_chr))
      retval = -(int)*str2;
    else if ((*str2 == null_chr) && (*str1 != null_chr))
      retval = (int)*str1;
    else
      retval = (int)*str1 - (int)*str2;
  }

  return retval;
}

size_t mud_strlen(char *str)
{
  size_t len = 0;

  while (*str)
  {
    if (*str == '^')
    {
      str++;
      if (*str == '^')
        len++;
    }
    else
      len++;
    str++;
  }

  return len;
}
