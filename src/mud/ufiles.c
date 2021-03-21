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

#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/proto.h"
#include "../include/user.h"
#include "../include/character.h"

#ifdef NEED_SCANDIR_DECL
extern int scandir(__const char *__restrict,
                   struct dirent ***__restrict,
                   int (*)(__const struct dirent *),
                   int (*)(__const struct dirent **,
                           __const struct dirent **));
extern int alphasort(__const struct dirent **, __const struct dirent **);
#endif

typedef struct ulist
{
  char name[MAX_USER_NAME];
  struct ulist *next;
} ulist;

ulist *all_users;

void load_user(user * u)
{
  char buf[MAX_USER_NAME + 1], temp[MAX_USER_NAME + 1];
  char path[160], *dot;
  unsigned char *r;
  file f;

  sprintf(temp, "%s", u->username);
  sprintf(buf, "%s", lower_case(temp));

  sprintf(path, "files/users/%c/%c/%s.user", buf[0], buf[1], buf);
  f = load_file(path);

  if (*(f.where) == null_chr)
  {
    free(f.where);
    return;
  }

  r = (unsigned char *)f.where;

  r = get_string_safe(u->username, r, f);
  r = get_string_safe(u->password, r, f);
  r = get_string_safe(u->normal_prompt, r, f);
  r = get_int32_safe((int32 *) & u->default_term_type, r, f);
  r = get_int32_safe((int32 *) & u->default_term_width, r, f);
  r = get_int32_safe((int32 *) & u->input_flags, r, f);
  r = get_int32_safe((int32 *) & u->conn_flags, r, f);
  r = get_int32_safe((int32 *) & u->default_term_lines, r, f);
  r = get_int32_safe((int32 *) & u->residency, r, f);
  /* todo: continue here */

  dot = strrchr(path, '.');
  *dot = null_chr;
  load_user_characters(u, path);
}

file save_user_data(user * u)
{
  file f;

  f.where = (char *)stack;
  *stack = null_chr;
  stack = store_string(stack, u->username);
  stack = store_string(stack, u->password);
  stack = store_string(stack, u->normal_prompt);
  stack = store_int32(stack, u->default_term_type);
  stack = store_int32(stack, u->default_term_width);
  stack = store_int32(stack, u->input_flags);
  stack = store_int32(stack, u->conn_flags);
  stack = store_int32(stack, u->default_term_lines);
  stack = store_int32(stack, u->residency);
  /* todo: continue adding fields *here* */

  f.length = (pint) stack - (pint) f.where;

  return f;
}

void save_user(user * u)
{
  file f;
  char path[160], buf[MAX_USER_NAME + 1], temp[MAX_USER_NAME + 1], *dot;
  FILE *fp;

  create_user_dirs();

  sprintf(temp, "%s", u->username);
  sprintf(buf, "%s", lower_case(temp));

  sprintf(path, "files/users/%c/%c/%s.user", buf[0], buf[1], buf);

  f = save_user_data(u);

  fp = fopen(path, "wb");
  if (fp)
  {
    fwrite(f.where, f.length, 1, fp);
    fflush(fp);
    fclose(fp);
    fp = null_ptr;
  }

  dot = strrchr(path, '.');
  *dot = null_chr;

  save_user_characters(u, path);
}

user *load_user_name(char *str)
{
  user *u = create_user(), *scan;
  char *tmp = lower_case(str);

  for (scan = users_list; scan; scan = scan->next)
  {
    if (!strcasecmp(tmp, scan->username))
      return scan;
  }

  strcpy(u->username, tmp);

  load_user(u);

  return u;
}

int32 user_exists(user * u)
{
  char buf[MAX_USER_NAME + 1], temp[MAX_USER_NAME + 1], path[160];
  struct stat sbuf;

  sprintf(temp, "%s", u->username);
  sprintf(buf, "%s", lower_case(temp));

  sprintf(path, "files/users/%c/%c/%s.user", buf[0], buf[1], buf);

  if (stat(path, &sbuf) < 0)
    return 0;

  return 1;
}

int32 user_exists_name(char *name)
{
  char buf[MAX_USER_NAME + 1], temp[MAX_USER_NAME + 1], path[160];
  struct stat sbuf;

  sprintf(temp, "%s", name);
  sprintf(buf, "%s", lower_case(temp));

  sprintf(path, "files/users/%c/%c/%s.user", buf[0], buf[1], buf);

  if (stat(path, &sbuf) < 0)
    return 0;

  return 1;
}

void create_user_dirs(void)
{
  struct stat sbuf;
  char c1, c2, path[160];

  memset(path, null_chr, 160);
  for (c1 = 'a'; c1 <= 'z'; c1++)
  {
    sprintf(path, "files/users/%c", c1);

    if (stat(path, &sbuf) < 0)
      mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR);

    for (c2 = 'a'; c2 <= 'z'; c2++)
    {
      sprintf(path, "files/users/%c/%c", c1, c2);

      if (stat(path, &sbuf) < 0)
        mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR);
    }
  }

  scan_users();
}

int32 valid_user(DIRENT_PROTO struct dirent *d)
{
  char *dotter;

  if (!(dotter = strstr(d->d_name, ".user")) || strlen(dotter) != 5)
    return 0;

  return 1;
}

void scan_users(void)
{
  char c1, c2, path[320];
  ulist *u, *last;
  struct dirent **de;
  int32 dc = 0, i;
  char name[256];

  all_users = null_ptr;
  last = null_ptr;

  memset(name, null_chr, 256);

  for (c1 = 'a'; c1 <= 'z'; c1++)
  {
    for (c2 = 'a'; c2 <= 'z'; c2++)
    {
      snprintf(path, 319, "files/users/%c/%c", c1, c2);

      dc = scandir(path, &de, valid_user, alphasort);
      if (dc > 0)
      {
        for (i = 0; i < dc; i++)
        {
          u = (ulist *) malloc(sizeof(ulist));
          memset(u, null, sizeof(ulist));

          snprintf(name, 256, "%s", de[i]->d_name);
          snprintf(u->name, MAX_USER_NAME - 1, "%s", name);
          free(de[i]);

          u->next = null_ptr;
          if (!all_users)
            all_users = u;
          if (last)
            last->next = u;
          last = u;
        }
        free(de);
      }
    }
  }
}

void load_user_characters(user * u, char *dir)
{
  TESTVARP(u);
  TESTVARP(dir);
}

void save_user_characters(user * u, char *dir)
{
  TESTVARP(u);
  TESTVARP(dir);
}
