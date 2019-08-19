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

#define __need_siginfo_t

#include <signal.h>
#if __GNUC__ < 7
# include <bits/siginfo.h>
#else
# include <bits/types/siginfo_t.h>
#endif
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "../include/proto.h"
#include "../include/user.h"

char smbuf[1024];

char *get_config_message(file config_file, char *type)
{
  char *got;

  /* Clear our buffer */
  memset(smbuf, null_chr, 1024);

  /* Check if we have loaded the config file */
  if (!config_file.where || !*(config_file.where))
  {
    /* In case we're missing it, get us valid defaults */
    if (!strcasecmp(type, "max_log_size"))
      return "5000";
    if (!strcasecmp(type, "time_format"))
      return "us";

    flog("error", "Soft configuration file isn't loaded!");
    return "error";
  }

  /* Find the entry from it */
  got = lineval(config_file.where, type);
  if (!got || !*got)
  {
    /* Oops, entry is missing; get some defaults out */
    if (!strcasecmp(type, "logon_prefix")
        || !strcasecmp(type, "logon_suffix")
        || !strcasecmp(type, "logoff_prefix")
        || !strcasecmp(type, "logoff_suffix")
        || !strcasecmp(type, "site_alias")
        || !strcasecmp(type, "welcome_msg"))
      return "";
    if (!strcasecmp(type, "max_log_size"))
      return "5000";
    if (!strcasecmp(type, "time_format"))
      return "us";

    /* And if it's a message we're not handling by default, log the error */
    vflog("error", "Soft configuration message for '%s' isn't there!",
          type);
    return "error";
  }

  /* Set up our return buffer and give the data to the caller */
  strncpy(smbuf, got, 1023);
  return smbuf;
}
