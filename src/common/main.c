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

#include <string.h>
#include <unistd.h>

#include "../include/proto.h"

file config_message;
file log_message;
file messages_message;
file prompts_message;

int32 init_framework(int32 argc, char *argv[])
{
  /* Initialize the root directory */
  int32 ret = init_root(argc, argv);

  /* If we failed, exit immediately (otherwise, we'll crash when trying to
     load configuration files) */
  if (ret)
    return ret;

  /* Load the configuration files */
  config_message = load_file("files/config/config.msg");
  log_message = load_file("files/config/log.msg");
  messages_message = load_file("files/config/messages.msg");
  prompts_message = load_file("files/config/prompts.msg");

  /* And return the return value received - note, this is 0 anyway :) */
  return ret;
}

int32 init_root(int32 argc, char *argv[])
{
  char cwd[256], buf[512], *scan;
  int tmp = argc;

  if (tmp)
    tmp = 0;

  memset(cwd, null_chr, 256);
  memset(buf, null_chr, 512);
  if ('/' != *argv[0])
  {
    /* Program location given is relative to current working directory */
    if (!getcwd(cwd, 255))
      return -1;
    strcpy(buf, cwd);

    /* Ensure working directory has trailing slash */
    if (buf[strlen(buf) - 1] != '/')
      strcat(buf, "/");
  }

  if (strchr(argv[0], '/'))
  {
    /* Need additional path from commandline */
    strcat(buf, argv[0]);

    /* Trim off program filename */
    scan = strrchr(buf, '/');
    *scan = null_chr;
  }

  /* Ensure working directory has trailing slash */
  if (buf[strlen(buf) - 1] != '/')
    strcat(buf, "/");

  /* Application root is parent of bin/ directory */
  scan = buf + (strlen(buf) - 5);
  if (!strcmp(scan, "/bin/"))
    buf[strlen(buf) - 5] = '\0';

  /* Try to change the directory, returning whatever chdir returns */
  return chdir(buf);
}
