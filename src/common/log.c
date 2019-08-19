/* IntrepidMUD
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

#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/proto.h"

void error(char *msg)
{
  /* Just log the error message and exit the program */
  flog("error", msg);
  exit(1);
}

void verror(char *fmt, ...)
{
  /* Varargs version of above; first get the arguments in shape and then
     log the error message before exiting the program */
  va_list argum;
  unsigned char *oldstack;

  oldstack = stack;
  va_start(argum, fmt);
  vsprintf((char *)stack, fmt, argum);
  va_end(argum);

  stack = end_string(stack);
  flog("error", (char *)oldstack);
  stack = oldstack;

  exit(1);
}

void flog(char *logfile, char *msg)
{
  int32 fd, length, maxl;
  char path[160];
  unsigned char *oldstack = stack;
  struct stat sbuf;

  /* Set the path for the log file */
  memset(path, null_chr, 160);
  sprintf(path, "files/logs/%s.log", logfile);

  /* Does the log file already exist? */
  if (stat(path, &sbuf) < 0)
  {
    /* No, create it */
    fd = creat(path, (S_IRUSR | S_IWUSR));

    /* Check required priviledge to access the logfile, and set the system
       priv of the file to mark it */
    length = log_required_priv(get_config_message(log_message, logfile));
    if (length > 0)
      chmod(path, S_IWUSR | length);
    length = 0;
  }
  else
  {
    /* Yes, just open it */
#ifdef BSDISH
    fd = open(path, (O_RDWR | O_CREAT | O_APPEND), (S_IRUSR | S_IWUSR));
#else
    fd =
      open(path, (O_RDWR | O_CREAT | O_SYNC | O_APPEND),
           (S_IRUSR | S_IWUSR));
#endif
    length = sbuf.st_size;
  }

  /* If it's in the bug or idea logs, don't care about the maximum log size
     configuration value */
  if (strcasecmp(logfile, "bug") && strcasecmp(logfile, "idea"))
  {
    /* Not in one of the above logfiles, so find out the largest size of a
       log file we allow */
    maxl = atoi(get_config_message(log_message, "max_log_size")) * 1024;

    /* With this log message we're adding, does the file grow too big? */
    if (((int32) (length + strlen((char *)msg))) > maxl)
    {
      /* Yes, read in the file */
      lseek(fd, 0, SEEK_SET);
      read(fd, stack, length);
      close(fd);

      /* And truncate the one in the file system */
      fd = open(path, (O_WRONLY | O_CREAT | O_TRUNC), (S_IRUSR | S_IWUSR));

      /* Make sure we have a terminating null char at the end */
      stack[length] = null_chr;

      /* And go forward enough characters to make sure we don't exceed the
         maximum length configured */
      stack += strlen((char *)msg);

      /* Keep skipping characters until we find a newline */
      while (*stack && *stack != '\n')
        stack++;

      /* If we have anything left, write it to the file */
      if (*stack)
      {
        stack++;
        write(fd, stack, strlen((char *)stack));
      }
      stack = oldstack;
    }
  }

  /* And finally write our own log file entry */
  sprintf((char *)stack, "%s - %s\n", sys_time(), msg);
  write(fd, stack, strlen((char *)stack));
  oldstack = stack;
  stack += strlen((char *)stack);
  send_to_debug((char *)oldstack);
  stack = oldstack;
  close(fd);
}

void vflog(char *logfile, char *fmt, ...)
{
  /* This is just a varargs wrapper around the previous function */
  va_list argum;
  unsigned char *oldstack = stack;

  va_start(argum, fmt);
  vsprintf((char *)stack, fmt, argum);
  va_end(argum);

  stack = end_string(stack);
  flog(logfile, (char *)oldstack);
  stack = oldstack;
}

int32 log_required_priv(char *str)
{
  /* Logs for admins only are user read-write only */
  if (!strncasecmp(str, "ad", 2))
    return S_IRUSR;
  /* Logs for lower admins as well as admins also have group-read privs */
  if (!strcasecmp(str, "la") || !strcasecmp(str, "lower_admin"))
    return S_IRUSR | S_IRGRP;
  /* And for superusers, they have even read privs for others */
  if (!strncasecmp(str, "su", 2))
    return S_IRUSR | S_IRGRP | S_IROTH;

  /* Oops, we have a failure */
  return -1;
}
