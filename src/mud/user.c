/*
 * IntrepidMUD
 * MUD Server
 * The main MUD server itself
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
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/proto.h"

#ifdef NEED_CRYPT_DECL
extern char *crypt(const char *, const char *);
#endif

int32 max_users, current_users;

user *create_user(void)
{
  user *u;

  u = (user *) malloc(sizeof(user));
  memset(u, null, sizeof(user));

  if (users_list)
    users_list->previous = u;
  u->next = users_list;
  users_list = u;

  u->fn_timer = null_ptr;
  u->count_timer = 0;
  u->fn_input = null_ptr;
  u->input_flags = 0;
  u->conn_flags = 0;
  u->conn_flags |= CONN_IAC_GA;
  u->term_width = 0;
  u->default_term_width = DEFAULT_TERM_WIDTH;
  u->detected_term_width = 0;
  u->term_type = TERM_NONE;
  u->default_term_type = TERM_NONE;
  u->detected_term_type = 0;
  u->logged_in = 0;
  u->term_lines = 0;
  u->default_term_lines = DEFAULT_TERM_LINES;
  u->detected_term_lines = 0;

  memset(u->temp_prompt, null_chr, MAX_PROMPT);
  snprintf(u->temp_prompt, MAX_PROMPT - 1, "%s",
           get_config_message(prompts_message, "default_prompt"));

  return u;
}

void quit_user(user * u)
{
  if (u->logged_in)
    save_user(u);

  if (u->fd > -1)
  {
    shutdown(u->fd, 2);
    close(u->fd);
  }

  current_users--;

  if (u->previous)
    u->previous->next = u->next;
  else
    users_list = u->next;
  if (u->next)
    u->next->previous = u->previous;

  free(u);
}

int32 user_hardcoded(user * u)
{
  char buf[512], *ptr, *start;

  strncpy(buf, get_config_message(config_message, "hardcoded"), 511);

  for (ptr = start = buf; *ptr; ptr++)
  {
    if (*ptr == ' ')
    {
      *ptr++ = null_chr;
      if (!strcasecmp(u->username, start))
        return 1;

      start = ptr;
    }
  }

  if (!strcasecmp(u->username, start))
    return 1;

  return 0;
}

void pwd_timeout(user * u)
{
  unsigned char *oldstack = stack;

  stack = (unsigned char *)add_line(u, (char *)stack, add_string);
  stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                     get_config_message(messages_message,
                                                        "too_long_time"));
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void password(user * u, char *str)
{
  unsigned char *oldstack = stack;

  if (strlen(str) > 0)
  {
    tell_user(u, "You need to have an empty string as parameter.\n");
    return;
  }

  snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
           get_config_message(prompts_message, "enter_pwd"));
  u->fn_timer = pwd_timeout;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_input = got_curr_password;

  u->failed_password = 0;

  stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                         "Please enter your password");
  stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                     get_config_message(messages_message,
                                                        "enter_pwd"));
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);

  password_mode_on(u);

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void got_curr_password(user * u, char *str)
{
  unsigned char *oldstack = stack;
  char *passwd;
  char username[MAX_USER_NAME];
  char *tmp = username;

  strcpy(username, u->username);
  while (tmp && *tmp)
  {
    *tmp = tolower(*tmp);
    tmp++;
  }

  memset(u->special_prompt, null_chr, MAX_PROMPT);

  passwd = crypt(str, username);
  if (strncmp(passwd, u->password, MAX_PASSWORD - 1)
      && u->failed_password < 2)
  {
    u->failed_password++;

    snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
             get_config_message(prompts_message, "enter_pwd"));
    u->fn_timer = pwd_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_curr_password;

    stack =
      (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                     "Password match failed");
    stack =
      (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                 get_config_message(messages_message,
                                                    "enter_pwd"));
    stack = add_string(stack);
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);

    tell_user(u, (char *)oldstack);
    stack = oldstack;

    return;
  }

  password_mode_off(u);

  if (strncmp(passwd, u->password, MAX_PASSWORD - 1))
  {
    stack =
      (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                     "Password match failed");
    stack =
      (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                 get_config_message(messages_message,
                                                    "failed_pwd"));
    stack = add_string(stack);
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);

    tell_user(u, (char *)oldstack);
    stack = oldstack;

    return;
  }

  stack =
    (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                   "Please select a password");
  stack =
    (unsigned char *)strprintf((char *)stack, add_string, "%s",
                               get_config_message(messages_message,
                                                  "enter_pwd"));
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);

  snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
           get_config_message(prompts_message, "enter_pwd"));
  u->fn_timer = pwd_timeout;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_input = got_new_password_change_1;

  password_mode_on(u);

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void got_new_password_change_1(user * u, char *str)
{
  unsigned char *oldstack = stack;
  char *passwd;
  char username[MAX_USER_NAME];
  char *tmp = username;

  strcpy(username, u->username);
  while (tmp && *tmp)
  {
    *tmp = tolower(*tmp);
    tmp++;
  }

  passwd = crypt(str, username);

  memset(u->new_password, null_chr, MAX_PASSWORD);
  memset(u->special_prompt, null_chr, MAX_PROMPT);

  if (strlen(str) < 5)
  {
    tell_user(u,
              "The minimum length for a password is four characters. Please try again.\n");
    snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
             get_config_message(prompts_message, "enter_pwd"));

    stack =
      (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                     "Please enter a password");
    stack =
      (unsigned char *)strprintf((char *)stack, add_string,
                                 get_config_message(messages_message,
                                                    "enter_pwd"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);

    u->fn_timer = pwd_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_new_password_change_1;

    tell_user(u, (char *)oldstack);
    stack = oldstack;

    return;
  }

  snprintf(u->new_password, MAX_PASSWORD - 1, "%s", passwd);
  snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
           get_config_message(prompts_message, "enter_pwd"));
  u->fn_timer = pwd_timeout;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_input = got_new_password_change_2;

  stack =
    (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                   "Please retype your new password");
  stack =
    (unsigned char *)strprintf((char *)stack, add_string, "%s",
                               get_config_message(messages_message,
                                                  "re_pwd"));
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void got_new_password_change_2(user * u, char *str)
{
  unsigned char *oldstack = stack;
  char *passwd;
  char username[MAX_USER_NAME], *tmp = username;

  strcpy(username, u->username);
  while (tmp && *tmp)
  {
    *tmp = tolower(*tmp);
    tmp++;
  }

  memset(u->special_prompt, null_chr, MAX_PROMPT);

  passwd = crypt(str, username);
  if (strncmp(passwd, u->new_password, MAX_PASSWORD - 1))
  {
    stack =
      (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                     "Please select a password");
    stack =
      (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                 get_config_message(messages_message,
                                                    "enter_pwd"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);

    snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
             get_config_message(prompts_message, "enter_pwd"));
    u->fn_timer = pwd_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_new_password_change_1;

    tell_user(u, (char *)oldstack);
    stack = oldstack;

    return;
  }

  password_mode_off(u);
  memset(u->password, null_chr, MAX_PASSWORD);
  memcpy(u->password, u->new_password, MAX_PASSWORD - 1);
  memset(u->new_password, null_chr, MAX_PASSWORD);

  tell_user(u, "You changed your password!\n");
}
