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

#include <sys/socket.h>
#include <unistd.h>

#include "../include/proto.h"

void sigpipe(int32 c)
{
  TESTVARV(c);
  if (current_user)
  {
    vflog("sigpipe", "Closing connection due to SIGPIPE. [%s]",
          current_user->username);
    shutdown(current_user->fd, 2);
    close(current_user->fd);
  }
  else
    vflog("sigpipe", "SIGPIPE received, last action: %s", action);
}

void sighup(int32 c)
{
  TESTVARV(c);
  flog("boot", "SIGHUP caught, ignoring...");
}

void sigquit(int32 c)
{
  TESTVARV(c);
  handle_error("SIGQUIT received.");
}

void sigill(int32 c)
{
  TESTVARV(c);
  handle_error("SIGILL received.");
}

void sigfpe(int32 c)
{
  TESTVARV(c);
  handle_error("SIGFPE received.");
}

void sigbus(int32 c)
{
  TESTVARV(c);
  handle_error("SIGBUS received.");
}

void sigsegv(int32 c)
{
  TESTVARV(c);
  handle_error("Segmentation Violation.");
}

void sigsys(int32 c)
{
  TESTVARV(c);
  handle_error("SIGSYS received.");
}

void sigterm(int32 c)
{
  TESTVARV(c);
  handle_error("SIGTERM received.");
}

void sigxfsz(int32 c)
{
  TESTVARV(c);
  handle_error("SIGXFSZ received.");
}

void sigusr1(int32 c)
{
  TESTVARV(c);
  /*
   * TODO: Add code to manage forking and syncing the users
   */
}

void sigusr2(int32 c)
{
  TESTVARV(c);
  /*
   * TODO: Add code to perform backup
   */
}

void sigchld(int32 c)
{
  TESTVARV(c);
}
