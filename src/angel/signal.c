/*
 * IntrepidMUD
 * Guardian Angel
 * Watches over the MUD server and reboots it when it goes down
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

#define __need_timespec

#include <signal.h>
#include <time.h>
#include <sys/types.h>

#include "../include/proto.h"

#undef __need_timespec

void sigpipe(int32 c)
{
  TESTVARV(c);
  /* We received a SIGPIPE - we don't want to handle it and continue, so
     exit the program while writing to the error log */
  error("Sigpipe received.");
}

void sighup(int32 c)
{
  TESTVARV(c);
  /* We got a SIGHUP - send it on to the MUD server and start the process
     of shutting it down */
  kill(fh, SIGHUP);
  die = 1;
}

void sigquit(int32 c)
{
  TESTVARV(c);
  /* We received a SIGQUIT - we don't want to handle it and continue, so
     exit the program while writing to the error log */
  error("Quit signal received.");
}

void sigill(int32 c)
{
  TESTVARV(c);
  /* We received a SIGILL - we don't want to handle it and continue, so
     exit the program while writing to the error log */
  error("Illegal instruction.");
}

void sigfpe(int32 c)
{
  TESTVARV(c);
  /* We received a SIGFPE - we don't want to handle it and continue, so
     exit the program while writing to the error log */
  error("Floating Point Error.");
}

void sigbus(int32 c)
{
  TESTVARV(c);
  /* We received a SIGBUS - we don't want to handle it and continue, so
     exit the program while writing to the error log */
  error("Bus Error.");
}

void sigsegv(int32 c)
{
  TESTVARV(c);
  /* We received a SIGSEGV - we don't want to handle it and continue, so
     exit the program while writing to the error log */
  error("Segmentation Violation.");
}

void sigterm(int32 c)
{
  TESTVARV(c);
  /* We received a SIGTERM - we don't want to handle it and continue, so
     exit the program while writing to the error log */
  error("Terminate signal received.");
}

void sigxfsz(int32 c)
{
  TESTVARV(c);
  /* We received a SIGXFSZ - we don't want to handle it and continue, so
     exit the program while writing to the error log */
  error("File descriptor limit exceeded.");
}

void sigchld(int32 c)
{
  TESTVARV(c);
  /* We got a SIGCHLD, just log the fact */
  flog("angel", "Received SIGCHLD");
  return;
}
