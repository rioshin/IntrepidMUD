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
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <arpa/telnet.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/proto.h"
#include "../include/user.h"
#include "../include/character.h"
#include "../include/clist.h"

#ifdef NEED_CRYPT_DECL
extern char *crypt(const char *, const char *);
#endif

#ifdef NEED_SCANDIR_DECL
extern int scandir(DIRENT_PROTO char *__restrict,
                   struct dirent ***__restrict,
                   int (*)(DIRENT_PROTO struct dirent *),
                   int (*)(DIRENT_PROTO struct dirent **,
                           DIRENT_PROTO struct dirent **));
extern int alphasort(DIRENT_PROTO struct dirent **,
                     DIRENT_PROTO struct dirent **);
#endif                          /* NEED_SCANDIR_DECL */

typedef struct file_list
{
  file f;
  struct file_list *next;
} file_list;

file_list *welcome_list = null_ptr;

int32 welcome_count = 0;

int32 valid_message(DIRENT_PROTO struct dirent *d)
{
  char *dotter;

  if (!(dotter = strstr(d->d_name, ".msg")) || strlen(dotter) != 4)
    return 0;

  return 1;
}

void init_welcome_list(void)
{
  struct dirent **de;
  struct stat sbuf;
  char path[320];
  unsigned char *oldstack = stack;
  int32 dc = 0, i;
  file lf;
  file_list *cur;

  memset(path, null_chr, 320);

  dc = scandir("files/welcome", &de, valid_message, alphasort);
  if (dc < 1)
    return;

  for (i = 0; i < dc; i++)
  {
    sprintf(path, "files/welcome/%s", de[i]->d_name);
    free(de[i]);

    if (stat(path, &sbuf) < 0)
      continue;

    lf = load_file(path);
    cur = (file_list *) malloc(sizeof(file_list));

    cur->f = lf;
    cur->next = welcome_list;
    welcome_list = cur;
    welcome_count++;
  }
  free(de);

  stack = oldstack;
}

void connect_to_mud(user * u)
{
  user *old_current = current_user;
  unsigned char *oldstack = stack;
  current_user = u;

  memset(u->special_prompt, null_chr, MAX_PROMPT);
  snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
           get_config_message(prompts_message, "press_enter"));
  u->fn_input = first_enter;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_timer = login_timeout;

  send_random_connect_screen(u);

  current_user = old_current;
  stack = oldstack;
}

void send_random_connect_screen(user * u)
{
  int32 screen = 0;
  file_list *scan = null_ptr;
  unsigned char *oldstack = stack;

  if (!welcome_list)
    init_welcome_list();

  if (!welcome_list)
    return;

  if (welcome_count > 1)
    screen = rand() % welcome_count;

  for (scan = welcome_list; screen; screen--, scan = scan->next)
    ;

  tell_user(u, scan->f.where);
  stack = oldstack;
}

void login_timeout(user * u)
{
  unsigned char *oldstack = stack;

  stack = (unsigned char *)add_line(u, (char *)stack, add_string);
  stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                     get_config_message(messages_message,
                                                        "too_long_time"));
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);

  tell_user(u, (char *)oldstack);
  stack = oldstack;

  quit_user(u);
}

void first_enter(user * u, char *str)
{
  unsigned char *oldstack = stack;

  TESTVARP(str);
  stack =
    (unsigned char *)vadd_line_text(u, (char *)stack, add_string,
                                    "Welcome to %s",
                                    get_config_message(config_message,
                                                       "mud_name"));
  stack =
    (unsigned char *)strprintf((char *)stack, add_string, "%s",
                               get_config_message(messages_message,
                                                  "login_instr"));
  stack =
    (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                   "Message of the Day");
  stack =
    (unsigned char *)strprintf((char *)stack, add_string, "%s",
                               get_config_message(messages_message,
                                                  "motd"));
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);

  memset(u->special_prompt, null_chr, MAX_PROMPT);
  snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
           get_config_message(prompts_message, "username"));
  u->fn_timer = login_timeout;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_input = got_login_name;

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void got_login_name(user * u, char *str)
{
  unsigned char *oldstack = stack;
  char buf[MAX_USER_NAME];

  if (!str || !*str)
  {
    first_enter(u, str);
    return;
  }

  if (!strcasecmp(str, "new"))
  {
    new_user_start(u, str);
    return;
  }

  if (!strcasecmp(str, "quit"))
  {
    quit_user(u);
    return;
  }

  if (!strcasecmp(str, "who"))
  {
    who_is_on(u, "");
    tell_user(u, "\n");

    stack =
      (unsigned char *)vadd_line_text(u, (char *)stack, add_string,
                                      "Please give a username");
    stack =
      (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                 get_config_message(messages_message,
                                                    "login_instr"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);

    memset(u->special_prompt, null_chr, MAX_PROMPT);
    snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
             get_config_message(prompts_message, "username"));
    u->fn_timer = login_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_login_name;

    tell_user(u, (char *)oldstack);
    stack = oldstack;

    return;
  }

  if (strlen(str) > (MAX_USER_NAME - 1))
  {
    stack =
      (unsigned char *)vadd_line_text(u, (char *)stack, add_string,
                                      "User name too long");
    stack =
      (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                 get_config_message(messages_message,
                                                    "login_instr"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);

    memset(u->special_prompt, null_chr, MAX_PROMPT);
    snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
             get_config_message(prompts_message, "username"));
    u->fn_timer = login_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_login_name;

    tell_user(u, (char *)oldstack);
    stack = oldstack;

    return;
  }

  memset(u->special_prompt, null_chr, MAX_PROMPT);
  memset(buf, null_chr, MAX_USER_NAME);
  snprintf(buf, MAX_USER_NAME - 1, "%s", str);

  if (!user_exists_name(buf))
  {
    stack =
      (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                     "No such user");
    stack =
      (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                 get_config_message(messages_message,
                                                    "login_instr"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);

    snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
             get_config_message(prompts_message, "username"));
    u->fn_timer = login_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_login_name;

    tell_user(u, (char *)oldstack);
    stack = oldstack;

    return;
  }
  memset(u->username, null_chr, MAX_USER_NAME);
  sprintf(u->username, "%s", buf);

  load_user(u);
  if (u->term_type == TERM_NONE)
    u->term_type = u->default_term_type;
  if (u->term_lines == 0)
    u->term_lines = u->default_term_lines;
  if (u->term_width == 0)
    u->term_width = u->default_term_width;

  u->temp_term_type = u->term_type;
  u->temp_term_lines = u->term_lines;
  u->temp_term_width = u->term_width;
  u->term_type = TERM_NONE;

  strcpy(u->temp_prompt, u->normal_prompt);
  memset(u->normal_prompt, null_chr, MAX_PROMPT);
  memset(u->special_prompt, null_chr, MAX_PROMPT);

  password_mode_on(u);

  snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
           get_config_message(prompts_message, "enter_pwd"));
  u->fn_timer = login_timeout;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_input = got_password;

  u->failed_password = 0;

  stack =
    (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                   "Please enter your password");
  stack =
    (unsigned char *)strprintf((char *)stack, add_string, "%s",
                               get_config_message(messages_message,
                                                  "enter_pwd"));
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void got_password(user * u, char *str)
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
    u->fn_timer = login_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_password;

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

    quit_user(u);

    return;
  }

  u->term_type = u->temp_term_type;
  u->term_lines = u->temp_term_lines;
  u->term_width = u->temp_term_width;

  pick_character(u);

  stack = oldstack;
}

void pick_character(user * u)
{
  char *charop;
  unsigned char *oldstack = stack;

  stack =
    (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                   "Select character");
  stack =
    (unsigned char *)strprintf((char *)stack, add_string, "%s",
                               get_config_message(messages_message,
                                                  "character"));
  stack = (unsigned char *)add_line(u, (char *)stack, add_string);
  charop = list_character_options(u, "");
  stack =
    (unsigned char *)strprintf((char *)stack, add_string, "%s", charop);
  free(charop);
  charop = null_ptr;
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);
  tell_user(u, (char *)oldstack);

  u->fn_timer = login_timeout;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_input = selected_character;
  stack = oldstack;
}

void new_user_start(user * u, char *str)
{
  unsigned char *oldstack = stack;

  TESTVARP(str);
  stack =
    (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                   "Select username");
  stack =
    (unsigned char *)strprintf((char *)stack, add_string, "%s",
                               get_config_message(messages_message,
                                                  "new_name"));
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);

  memset(u->special_prompt, null_chr, MAX_PROMPT);
  snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
           get_config_message(prompts_message, "new_name"));
  u->fn_timer = login_timeout;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_input = got_new_name;

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void got_new_name(user * u, char *str)
{
  unsigned char *oldstack = stack;
  char buf[MAX_USER_NAME], *scan;
  int32 check = 0;

  memset(u->special_prompt, null_chr, MAX_PROMPT);
  memset(buf, null_chr, MAX_USER_NAME);
  snprintf(buf, MAX_USER_NAME - 1, "%s", str);

  if (strlen(str) > (MAX_USER_NAME - 1))
  {
    stack =
      (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                     "User name too long");
    stack =
      (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                 get_config_message(messages_message,
                                                    "new_name"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);

    snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
             get_config_message(prompts_message, "username"));
    u->fn_timer = login_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_login_name;

    tell_user(u, (char *)oldstack);
    stack = oldstack;

    return;
  }

  scan = str;
  while (*scan)
  {
    if (!isalpha(*scan))
      check++;
    scan++;
  }

  if (check > 0)
  {
    stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                           "User name may only contain letters");
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       get_config_message(messages_message,
                                                          "new_name"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);

    snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
             get_config_message(prompts_message, "username"));
    u->fn_timer = login_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_login_name;

    tell_user(u, (char *)oldstack);
    stack = oldstack;

    return;
  }

  if (user_exists_name(buf))
  {
    stack =
      (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                     "Username exists");
    stack =
      (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                 get_config_message(messages_message,
                                                    "new_name"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);

    snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
             get_config_message(prompts_message, "new_name"));
    u->fn_timer = login_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_new_name;

    tell_user(u, (char *)oldstack);
    stack = oldstack;

    return;
  }

  memset(u->username, null_chr, MAX_USER_NAME);
  snprintf(u->username, MAX_USER_NAME - 1, "%s", str);

  if (user_hardcoded(u))
  {
    stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                           "Hard-coded administrator");
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       get_config_message(messages_message,
                                                          "hardcoded"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);

    snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
             get_config_message(prompts_message, "enter_pwd"));
    u->fn_timer = login_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_hc_pass;

    u->failed_password = 0;

    password_mode_on(u);

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
  u->fn_timer = login_timeout;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_input = got_new_password_1;

  password_mode_on(u);

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void got_hc_pass(user * u, char *str)
{
  unsigned char *oldstack = stack;

  if (strcmp(str, get_config_message(config_message, "hc_passwd"))
      && u->failed_password < 2)
  {
    stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                           "Hard-coded password failed");
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       get_config_message(messages_message,
                                                          "hardcoded"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);

    snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
             get_config_message(prompts_message, "enter_pwd"));
    u->fn_timer = login_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_hc_pass;

    u->failed_password++;

    tell_user(u, (char *)oldstack);
    stack = oldstack;

    return;
  }
  else if (strcmp(str, get_config_message(config_message, "hc_passwd")))
  {
    stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                           "Hard-coded password failed");
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       get_config_message(messages_message,
                                                          "failed_pwd"));
    stack = (unsigned char *)add_string(stack);
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);

    tell_user(u, (char *)oldstack);
    stack = oldstack;

    quit_user(u);

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
  u->fn_timer = login_timeout;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_input = got_new_password_1;

  u->residency |= (HCADMIN | ADMIN | LOWER_ADMIN | ADVANCED_SU | SU);

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void got_new_password_1(user * u, char *str)
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

  memset(u->password, null_chr, MAX_PASSWORD);
  memset(u->special_prompt, null_chr, MAX_PROMPT);

  passwd = crypt(str, username);
  snprintf(u->password, MAX_PASSWORD - 1, "%s", passwd);

  snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
           get_config_message(prompts_message, "enter_pwd"));
  u->fn_timer = login_timeout;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_input = got_new_password_2;

  stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                         "Please retype your password");
  stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                     get_config_message(messages_message,
                                                        "re_pwd"));
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void got_new_password_2(user * u, char *str)
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
  if (strncmp(passwd, u->password, MAX_PASSWORD - 1))
  {
    stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                           "Please select a password");
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       get_config_message(messages_message,
                                                          "enter_pwd"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);

    snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
             get_config_message(prompts_message, "enter_pwd"));
    u->fn_timer = login_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_new_password_1;

    tell_user(u, (char *)oldstack);
    stack = oldstack;

    return;
  }

  password_mode_off(u);

  if (u->term_type != TERM_NONE)
  {
    pick_character(u);
  }
  else
  {
    snprintf(u->special_prompt, MAX_PROMPT - 1, "%s",
             get_config_message(prompts_message, "want_color"));
    u->fn_timer = login_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = want_color;

    stack =
      (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                     "Do you want color?");
    stack =
      (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                 get_config_message(messages_message,
                                                    "want_color"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);

    tell_user(u, (char *)oldstack);
  }
  stack = oldstack;
}

void want_color(user * u, char *str)
{
  unsigned char *oldstack = stack;

  if (!strcasecmp(str, "y") || !strcasecmp(str, "yes"))
  {
    u->term_type = TERM_VT100;
    u->default_term_type = TERM_VT100;
  }

  pick_character(u);

  stack = oldstack;
}

char *list_character_options(user * u, char *str)
{
  unsigned char *tmp, *oldtmp;
  character *uc;
  int i = 1;

  tmp = malloc(1024);
  if (!tmp)
    return null_ptr;
  oldtmp = tmp;

  TESTVARP(str);

  load_characters(u);
  uc = u->user_chars;

  if (!uc)
  {
    /* has no characters, say so */
    tmp =
      (unsigned char *)strprintf((char *)tmp, add_string, "%s",
                                 "You seem to have no characters so far. "
                                 "Type create to start the process of "
                                 "creating your first one.\n");

    return (char *)oldtmp;
  }

  while (uc)
  {
    tmp =
      (unsigned char *)strprintf((char *)tmp, add_string,
                                 "%s %s (%s %s)\n", uc->firstname,
                                 uc->familyname, uc->race->name,
                                 uc->class->name);
    uc = uc->user_next;
    i++;
  }

  if (i < max_characters(u))
    tmp =
      (unsigned char *)strprintf((char *)tmp, end_string, "\n%s\n",
                                 "Or you may type create to begin the process "
                                 "of creating a new character.");
  else
    tmp = (unsigned char *)strprintf((char *)tmp, end_string, "");

  return (char *)oldtmp;
}

void selected_character(user * u, char *str)
{
  unsigned char *oldstack = stack;

  TESTVARP(str);

  if (!strcasecmp(str, "create"))
    create_character(u);
  else
  {
    u->current_char = load_char(u, str);
    if (u->current_char)
      link_to_program(u);
    else
    {
      stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                             "Character Not Found");
      stack =
        (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                   get_config_message(messages_message,
                                                      "char_not_found"));
      stack = (unsigned char *)add_line(u, (char *)stack, end_string);

      u->fn_timer = login_timeout;
      u->count_timer = 3 * ONE_MINUTE;
      u->fn_input = selected_character;
    }
  }
  stack = oldstack;
}

void link_to_program(user * u)
{
  user *scan = users_list;
  int32 orig_fd = -1;
  unsigned char *oldstack = stack;

  memset(u->special_prompt, null_chr, MAX_PROMPT);

  for (; scan; scan = scan->next)
  {
    if (u == scan)
      continue;
    if (!strcasecmp(u->username, scan->username))
      break;
  }

  strcpy(u->normal_prompt, u->temp_prompt);

  if (scan)
  {
    current_user = scan;
    orig_fd = scan->fd;

    scan->fd = u->fd;
    u->fd = orig_fd;
    strcpy(scan->addr_inet, u->addr_inet);
    strcpy(scan->addr_num, u->addr_num);
    scan->term_width = u->term_width;
    scan->term_lines = u->term_lines;
    scan->term_type = u->term_type;
    scan->detected_term_width = u->detected_term_width;
    scan->detected_term_lines = u->detected_term_lines;
    scan->detected_term_type = u->detected_term_type;
    scan->time_login = 0;

    u->logged_in = 0;
    quit_user(u);
    u = scan;

    current_user = null_ptr;

    vtell_user(u, "\n\n** Welcome back, %s! **\n\n", u->username);
    vtell_all_users_but(u,
                        "\n++ %s has reconnected to IntrepidMUD ++\n\n",
                        u->username);
  }
  else
  {
    vtell_user(u, "\n\n** Welcome onboard, %s! **\n\n", u->username);
    vtell_all_users_but(u, "\n++ %s has entered IntrepidMUD ++\n\n",
                        u->username);
  }

  u->fn_timer = null_ptr;
  u->count_timer = 0;
  u->fn_input = null_ptr;
  u->logged_in = 1;

  stack = oldstack;
  save_user(u);
}
