/*
 * IntrepidMUD
 * Guardian Angel
 * Watches over the MUD server and reboots it when it goes down
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
#include <sys/ioctl.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include "../include/proto.h"
#include "../include/un.h"

unsigned char *stack_start, *stack;
char angel_name[256], mud_name[256];
int32 fh = 0, die = 0, crashes = 0, syncing = 0, no_tty = 0;
int32 time_out = 0, t = 0;

extern int nice(int);

int32 main(int32 argc, char *argv[])
{
  FILE *angel_pid_fd;
  struct sigaction siga;

  /* Create the stack for the Guardian Angel */
  stack_start = create_stack(5000);
  stack = stack_start;

  /* Load required messages as well as set the correct path */
  if (init_framework(argc, argv))
  {
    fprintf(stderr, "\n\nFailed to initialize framework!\n\n");
    exit(-1);
  }

  /* Create name of Guardian Angel */
  sprintf(angel_name, "-=> %s <=- Angel watching port",
          get_config_message(config_message, "mud_name"));

  /* Create file containing process id, removing any previous one */
  unlink(ANGEL_PID);
  if (!(angel_pid_fd = fopen(ANGEL_PID, "w")))
  {
    fprintf(stderr, "Unable to create pid log file!\n");
    exit(1);
  }
  fprintf(angel_pid_fd, "%d\n", (int32) getpid());
  fclose(angel_pid_fd);

  /* Are we already running with corrected name? */
  if (strcmp(angel_name, argv[0]))
  {
    /* No, start us again with the name set to the nice one */
    if (!argv[1])
    {
      sprintf((char *)stack, "%d",
              atoi(get_config_message(config_message, "port")));
      execlp("bin/" ANGEL_EXEC, angel_name, stack, null_ptr);
    }
    else
    {
      argv[0] = angel_name;
      execvp("bin/" ANGEL_EXEC, argv);
    }
    error("exec failed");
  }

  /* Yes, try to nice us */
  if (nice(5) < 0)
    error("Failed to renice");

  /* Set global variables for timing purposes */
  t = time(0);
  time_out = t + 60;

  /* Set required signal handlers (even if they're empty!) */
#ifdef HAVE_SIGEMPTYSET
  sigemptyset(&siga.sa_mask);
#else
  siga.sa_mask = 0;
#endif
  siga.sa_flags = 0;

  siga.sa_handler = sigpipe;
  sigaction(SIGPIPE, &siga, 0);
  siga.sa_handler = sighup;
  sigaction(SIGHUP, &siga, 0);
  siga.sa_handler = sigquit;
  sigaction(SIGQUIT, &siga, 0);
  siga.sa_handler = sigill;
  sigaction(SIGILL, &siga, 0);
  siga.sa_handler = sigfpe;
  sigaction(SIGFPE, &siga, 0);
  siga.sa_handler = sigbus;
  sigaction(SIGBUS, &siga, 0);
  siga.sa_handler = sigsegv;
  sigaction(SIGSEGV, &siga, 0);
  siga.sa_handler = sigterm;
  sigaction(SIGTERM, &siga, 0);
  siga.sa_handler = sigxfsz;
  sigaction(SIGXFSZ, &siga, 0);
  siga.sa_handler = sigchld;
  sigaction(SIGCHLD, &siga, 0);

  /* And start the MUD server itself */
  return handle_server(argc, argv);
}

int32 handle_server(int32 argc, char *argv[])
{
#ifdef HAVE_SOCKLEN_T
  socklen_t length;
#else
  uint32 length;
#endif
  int32 status, alive_fd = 0, sock_fd, dieing;
  struct sockaddr_un sa;
  char dummy;
  fd_set fds;
  struct timeval timeout;

  TESTVARV(argc);
  TESTVARP(argv);

  /* We run as long as possible, to guard the MUD server */
  while (!die)
  {
    /* Make sure we have the latest configuration message loaded */
    if (config_message.where)
      free(config_message.where);

    config_message = load_file("files/config/config.msg");

    /* Check for too many crashes */
    t = time(0);
    if (crashes >= 4 && time_out >= t)
    {
      flog("error", "Crashing lots! - Giving up.");
      exit(-1);
    }
    else if (time_out < t)
    {
      time_out = t + 30;
      crashes = 0;
    }
    crashes++;
    printf("\n\n");
    flog("angel", "IntrepidMUD Guardian Angel bootloader");
    dieing = 0;

    /* Launch the MUD server itself */
    fh = fork();
    switch (fh)
    {
    case 0:
      /* Child process, so create new session and launch MUD server */
      setsid();
      sprintf(mud_name, "-=> %s <=- MUD Server on port",
              get_config_message(config_message, "mud_name"));
      argv[0] = mud_name;
      argv[1] = get_config_message(config_message, "port");
      execvp("bin/" MUD_EXEC, argv);
      error("Failed to exec the MUD server!");
      break;

    case -1:
      /* Oops? */
      error("Failed to fork()");
      break;

    default:
      /* Parent process, so guard the MUD server */
      no_tty = 1;

      /* Create file socket for the MUD server to contact us with */
      unlink(MUD_SOCKET_PATH);
      sock_fd = socket(PF_UNIX, SOCK_STREAM, 0);
      if (sock_fd < 0)
        error("Failed to create alive socket!");

      /* Bind the socket */
      sa.sun_family = AF_UNIX;
      strcpy(sa.sun_path, MUD_SOCKET_PATH);
      if (bind(sock_fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
        error("Failed to bind!");

      /* Listen in on it */
      if (listen(sock_fd, 1) < 0)
        error("Failed to listen!");

      /* Do we have a connection? */
      timeout.tv_sec = 120;
      timeout.tv_usec = 0;
      FD_ZERO(&fds);
      FD_SET(sock_fd, &fds);
      if (select(FD_SETSIZE, &fds, null_ptr, null_ptr, &timeout) <= 0)
      {
        /* Timed out, kill the MUD server (after all, we gave it two
           minutes!) */
        kill(fh, SIGKILL);
        flog("angel", "Killed server before connect");
        waitpid(fh, &status, 0);
      }
      else
      {
        /* Accept the connection from the MUD server */
        length = sizeof(sa);
        alive_fd = accept(sock_fd, (struct sockaddr *)&sa, &length);
        if (alive_fd < 0)
          error("Bad accept!?");
        close(sock_fd);

        /* Wait for as long as the MUD server is up and running */
        while (waitpid(fh, &status, WNOHANG) <= 0)
        {
          /* Listen occasionally to the socket the MUD server is connected
             on */
          timeout.tv_sec = 300;
          timeout.tv_usec = 0;
          FD_ZERO(&fds);
          FD_SET(alive_fd, &fds);
          if (select(FD_SETSIZE, &fds, null_ptr, null_ptr, &timeout) <= 0)
          {
            /* Failure? Check against retry code */
            if (errno != EINTR)
            {
              /* We have a failure */
              if (dieing)
              {
                /* It's already dieing, so send a SIGKILL to it */
                kill(fh, SIGKILL);
                flog("angel", "Server KILLed");
              }
              else
              {
                /* Send SIGTERM to the MUD server */
                kill(fh, SIGTERM);
                flog("angel", "Server TERMinated");
                dieing = 1;
              }
            }
          }
          else
          {
            /* We have a live connection to the MUD server, so try to find
               out how many characters of input we have */
            if (ioctl(alive_fd, FIONREAD, &length) < 0)
              error("Bad FIONREAD");

            /* If we didn't get anything... */
            if (!length)
            {
              /* Send a SIGKILL to the server */
              kill(fh, SIGKILL);
              flog("angel", "Server disconnected");
              dieing = 1;
            }
            else
            {
              /* Read each character off the socket */
              for (; length; length--)
              {
                read(alive_fd, &dummy, 1);
              }
            }
          }
        }
      }

      /* Ok, we're closing down the MUD server */
      close(alive_fd);
      switch ((status & 255))
      {
      case 0:
        flog("angel", "Server exited safely");
        break;

      case 127:
        vflog("angel", "Server stopped due to signal %d",
              (status >> 8) & 255);
        break;

      default:
        vflog("angel", "Server terminated due to signal %d", status & 127);
        if (status & 128)
          flog("angel", "Core dump produced - oops!?");
        break;
      }
      break;
    }
  }

  return 0;
}
