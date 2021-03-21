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
#include <strings.h>
#include <arpa/telnet.h>
#include <unistd.h>
#include <sys/select.h>
#include <linux/errno.h>

#include "../include/proto.h"
#include "../include/user.h"
#include "../include/un.h"

#ifdef NEED_GETHOSTNAME_DECL
extern int gethostname(char *, size_t);
extern int sethostname(const char *, size_t);
#endif                          /* NEED_GETHOSTNAME_DECL */

int32 intermud_port;
int32 alive_descriptor = -1, main_descriptor = -1, dns_descriptor = -1,
  ident_descriptor = -1, intermud_descriptor = -1;
terminal terms[] = {
  {"adm", TERM_ADM, 0},
  {"ansi", TERM_ANSI, 0},
  {"cygwin", TERM_ANSI, 1},     /* alias for "ansi" */
  {"hp2392", TERM_HP2392, 0},
  {"linux", TERM_ANSI, 1},      /* alias for "ansi" */
  {"sun", TERM_SUN, 0},
  {"tvi912", TERM_TVI912, 0},
  {"vt100", TERM_VT100, 0},
  {"vt102", TERM_VT102, 0},
  {"vt220", TERM_VT220, 0},
  {"wyse30", TERM_WYSE30, 0},
  {"xterm", TERM_XTERM, 0},
  {"unknown", TERM_NONE, 1},    /* alias for "none" */
  {null_ptr, TERM_NONE, 0}
};

void accept_new_connection(void)
{
  struct sockaddr_in incoming;
  struct hostent *hp;
#ifdef HAVE_SOCKLEN_T
  socklen_t length;
#else
  uint32 length;
#endif
  int32 new_socket;
  user *u;
  int32 dummy = 1, no1, no2, no3, no4;
#ifdef LOLIGO
  char *numerical_address;
#endif

  length = sizeof(incoming);
  new_socket =
    accept(main_descriptor, (struct sockaddr *)&incoming, &length);
  if ((new_socket < 0) && errno == EINTR)
  {
    flog("error", "EINTR accept trap");
    return;
  }
  if (new_socket < 0)
  {
    flog("error", "Error accepting new connection.");
    return;
  }

  if (ioctl(new_socket, FIONBIO, &dummy) < 0)
    handle_error("Can't set non-blocking");

  if (current_users >= max_users)
  {
    char *full = get_config_message(messages_message, "full");

    write(new_socket, full, strlen(full));
    return;
  }

  u = create_user();
  current_user = u;
  u->fd = new_socket;

  strncpy(u->addr_num, inet_ntoa(incoming.sin_addr), MAX_INET_ADDR - 2);
#ifdef LOLIGO
  numerical_address = u->addr_num;
#else
  hp =
    gethostbyaddr((char *)&(incoming.sin_addr.s_addr),
                  sizeof(incoming.sin_addr.s_addr), AF_INET);
  if (hp)
    strncpy(u->addr_inet, hp->h_name, MAX_INET_ADDR - 2);
  else
    strncpy(u->addr_inet, u->addr_num, MAX_INET_ADDR - 2);
#endif
  sscanf(u->addr_num, "%d.%d.%d.%d", &no1, &no2, &no3, &no4);

  vtell_user(u, "%c%c%c", IAC, DO, TELOPT_TTYPE);
  vtell_user(u, "%c%c%c", IAC, DO, TELOPT_NAWS);

  connect_to_mud(u);
  current_user = null_ptr;
}

void alive_connect(void)
{
  struct sockaddr_un sa;

  alive_descriptor = socket(PF_UNIX, SOCK_STREAM, 0);
  if (alive_descriptor < 0)
    handle_error("Failed to make alive socket");

  sa.sun_family = AF_UNIX;
  strcpy(sa.sun_path, MUD_SOCKET_PATH);

  if (connect(alive_descriptor, (struct sockaddr *)&sa, sizeof(sa)) < 0)
  {
    close(alive_descriptor);
    alive_descriptor = -1;
    flog("boot", "Failed to connect to Angel, we're on our own.");
    return;
  }
  do_alive_ping();

  flog("boot", "Alive and kicking");
}

void do_alive_ping(void)
{
  static int32 count = 5;

  count--;
  if (!count && alive_descriptor > 0)
  {
    count = 5;
    if (write(alive_descriptor, "SpAng!", 6) < 0)
    {
      if (errno != EPIPE)
        return;

      flog("sigpipe", "Angel pipe closed");
      close(alive_descriptor);
      alive_descriptor = -1;
    }
  }
}

void get_user_input(user * u)
{
  int32 chars_ready = 0;
  char c;

  if (ioctl(u->fd, FIONREAD, &chars_ready) == -1)
  {
    quit_user(u);
    flog("error", "PANIC on FIONREAD ioctl");
    return;
  }

  if (!chars_ready)
  {
    if (u->username[0])
      vflog("connection", "%s went netdead.", u->username);
    else
      flog("connection", "Connection went netdead on login");

    quit_user(u);
    return;
  }

  for (; chars_ready; chars_ready--)
  {
    if (read(u->fd, &c, 1) != 1)
    {
      flog("connection", "Caught read error on socket.");
      return;
    }

    switch (c)
    {
    case -1:
      u->input_flags &= ~(INPUT_LAST_R | INPUT_LAST_N);
      telnet_options(u);
      return;
      break;

    case '\n':
      if (!(u->input_flags & INPUT_LAST_R))
      {
        u->input_flags |= INPUT_LAST_N;
        u->input_flags |= INPUT_READY;
        u->input_buffer[u->input_pointer] = null_chr;
        u->input_pointer = 0;
        return;
      }
      break;

    case '\r':
      if (!(u->input_flags & INPUT_LAST_N))
      {
        u->input_flags |= INPUT_LAST_R;
        u->input_flags |= INPUT_READY;
        u->input_buffer[u->input_pointer] = null_chr;
        u->input_pointer = 0;
        return;
      }
      break;

    default:
      u->input_flags &= ~(INPUT_LAST_R | INPUT_LAST_N);
      if (c == 8 || c == 127 || c == -9)
      {
        backspace(u);
        return;
      }
      if ((c > 31) && (u->input_pointer < (MAX_INPUT_BUFFER - 3)))
      {
        u->input_buffer[u->input_pointer++] = c;

        if ((!(u->conn_flags & CONN_PASSWORD_MODE))
            && (u->conn_flags & CONN_DO_LOCAL_ECHO))
        {
          if (write(u->fd, &c, 1) < 0 && errno != EINTR)
          {
            flog("error", "Echoing back to player.");
            quit_user(u);
            return;
          }
        }
      }
      else if (!(u->input_pointer < (MAX_INPUT_BUFFER - 3)))
      {
        return;
      }
      break;
    }
  }
}

void init_socket(int32 port)
{
  struct sockaddr_in main_socket;
  int32 dummy = 1;
  char *hostname;
  struct hostent *hp;

  hostname = (char *)malloc(101);
  memset(hostname, null_chr, 101);
  memset(&main_socket, null, sizeof(struct sockaddr_in));
  gethostname(hostname, 100);

  hp = gethostbyname(hostname);
  if (!hp)
    handle_error("Error: Host machine does not exist!");

  main_socket.sin_family = hp->h_addrtype;
  main_socket.sin_port = htons(port);

  main_descriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (main_descriptor < 0)
  {
    flog("boot", "Couldn't grab the socket!");
    exit(-1);
  }

  if (setsockopt(main_descriptor, SOL_SOCKET, SO_REUSEADDR, (char *)&dummy,
                 sizeof(dummy)) < 0)
    handle_error("Couldn't setsockopt()");

  if (ioctl(main_descriptor, FIONBIO, &dummy) < 0)
    handle_error("Can't set non-blocking");

  if (bind(main_descriptor, (struct sockaddr *)&main_socket,
           sizeof(main_socket)) < 0)
  {
    flog("boot",
         "Couldn't bind socket - something is on that port number!");
    exit(-2);
  }

  if (listen(main_descriptor, 5) < 0)
    handle_error("Listen refused");

  vflog("boot", "Main socket bound and listening on port %d", port);
}

int32 scan_sockets(void)
{
  fd_set fset;
  user *scan;
  int32 ret = 0;

  FD_ZERO(&fset);

  FD_SET(main_descriptor, &fset);
  for (scan = users_list; scan; scan = scan->next)
  {
    if (scan->fd >= 0)
      FD_SET(scan->fd, &fset);
  }

  if (dns_descriptor >= 0)
    FD_SET(dns_descriptor, &fset);
  if (ident_descriptor >= 0)
    FD_SET(ident_descriptor, &fset);
  if (intermud_descriptor >= 0)
    FD_SET(intermud_descriptor, &fset);

  ret = select(FD_SETSIZE, &fset, null_ptr, null_ptr, null_ptr);
  if (ret > 0)
  {
    if (FD_ISSET(main_descriptor, &fset))
      accept_new_connection();

    for (scan = users_list; scan; scan = scan->next)
    {
      if (scan->fd >= 0 && FD_ISSET(scan->fd, &fset))
        get_user_input(scan);
    }
  }

  return ret;
}

void telnet_sb_telopt_ttype(user * u)
{
  unsigned char c;
  terminal *scan = null_ptr;
  unsigned char *oldstack = stack;
  char *str;

  while ((read(u->fd, &c, 1) == 1))
  {
    if (IAC == c)
      break;
    else
      *stack++ = c;
  }
  *stack++ = null_chr;
  str = (char *)oldstack;

  for (scan = terms; scan->name != null_ptr; scan++)
  {
    if (!strcasecmp(str, scan->name))
    {
      u->term_type = scan->type;
      u->detected_term_type = 1;
      break;
    }
  }

  /* if we didn't match a terminal type, try again */
  if (!u->detected_term_type)
  {
    if (strncasecmp(str, u->prev_ttype, MAX_TTYPE - 1))
      vtell_user(u, "%c%c%c%c%c%c", IAC, SB, TELOPT_TTYPE, TELQUAL_SEND,
                 IAC, SE);
    snprintf(u->prev_ttype, MAX_TTYPE - 1, "%s", str);
  }

  stack = oldstack;

  if (read(u->fd, &c, 1) != 1)  /* gobble SE */
    return;
}

void telnet_sb_telopt_naws(user * u)
{
  unsigned char c;
  int32 lines, width, temp[2], i;

  /* read width: note, if we get IAC we need to read again */
  for (i = 0; i < 2; i++)
  {
    if (read(u->fd, &c, 1) != 1)
      return;
    if (c == IAC)
      if (read(u->fd, &c, 1) != 1)
        return;
    temp[i] = c;
  }
  width = temp[0] * 256 + temp[1];

  /* read lines: note, if we get IAC we need to read again */
  for (i = 0; i < 2; i++)
  {
    if (read(u->fd, &c, 1) != 1)
      return;
    if (c == IAC)
      if (read(u->fd, &c, 1) != 1)
        return;
    temp[i] = c;
  }
  lines = temp[0] * 256 + temp[1];

  /* gobble up the IAC SE at the end */
  if (read(u->fd, &c, 1) != 1)
    return;
  if (read(u->fd, &c, 1) != 1)
    return;

  /* set the size */
  if (!width)
    u->term_width = u->default_term_width;
  else
  {
    u->term_width = width - 1;
    u->detected_term_width = 1;
  }

  if (!lines)
    u->term_lines = u->default_term_lines;
  else
  {
    u->term_lines = lines - 1;
    u->detected_term_lines = 1;
  }

  /* sanity check */
  if (u->term_width < 1)
  {
    u->term_width = u->default_term_width;
    u->detected_term_width = 0;
  }
  if (u->term_lines < 1)
  {
    u->term_lines = u->default_term_lines;
    u->detected_term_lines = 0;
  }
}

void telnet_options(user * u)
{
  unsigned char c;

  if (read(u->fd, &c, 1) != 1)
    return;

  switch (c)
  {
  case SB:
    if (read(u->fd, &c, 1) != 1)
      return;
    switch (c)
    {
    case TELOPT_TTYPE:
      if (read(u->fd, &c, 1) != 1)
        return;
      if (TELQUAL_IS == c)
      {
        telnet_sb_telopt_ttype(u);
      }
      break;

    case TELOPT_NAWS:
      telnet_sb_telopt_naws(u);
      break;
    }
    break;

  case WILL:
    if (read(u->fd, &c, 1) != 1)
      return;

    switch (c)
    {
    case TELOPT_TTYPE:
      vtell_user(u, "%c%c%c%c%c%c", IAC, SB, TELOPT_TTYPE,
                 TELQUAL_SEND, IAC, SE);
      break;

    case TELOPT_NAWS:
      break;
    }
    break;

  case WONT:
    if (read(u->fd, &c, 1) != 1)
      return;

    switch (c)
    {
    case TELOPT_STATUS:
      break;

    case TELOPT_NAWS:
      break;
    }
    break;

  case EC:
    backspace(u);
    break;

  case EL:
    u->input_pointer = 0;
    break;

  case IP:
    quit_user(u);
    break;

  case DO:
    if (read(u->fd, &c, 1) != 1)
      return;

    switch (c)
    {
    case TELOPT_ECHO:
      u->conn_flags |= CONN_DO_LOCAL_ECHO;
      break;

    case TELOPT_SGA:
      break;

    case TELOPT_EOR:
      u->conn_flags |= CONN_IAC_EOR;
      u->conn_flags &= ~CONN_IAC_GA;
      vtell_user(u, "%c%c%c", IAC, WILL, TELOPT_EOR);
      break;
    }
    break;

  case DONT:
    if (read(u->fd, &c, 1) != 1)
      return;

    switch (c)
    {
    case TELOPT_ECHO:
      u->conn_flags &= ~CONN_DO_LOCAL_ECHO;
      break;

    case TELOPT_SGA:
      break;

    case TELOPT_EOR:
      u->conn_flags &= ~CONN_IAC_EOR;
      if (u->conn_flags & CONN_DEFAULT_IAC_GA)
        u->conn_flags |= CONN_IAC_GA;
      break;
    }
    break;
  }
}

void password_mode_on(user * u)
{
  u->conn_flags |= CONN_PASSWORD_MODE;

  vtell_user(u, "%c%c%c", IAC, WILL, TELOPT_ECHO);
}

void password_mode_off(user * u)
{
  u->conn_flags &= ~CONN_PASSWORD_MODE;

  vtell_user(u, "%c%c%c", IAC, WONT, TELOPT_ECHO);
}
