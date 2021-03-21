/*
 * IntrepidMUD
 * Archangel
 * Guards the guardian angel, rebooting it when necessary
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
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include "../include/proto.h"
#include "../include/un.h"

extern int nice(int);

unsigned char *stack_start, *stack;
char archangel_name[256], angel_name[256];
int32 fh = 0, die = 0, crashes = 0, syncing = 0, no_tty = 0;
int32 time_out = 0, t = 0;

int32 main(int32 argc, char *argv[])
{
  FILE *archangel_pid_fd;
  struct sigaction siga;

  TESTVARP(argv);
  TESTVARV(argc);

  /* Create the stack for the Archangel */
  stack_start = create_stack(5000);
  stack = stack_start;

  /* Initialize the framework for us */
  if (init_framework(argc, argv))
  {
    fprintf(stderr, "\n\nFailed to initialize framework!\n\n");
    exit(-1);
  }

  /* Create name of Archangel */
  sprintf(archangel_name,
          "-=> %s <=- Archangel watching the Angel on port %d",
          get_config_message(config_message, "mud_name"),
          atoi(get_config_message(config_message, "port")));


  /* Create file containing process id, removing any previous one */
  unlink(ARCHANGEL_PID);
  if (!(archangel_pid_fd = fopen(ARCHANGEL_PID, "w")))
  {
    fprintf(stderr, "Unable to create pid log file!\n");
    exit(1);
  }
  fprintf(archangel_pid_fd, "%d\n", (int32) getpid());
  fclose(archangel_pid_fd);

  /* Are we already running with corrected name? */
  if (strcmp(archangel_name, argv[0]))
  {
    /* No, start us again with the name set to the nice one */
    argv[0] = archangel_name;
    execvp("bin/" ARCHANGEL_EXEC, argv);
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

  /* And start the Archangel itself */
  return handle_angel(argc, argv);
}

int32 handle_angel(int32 argc, char *argv[])
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

  /* So we run as long as possible, to guard the Angel */
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
    flog("archangel", "IntrepidMUD Archangel bootloader");
    dieing = 0;

    /* Launch the Angel itself */
    fh = fork();
    switch (fh)
    {
    case 0:
      /* Child process, so create new session and launch the Angel */
      setsid();
      sprintf(angel_name, "-=> %s <=- Guardian Angel watching port %d",
              get_config_message(config_message, "mud_name"),
              atoi(get_config_message(config_message, "port")));
      argv[0] = angel_name;
      execvp("bin/" ANGEL_EXEC, argv);
      error("Failed to exec the Guardian Angel!");
      break;

    case -1:
      /* Oops? */
      error("Failed to fork()");
      break;

    default:
      /* Parent process, so guard the Guardian Angel */
      no_tty = 1;

      /* Create file socket for the Angel to contact us with */
      unlink(ANGEL_SOCKET_PATH);
      sock_fd = socket(PF_UNIX, SOCK_STREAM, 0);
      if (sock_fd < 0)
        error("Failed to create alive socket!");

      /* Bind the socket */
      sa.sun_family = AF_UNIX;
      strcpy(sa.sun_path, ANGEL_SOCKET_PATH);
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
        /* Timed out, kill the Angel (after all, we gave it two
           minutes!) */
        kill(fh, SIGKILL);
        flog("archangel", "Killed angel before connect");
        waitpid(fh, &status, 0);
      }
      else
      {
        /* Accept the connection from the Angel */
        length = sizeof(sa);
        alive_fd = accept(sock_fd, (struct sockaddr *)&sa, &length);
        if (alive_fd < 0)
          error("Bad accept!?");
        close(sock_fd);

        /* Wait for as long as the Angel is up and running */
        while (waitpid(fh, &status, WNOHANG) <= 0)
        {
          /* Listen occasionally to the socket the Angel is connected on */
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
                flog("archangel", "Angel KILLed");
              }
              else
              {
                /* Send SIGTERM to the Angel */
                kill(fh, SIGTERM);
                flog("archangel", "Angel TERMinated");
                dieing = 1;
              }
            }
          }
          else
          {
            /* We have a live connection to the Angel, so try to find out
               how many characters of input we have */
            if (ioctl(alive_fd, FIONREAD, &length) < 0)
              error("Bad FIONREAD");

            /* If we didn't get anything... */
            if (!length)
            {
              /* Send a SIGKILL to the Angel */
              kill(fh, SIGKILL);
              flog("archangel", "Angel disconnected");
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

      /* Ok, we're closing down the Angel */
      close(alive_fd);
      switch (status & 255)
      {
      case 0:
        flog("archangel", "Angel exited safely");
        break;

      case 127:
        vflog("archangel", "Angel stopped due to signal %d",
              (status >> 8) & 255);
        break;

      default:
        vflog("archangel", "Angel terminated due to signal %d",
              status & 127);
        if (status & 128)
          flog("archangel", "Core dump produced - oops!?");
        break;
      }
      break;
    }
  }

  return 0;
}
