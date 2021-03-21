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

#include <errno.h>
#include <sys/resource.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "../include/proto.h"
#include "../include/user.h"

unsigned char *stack_start, *stack;
char *action = "";
user *current_user;
int32 panic = 0, shut_down = 0;

int32 main(int32 argc, char *argv[])
{
  int32 port = 0;
  sigset_t ss;

  action = "boot";
  characters_list = null_ptr;

  if (init_framework(argc, argv))
  {
    fprintf(stderr, "\n\nFailed to initialize framework!\n\n");
    exit(-1);
  }

  stack_start = create_stack(500001);
  stack = stack_start;

  init_commands();
  create_user_dirs();
  create_char_dirs();
  load_areas();

  if (argv[1])
    port = atoi(argv[1]);

  if (!port)
    port = atoi(get_config_message(config_message, "port"));

  if (port < 1024 || port > 31999)
  {
    flog("boot", "Check your port configuration, setting to default 8765");
    port = 8765;
  }
  boot(port);
  log_pid();

  flog("boot", "Ready and listening for connections");

  while (!shut_down)
  {
    action = "stack check";
    errno = 0;
    if (stack != stack_start)
    {
      pint diff = (pint) stack - (pint) stack_start;

      diff = diff < 0 ? -diff : diff;
      stack = stack_start;
      vflog("stack", "Lost stack reclaimed %d bytes", diff);
    }

    action = "scan sockets";
    if (scan_sockets() > 0)
    {
      action = "processing users";
      process_users();
    }
    current_user = null_ptr;
    current_room = null_ptr;

    action = "timer function";
    timer_function();
    sigemptyset(&ss);
    sigsuspend(&ss);

    action = "alive ping";
    do_alive_ping();

    action = "";
  }

  close_down();
  return 0;
}

void boot(int32 port)
{
  struct rlimit rlp;
  struct itimerval new, old;
  struct sigaction sa;
#ifdef HAVE_SIGEMPTYSET
#elif __GNUC__ >= 7
  unsigned int i;
#endif

  vflog("boot", "Starting to boot up %s...",
        get_config_message(config_message, "mud_name"));

  getrlimit(RLIMIT_NOFILE, &rlp);
  rlp.rlim_cur = rlp.rlim_max;
  setrlimit(RLIMIT_NOFILE, &rlp);
  max_users = rlp.rlim_max - 20;

  getrlimit(RLIMIT_RSS, &rlp);
  rlp.rlim_cur = 1024 << 10;
  setrlimit(RLIMIT_RSS, &rlp);

  new.it_interval.tv_sec = 0;
  new.it_interval.tv_usec = (1000000 / TIMER_CLICK);
  new.it_value.tv_sec = 0;
  new.it_value.tv_usec = new.it_interval.tv_usec;
#ifdef HAVE_SIGEMPTYSET
  sigemptyset(&sa.sa_mask);
#elif __GNUC__ >= 7
  for (i = 0; i < _SIGSET_NWORDS; i++)
    sa.sa_mask.__val[i] = 0;
#else
  sa.sa_mask = (sigset_t) 0;
#endif
  sa.sa_flags = 0;
  sa.sa_handler = timing_function;

  if ((int32) sigaction(SIGALRM, &sa, 0) < 0)
    handle_error("Can't set timer signal.");
  if (setitimer(ITIMER_REAL, &new, &old) < 0)
    handle_error("Can't set timer.");
#ifdef DEBUG_VERBOSE
  flog("boot", "Timer started.");
#endif

  sa.sa_handler = sigpipe;
  sigaction(SIGPIPE, &sa, 0);
  sa.sa_handler = sighup;
  sigaction(SIGHUP, &sa, 0);
  sa.sa_handler = sigill;
  sigaction(SIGILL, &sa, 0);
  sa.sa_handler = sigfpe;
  sigaction(SIGFPE, &sa, 0);
  sa.sa_handler = sigbus;
  sigaction(SIGBUS, &sa, 0);
  sa.sa_handler = sigsegv;
  sigaction(SIGSEGV, &sa, 0);
  sa.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &sa, 0);
  sa.sa_handler = sigterm;
  sigaction(SIGTERM, &sa, 0);
  sa.sa_handler = sigxfsz;
  sigaction(SIGXFSZ, &sa, 0);
  sa.sa_handler = sigusr1;
  sigaction(SIGUSR1, &sa, 0);
  sa.sa_handler = sigusr2;
  sigaction(SIGUSR2, &sa, 0);
  sa.sa_handler = sigchld;
  sigaction(SIGCHLD, &sa, 0);

  intermud_port = port + 1;

  init_socket(port);
  alive_connect();

  current_users = 0;
}

void handle_error(char *msg)
{
  stack = stack_start;

  if (panic)
  {
    vflog("error", "PANIC shutdown: %s", msg);
    exit(-1);
  }

  panic = 1;
  stack = stack_start;

  flog("error", msg);
  flog("boot", "Abnormal exit from error handler");

  flog("dump", "----- Starting dump -----");
  vflog("dump", "Errno set to %d: %s", errno, strerror(errno));
  if (current_user)
  {
    vflog("dump", "Current user: %s", current_user->username);
    if (current_user->current_char)
    {
      vflog("dump", "Current character: %s %s",
            current_user->current_char->firstname,
            current_user->current_char->familyname);
      if (current_user->current_char->location)
      {
        vflog("dump", "Current location: [%s].[%s]",
              current_user->current_char->location->area->areaname,
              current_user->current_char->location->roomname);
      }
      else
        flog("dump", "No location detected");
    }
    else
      flog("dump", "No character detected");

    vflog("dump", "Input buffer: %s", current_user->input_buffer);
  }
  else
    flog("dump", "No current user detected");

  vflog("dump", "Current action: %s", action);
  flog("dump", "----- End of dump -----");

  tell_all_users("\n\n"
                 "     -=> *WIBBLE* Something bad has happened. Trying to save files <=-");
  close_down();
  exit(-1);
}

void close_down(void)
{
  struct itimerval new, old;

  vtell_all_users("^W\n\n     --==>> %s shutting down NOW <<==--\n\n",
                  get_config_message(config_message, "mud_name"));

  new.it_interval.tv_sec = 0;
  new.it_interval.tv_usec = 0;
  new.it_value.tv_sec = 0;
  new.it_value.tv_usec = 0;
  if (setitimer(ITIMER_REAL, &new, &old) < 0)
    handle_error("Can't set timer.");

  /*
   * TODO: Add syncing of users...
   */

  unlink(MUD_PID);
  if (!panic)
    flog("boot", "Program exited normally.");
  exit(0);
}

void log_pid(void)
{
  FILE *f;

  f = fopen(MUD_PID, "w");
  if (!f)
  {
    fprintf(stderr,
            "log_pid(): Couldn't open \"" MUD_PID "\" for writing!\n");
    exit(-1);
  }
  fprintf(f, "%d", getpid());
  fflush(f);
  fclose(f);
}
