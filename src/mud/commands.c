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
 * This program is distributed in the hope that it will be useful, but WIthOUT
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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include "../include/proto.h"
#include "../include/clist.h"

extern char *stpcpy(char *__restrict, __const char *__restrict);
#ifdef NEED_SCANDIR_DECL
extern int scandir(DIRENT_PROTO char *__restrict,
                   struct dirent ***__restrict,
                   int (*)(DIRENT_PROTO struct dirent *),
                   int (*)(DIRENT_PROTO struct dirent **,
                           DIRENT_PROTO struct dirent **));
extern int alphasort(DIRENT_PROTO struct dirent **,
                     DIRENT_PROTO struct dirent **);
#endif                          /* NEED_SCANDIR_DECL */
extern int kill(pid_t, int);

void say(user * u, char *str)
{
  char *scan = str;
  char buf[16];
  character *ch = u->current_char;

  if (!str || !*str)
  {
    tell_user(u, "Say what?\n");
    return;
  }

  while (*scan)
    scan++;
  scan--;
  memset(buf, null_chr, 16);

  if (*scan == '!')
    sprintf(buf, "exclaim");
  else if (*scan == '?')
    sprintf(buf, "ask");
  else
    sprintf(buf, "say");

  vtell_user(u, "You %s '%s^N'\n", buf, str);
  vtell_all_users_but(u, "%s %s %ss '%s^N'\n", ch->firstname,
                      ch->familyname, buf, str);
}

void emote(user * u, char *str)
{
  character *ch = u->current_char;

  if (!str || !*str)
  {
    tell_user(u, "Emote what?\n");
    return;
  }

  if (*str == '\'')
  {
    vtell_user(u, "You emote: %s %s%s^N\n", ch->firstname, ch->familyname,
               str);
    vtell_all_users_but(u, "%s %s%s^N\n", ch->firstname, ch->familyname,
                        str);
  }
  else
  {
    vtell_user(u, "You emote: %s %s %s^N\n", ch->firstname,
               ch->familyname, str);
    vtell_all_users_but(u, "%s %s %s^N\n", ch->firstname, ch->familyname,
                        str);
  }
}

void think(user * u, char *str)
{
  character *ch = u->current_char;

  if (!str || !*str)
  {
    tell_user(u, "think what?\n");
    return;
  }

  vtell_user(u, "You think . o O ( %s^N )\n", str);
  vtell_all_users_but(u, "%s %s thinks . o O ( %s^N )\n",
                      ch->firstname, ch->familyname, str);
}

void sing(user * u, char *str)
{
  character *ch = u->current_char;

  if (!str || !*str)
  {
    tell_user(u, "Sing what?\n");
    return;
  }

  vtell_user(u, "You sing o/~ %s^N o/~\n", str);
  vtell_all_users_but(u, "%s %s sings o/~ %s^N o/~\n",
                      ch->firstname, ch->familyname, str);
}

void list_commands(user * u, char *str)
{
  int32 i;
  unsigned char *oldstack = stack;
  command *scan;

  TESTVARP(str);
  strcpy((char *)stack, "You can use the following commands: ");
  stack = add_string(stack);
  for (i = 0; i < 27; i++)
  {
    for (scan = cmd_list[i]; scan->cmd != null_ptr; scan++)
    {
      if (!(scan->flags & CMD_INVIS))
      {
        strcpy((char *)stack, scan->cmd);
        strcat((char *)stack, " ");
        stack = add_string(stack);
      }
    }
  }

  vtell_user(u, "%s\n", (char *)oldstack);
  stack = oldstack;
}

void quit_mud(user * u, char *str)
{
  TESTVARP(str);
  vtell_all_users_but(u, "-- %s has left IntrepidMUD --\n", u->username);
  quit_user(u);
}

void who_is_on(user * u, char *str)
{
  user *scan;
  int32 count = 0;
  unsigned char *oldstack = stack;

  TESTVARP(str);

  for (scan = users_list; scan; scan = scan->next)
    if (scan->logged_in)
      count++;

  stack =
    (unsigned char *)vadd_line_text(u, (char *)stack, add_string,
                                    "There %s %d user%s on",
                                    count == 1 ? "is" : "are", count,
                                    count == 1 ? "" : "s");
  for (scan = users_list; scan; scan = scan->next, count--)
    if (scan->logged_in)
    {
      stack =
        (unsigned char *)strprintf((char *)stack, add_string, "%s %s%s ",
                                   scan->current_char->firstname,
                                   scan->current_char->familyname,
                                   count > 2 ? "," : (count ==
                                                      2 ? " and" : ""));
    }
  stack--;
  *stack++ = '\n';
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void nwho(user * u, char *str)
{
  user *scan;
  int32 count = 0, len, idle, h, m, s, d, i, j = 0, k = 0;
  unsigned char *oldstack = stack, *temp_stack;
  char line[80];
  character *ch;

  TESTVARP(str);

  for (scan = users_list; scan; scan = scan->next)
    if (scan->logged_in)
      count++;

  stack =
    (unsigned char *)vadd_line_text(u, (char *)stack, add_string,
                                    "There %s%s user%s on",
                                    count == 1 ? "is only one" : "are ",
                                    count == 1 ? "" : get_number(count),
                                    count == 1 ? "" : "s");
  for (scan = users_list; scan; scan = scan->next)
  {
    ch = scan->current_char;
    len = u->term_width - mud_strlen(ch->firstname) -
      mud_strlen(ch->familyname) - 27;
    idle = scan->time_idle / 86400;
    while (idle >= 10)
    {
      idle /= 10;
      len--;
    }
    d = scan->time_idle / 86400;
    h = (scan->time_idle / 3600) % 24;
    m = (scan->time_idle / 60) % 60;
    s = scan->time_idle % 60;
    temp_stack = stack;
    for (i = 0; i < 80; i++)
      line[i] = null_chr;
    stack = (unsigned char *)strprintf((char *)stack, add_string,
                                       " %s %s%s%*s Idle: %dd %02dh %02dmin %02ds\n",
                                       scan->username,
                                       scan->title[0] == '\'' ? "" : " ",
                                       len, scan->title, d, h, m, s);
    *(--stack) = null_chr;
    j = 0;
    for (i = 0; ((size_t) (i)) < strlen((char *)temp_stack); i++)
    {
      if (*temp_stack == '\n')
        line[j] = null_chr;
      else
        line[j] = *temp_stack++;
      if (j > 0 && line[j] == ' ' && line[j - 1] == ' ')
      {
        if (k == 0)
        {
          line[j++] = '-';
          line[j++] = ' ';
          k++;
        }
        else
          line[j] = null_chr;
      }
      else
      {
        if (j == 0 && line[j] == ' ')
          line[j] = null_chr;
        else
          j++;
      }
    }
    *(stack++) = '\n';
  }
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);
  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void pemote(user * u, char *str)
{
  character *ch = u->current_char;

  if (!str || !*str)
  {
    tell_user(u, "Pemote what?\n");
    return;
  }

  vtell_user(u, "You emote: %s %s'%s %s^N\n", ch->firstname,
             ch->familyname,
             ch->familyname[strlen(ch->familyname) - 1] == 's' ? "" : "s",
             str);
  vtell_all_users_but(u, "%s %s'%s %s^N\n", ch->firstname, ch->familyname,
                      ch->familyname[strlen(ch->familyname) - 1] ==
                      's' ? "" : "s", str);
}

void kill_arch(user * u, char *str)
{
  FILE *fp;
  int32 pid;

  TESTVARP(str);
  tell_user(u, " -=> Attempting to kill guardian archangel...\n");

  fp = fopen(ARCHANGEL_PID, "r");
  if (!fp)
  {
    tell_user(u, " -=> No PID file! Unsuccessful.\n");
    return;
  }

  fscanf(fp, "%d", &pid);
  fclose(fp);

  if (!kill(pid, 9))
  {
    tell_user(u, " -=> Archangel killed successfully.\n");
    unlink(ARCHANGEL_PID);
  }
  else
    tell_user(u, " -=> Kill failed! Unsuccessful.\n");
}

void kill_angel(user * u, char *str)
{
  FILE *fp;
  int32 pid;

  TESTVARP(str);
  tell_user(u, " -=> Attempting to kill guardian angel...\n");

  fp = fopen(ANGEL_PID, "r");
  if (!fp)
  {
    tell_user(u, " -=> No PID file! Unsuccessful.\n");
    return;
  }

  fscanf(fp, "%d", &pid);
  fclose(fp);

  if (!kill(pid, 9))
  {
    tell_user(u, " -=> Angel killed successfully.\n");
    unlink(ANGEL_PID);
  }
  else
    tell_user(u, " -=> Kill failed! Unsuccessful.\n");
}

void shutdown_mud(user * u, char *str)
{
  character *ch = u->current_char;

  TESTVARP(str);

  tell_user(u, " -=> You shutdown IntrepidMUD!\n\n");
  vtell_all_users_but(u, " -=> %s (%s %s) shuts down IntrepidMUD!\n\n",
                      u->username, ch->firstname, ch->familyname);
  close_down();
}

void reboot_mud(user * u, char *str)
{
  /* user *scan; */
  char server_name[256];
  /* int cpid;
     FILE *f;
     struct itimerval itimer; */

  TESTVARP(u);
  TESTVARP(str);

  sprintf(server_name, "-=> %s <=- MUD Server on port %d",
          get_config_message(config_message, "mud_name"),
          atoi(get_config_message(config_message, "port")));

}

void version_func(user * u, char *str)
{
  if (!str || !*str)
  {
    version_full(u);
    return;
  }

  if (!strcasecmp(str, "short"))
  {
    version_short(u);
    return;
  }

  version_full(u);
}

void pick_term(user * u, char *str)
{
  terminal *scan = null_ptr;
  unsigned char *oldstack = stack;

  if (!str || !*str)
  {
    if (u->default_term_type == TERM_NONE)
    {
      tell_user(u, "You haven't chosen a terminal type.\n\n");
    }

    for (scan = terms; scan->name != null_ptr; scan++)
    {
      if (!scan->hidden && scan->type == u->default_term_type)
      {
        vtell_user(u, "You have chosen the '%s' terminal type.\n\n",
                   scan->name);
      }
    }

    stack = (unsigned char *)strprintf((char *)stack, add_string,
                                       "You can choose one of the following terminal types: none ");
    for (scan = terms; scan->name != null_ptr; scan++)
    {
      if (!scan->hidden)
      {
        stack =
          (unsigned char *)strprintf((char *)stack, add_string, "%s ",
                                     scan->name);
      }
    }
    stack = (unsigned char *)strprintf((char *)stack, end_string, "\n");

    tell_user(u, (char *)oldstack);
    stack = oldstack;

    return;
  }

  if (!strcasecmp(str, "none"))
  {
    tell_user(u, "You clear your terminal type.\n");
    u->default_term_type = TERM_NONE;
    if (!u->detected_term_type)
      u->term_type = u->default_term_type;

    return;
  }

  for (scan = terms; scan->name != null_ptr; scan++)
  {
    if (!strcasecmp(str, scan->name))
    {
      vtell_user(u, "You set your terminal type to '%s'.\n", scan->name);
      u->default_term_type = scan->type;
      if (!u->detected_term_type)
        u->term_type = scan->type;

      return;
    }
  }

  stack = (unsigned char *)strprintf((char *)stack, add_string,
                                     "that wasn't one of the terminal type possibilites.\n\n");
  stack = (unsigned char *)strprintf((char *)stack, add_string,
                                     "the possible terminal types are: none ");
  for (scan = terms; scan->name != null_ptr; scan++)
  {
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       scan->name);
  }
  stack = (unsigned char *)strprintf((char *)stack, end_string, "\n");

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void help_command(user * u, char *str)
{
  file f;
  char buf[256], buf2[256];
  char *scan;
  unsigned char *oldstack = stack;

  if (!str || !*str)
  {
    tell_user(u, "About which command or subject do you want help?\n");
    return;
  }

  memset(buf2, null_chr, 256);
  memset(buf, null_chr, 256);

  snprintf(buf2, 128, "%s", str);
  scan = buf2;
  while (*scan)
  {
    if (*scan == ' ')
    {
      *scan = null_chr;
      break;
    }
    *scan = tolower(*scan);
    scan++;
  }
  sprintf(buf, "files/help/%s.help", buf2);

  f = load_file(buf);
  if (!(*f.where))
  {
    vtell_user(u, "No help found on subject '%s'.\n", buf2);
    free(f.where);
    return;
  }

  stack =
    (unsigned char *)vadd_line_text(u, (char *)stack, add_string,
                                    "Help on '%s'", buf2);
  stack =
    (unsigned char *)strprintf((char *)stack, add_string, "%s", f.where);
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void set_prompt(user * u, char *str)
{
  if (!str || !*str)
  {
    tell_user(u, "You clear your prompt.\n");
    memset(u->normal_prompt, null_chr, MAX_PROMPT);
    return;
  }

  memset(u->normal_prompt, null_chr, MAX_PROMPT);
  if (!strcasecmp(str, "reset"))
  {
    snprintf(u->normal_prompt, MAX_PROMPT - 1, "%s",
             get_config_message(prompts_message, "default_prompt"));
    tell_user(u, "You reset your prompt.\n");
    return;
  }

  snprintf(u->normal_prompt, MAX_PROMPT - 2, "%s", str);
  strcat(u->normal_prompt, " ");
  vtell_user(u, "You set your prompt to '%s'.\n", u->normal_prompt);
}

void term_lines(user * u, char *str)
{
  int32 count = 0;

  if (!str || !*str)
  {
    vtell_user(u, "Your pager is currently set to %d line%s.\n",
               u->term_lines,
               u->term_lines ==
               DEFAULT_TERM_LINES ? "s (the MUD default)" : (u->term_lines
                                                             ==
                                                             1) ? "" :
               "s");
    if (u->detected_term_lines)
      vtell_user(u, "Your pager lines has been detected as %d line%s.\n",
                 u->detected_term_lines,
                 u->detected_term_lines == 1 ? "" : "s");
    return;
  }

  if (!strcasecmp(str, "reset"))
  {
    vtell_user(u, "You reset your terminal line count to %d.\n",
               u->default_term_lines ? u->default_term_lines :
               DEFAULT_TERM_LINES);
    u->default_term_lines = DEFAULT_TERM_LINES;
    u->term_lines = u->detected_term_lines;
    if (!u->detected_term_lines)
      u->term_lines = u->default_term_lines;
  }

  count = atoi(str);
  if (!count || count < 5 || count > 40)
  {
    vtell_user(u,
               "You need to enter a number between 5 and 40, "
               "inclusive, or 'reset'.\n"
               "Please remember that the default is %d.\n",
               DEFAULT_TERM_LINES);
    return;
  }

  u->term_lines = count;
  vtell_user(u, "You set your terminal line count to %d.\n",
             u->term_lines);
  if (!u->detected_term_lines)
  {
    u->term_lines = u->default_term_lines;
    vtell_user(u, "(But it's restored to the default of %d.)\n",
               u->default_term_lines);
  }
}

void term_width(user * u, char *str)
{
  int32 count = 0;

  if (!str || !*str)
  {
    vtell_user(u, "Your terminal width is currently set to %d char%s.\n",
               u->term_width,
               u->term_width == 77 ? "s (the MUD default)" :
               u->term_width == 1 ? "" : "s");
    if (u->detected_term_width)
      vtell_user(u,
                 "Your terminal width has been detected as %d char%s.\n",
                 u->detected_term_width,
                 u->detected_term_width == 1 ? "" : "s");
    return;
  }

  if (!strcasecmp(str, "reset"))
  {
    tell_user(u, "You reset your terminal width to 77.\n");
    u->default_term_width = 77;
    u->term_width = u->detected_term_width;
    if (!u->detected_term_width)
      u->term_width = u->default_term_width;
  }

  count = atoi(str);
  if (!count || count < 30 || count > 160)
  {
    tell_user(u, "You need to enter a number between 30 and 160, "
              "inclusive, or 'reset'.\n"
              "Please remember that the default is 77.\n");
    return;
  }

  u->default_term_width = count;
  vtell_user(u, "You set your terminal width to %d.\n",
             u->default_term_width);
  if (!u->detected_term_width)
  {
    u->term_width = u->default_term_width;
    vtell_user(u, "(But it's restored to the default of %d.)\n",
               u->default_term_width);
  }
}

void term_compare(user * u, char *str)
{
  unsigned char *oldstack = stack;
  char *curterm = "none", *defterm = "none";
  terminal *scan;

  TESTVARP(str);

  for (scan = terms; scan->name != null_ptr; scan++)
  {
    if (!scan->hidden && scan->type == u->default_term_type)
      defterm = scan->name;
    if (!scan->hidden && scan->type == u->term_type)
      curterm = scan->name;
  }

  stack =
    (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                   "Terminal Comparison");
  stack =
    (unsigned char *)strprintf((char *)stack, add_string,
                               "Default terminal type:  %-10s Terminal type:  %s%s\n",
                               defterm, curterm,
                               u->detected_term_type ? " (detected)" : "");
  stack =
    (unsigned char *)strprintf((char *)stack, add_string,
                               "Default terminal width: %-10d Terminal width: %d%s\n",
                               u->default_term_width, u->term_width,
                               u->detected_term_width ? " (detected)" :
                               "");
  stack =
    (unsigned char *)strprintf((char *)stack, add_string,
                               "Default terminal lines: %-10d Terminal lines: %d%s\n",
                               u->default_term_lines, u->term_lines,
                               u->detected_term_lines ? " (detected)" :
                               "");
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

/* the wake command */
void wake(user * u, char *str)
{
  user *u2;
  character *ch1, *ch2;

  ch1 = u->current_char;

  if (!*str)
  {
    tell_user(u, " Format : wake <player>\n");
    return;
  }

  if (!user_exists_name(str))
    return;

  if (!u)
    return;

  u2 = load_user_name(str);
  if (!u2)
    return;

  ch2 = u2->current_char;

  vtell_user(u2,
             "!!!!!!!!!! OI !!!!!!!!!! WAKE UP, %s %s wants to talk to you!",
             ch1->firstname, ch1->familyname);

  vtell_user(u,
             " You scream loudly at %s %s in an attempt to wake them up.\n",
             ch2->firstname, ch2->familyname);
}

void wall(user * u, char *str)
{
  character *ch = u->current_char;

  if (!*str)
  {
    tell_user(u, " Format: wall <message>\n");
    return;
  }

  vtell_all_users("  %s %s announces -=*> %s ^N<*=-^W\n",
                  ch->firstname, ch->familyname, str);
}

void recap(user * u, char *str)
{
  character *ch = u->current_char;
#ifdef ___NOTHING___
  char *tmp = str;
#endif
  char *name = null_ptr;

  if (!str || !*str)
  {
    tell_user(u, " Format: recap <new name>\n");
    return;
  }

  name = (char *)malloc(42);
  if (!name)
    return;
  memset(name, null_chr, 42);
  name =
    strprintf(name, end_string, "%s %s", ch->firstname, ch->familyname);

#ifdef ___NOTHING___
  while (*tmp)
  {
    if (*tmp == ' ')
    {
      *tmp = null_chr;
      break;
    }
    tmp++;
  }

  if (strcasecmp(name, str))
  {
    tell_user(u, " But that name doesn't match your current name!\n");
    return;
  }

  strcpy(u->username, str);
  vtell_user(u, " Name recapped. You are now known as %s.\n", u->username);
#endif
}

int32 valid_log(DIRENT_PROTO struct dirent * de)
{
  char *dotter;

  if (!(dotter = strstr(de->d_name, ".log")) || strlen(dotter) != 4)
    return 0;

  return 1;
}

int32 priv_for_log(user * u)
{
  if (u->residency & HCADMIN)
    return S_IRUSR;
  if (u->residency & LOWER_ADMIN)
    return S_IRGRP;
  return S_IROTH;
}

void view_log(user * u, char *str)
{
  struct dirent **de;
  struct stat sbuf;
  int32 dc = 0, i, hits = 0;
  char path[320];
  unsigned char *oldstack = stack;
  FILE *f;

  memset(path, null_chr, 320);

  if (!*str)
  {
    memset(stack, null_chr, 4096);
    snprintf(path, 319, "files/logs/");

    dc = scandir(path, &de, valid_log, alphasort);
    if (dc > 0)
    {
      add_line_text(u, (char *)stack, add_string, "Accessible logs");
      for (i = 0; i < dc; i++)
      {
        sprintf(path, "files/logs/%s", de[i]->d_name);

        if (stat(path, &sbuf) < 0)
          continue;

        if (!(sbuf.st_mode & priv_for_log(u)))
          continue;

        hits++;
        stack = (unsigned char *)stpcpy((char *)stack, de[i]->d_name);
        while (*stack != '.')
          *stack-- = null_chr;
        *stack++ = ',';
        *stack++ = ' ';
        free(de[i]);
      }
      free(de);

      if (!hits)
      {
        stack = oldstack;
        tell_user(u,
                  " There are currently no log files available to you.\n");
        return;
      }

      while (*stack != ',')
        *stack-- = null_chr;
      stack += sprintf((char *)stack, ".\n");

      vadd_line_text(u, (char *)stack, end_string,
                     "There %s %d log%s available to you",
                     hits == 1 ? "is" : "are", hits, hits == 1 ? "" : "s");
      tell_user(u, (char *)oldstack);
      stack = oldstack;
      return;
    }
    else
      tell_user(u, "^H -=> No log files are available^N\n");

    if (de)
    {
      free(de);
    }
    else
    {
      snprintf(path, 159, "files/logs/%s.log", str);
      f = fopen(path, "r");
      if (!f)
      {
        vtell_user(u, "Unable to open the '%s' log!\n", str);
        return;
      }
    }
  }
}

void setidle(user * u, char *str)
{
  int idle;

  if (!str || !*str)
  {
    tell_user(u, " Format: set_idle <minutes>\n");
    return;
  }

  idle = atoi(str) * 60;
  u->time_idle = idle;
}

void list_addresses(user * u, char *str)
{
  user *scan;
  int32 count = 0, i, j = 0, k = 0;
  unsigned char *oldstack = stack, *temp_stack;
  char line[80];
  character *ch;

  TESTVARP(str);

  for (scan = users_list; scan; scan = scan->next)
    if (scan->logged_in)
      count++;

  stack =
    (unsigned char *)vadd_line_text(u, (char *)stack, add_string,
                                    "There %s%s user%s on",
                                    count == 1 ? "is only one" : "are ",
                                    count == 1 ? "" : get_number(count),
                                    count == 1 ? "" : "s");
  for (scan = users_list; scan; scan = scan->next)
  {
    temp_stack = stack;
    for (i = 0; i < 80; i++)
      line[i] = null_chr;
    ch = scan->current_char;
    stack = (unsigned char *)strprintf((char *)stack, add_string,
                                       " %s %s from %s (%s)\n",
                                       ch->firstname, ch->familyname,
                                       scan->addr_inet, scan->addr_num);
    *(--stack) = null_chr;
    j = 0;
    for (i = 0; ((size_t) (i)) < strlen((char *)temp_stack); i++)
    {
      if (*temp_stack == '\n')
        line[j] = null_chr;
      else
        line[j] = *temp_stack++;
      if (j > 0 && line[j] == ' ' && line[j - 1] == ' ')
      {
        if (k == 0)
        {
          line[j++] = '-';
          line[j++] = ' ';
          k++;
        }
        else
          line[j] = null_chr;
      }
      else
      {
        if (j == 0 && line[j] == ' ')
          line[j] = null_chr;
        else
          j++;
      }
    }
    *(stack++) = '\n';
  }
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);
  tell_user(u, (char *)oldstack);
  stack = oldstack;
}
