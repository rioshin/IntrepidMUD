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
extern int scandir(DIRENT_PROTO char *__restrict,
                   struct dirent ***__restrict,
                   int (*)(DIRENT_PROTO struct dirent *),
                   int (*)(DIRENT_PROTO struct dirent **,
                           DIRENT_PROTO struct dirent **));
extern int alphasort(DIRENT_PROTO struct dirent **,
                     DIRENT_PROTO struct dirent **);
#endif                          /* NEED_SCANDIR_DECL */

character *characters_list;

character *load_char(user * u, char *name)
{
  file f;
  char *filename, firstname[MAX_CHAR_NAME + 1];
  char familyname[MAX_CHAR_NAME + 1], *space;
  unsigned char *r;
  character *c;

  TESTVARP(u);
  TESTVARP(name);

  filename = (char *)malloc(256);
  if (!filename)
    return null_ptr;

  c = (character *) malloc(sizeof(character));
  if (!c)
  {
    free(filename);
    return null_ptr;
  }

  memset(&f, null_chr, sizeof(file));
  memset(filename, null_chr, 256);

  space = next_space(name);
  *space++ = null_chr;

  memcpy(firstname, name, MAX_CHAR_NAME);
  memcpy(familyname, space, MAX_CHAR_NAME);

  filename = strprintf((char *)stack, end_string,
                       "files/characters/%c/%c/%s.char",
                       familyname[0], familyname[1], firstname);

  f = load_file(filename);
  if (*(f.where) == null_chr)
  {
    free(f.where);
    free(c);
    free(filename);
    return null_ptr;
  }

  r = (unsigned char *)f.where;
  r = get_string_safe(c->firstname, r, f);
  r = get_string_safe(c->familyname, r, f);
  r = get_string_safe(c->user, r, f);
  r = get_int32_safe((int32 *) & c->remorted, r, f);
  r = get_int32_safe((int32 *) & c->level, r, f);
  r = get_int32_safe((int32 *) & c->align, r, f);
  r = get_int32_safe((int32 *) & c->ethos, r, f);
  /* load race here (str) */
  /* load class here (str) */
  /* load location here (str.str) */
  /* todo: continue here */

  return c;
}

file save_char_data(user * u, character * c)
{
  file f;

  TESTVARP(u);
  TESTVARP(c);

  f.where = (char *)stack;
  *stack = null_chr;
  stack = store_string(stack, c->firstname);
  stack = store_string(stack, c->familyname);
  stack = store_string(stack, c->user);
  stack = store_int32(stack, c->remorted);
  stack = store_int32(stack, c->level);
  stack = store_int32(stack, c->align);
  stack = store_int32(stack, c->ethos);
  /* todo: continue adding fields *here* */
  /* save race here */
  /* stack = store_string(stack, c->race->name); */
  /* save class here */
  /* stack = store_string(stack, c->class->name); */
  /* save location here */
  /* stack = store_string(stack, c->room->area->areaname); */
  /* stack = store_string(stack, c->room->roomname); */
  /* todo: continue adding fields *here* */
  f.length = (pint) stack - (pint) f.where;

  return f;
}

void save_char(user * u, character * c)
{
  FILE *fp;
  file f;
  char path[160], buf[(MAX_CHAR_NAME * 2) + 2],
    temp[(MAX_CHAR_NAME * 2) + 2];

  TESTVARP(u);
  TESTVARP(c);

  sprintf(temp, "%s", c->firstname);
  sprintf(buf, "%s", lower_case(temp));

  sprintf(path, "files/characters/%c/%c/%s.char", buf[0], buf[1], buf);

  f = save_char_data(u, c);

  fp = fopen(path, "wb");
  if (fp)
  {
    fwrite(f.where, f.length, 1, fp);
    fflush(fp);
    fclose(fp);
    fp = null_ptr;
  }
}

void create_char_dirs(void)
{
  struct stat sbuf;
  char c1, c2, path[160];

  memset(path, null_chr, 160);
  for (c1 = 'a'; c1 <= 'z'; c1++)
  {
    sprintf(path, "files/characters/%c", c1);

    if (stat(path, &sbuf) < 0)
      mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR);

    for (c2 = 'a'; c2 <= 'z'; c2++)
    {
      sprintf(path, "files/characters/%c/%c", c1, c2);

      if (stat(path, &sbuf) < 0)
        mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR);
    }
  }

  scan_chars();
}

int32 valid_char(DIRENT_PROTO struct dirent *d)
{
  char *dotter;

  if (!(dotter = strstr(d->d_name, ".char")) || strlen(dotter) != 5)
    return 0;

  return 1;
}

void scan_chars(void)
{
  char c1, c2, path[320], name[257];
  character *c, *last;
  struct dirent **de;
  int32 dc = 0, i;

  last = null_ptr;
  memset(name, null_chr, 257);

  for (c1 = 'a'; c1 <= 'z'; c1++)
  {
    for (c2 = 'a'; c2 <= 'z'; c2++)
    {
      snprintf(path, 319, "files/characters/%c/%c", c1, c2);

      dc = scandir(path, &de, valid_char, alphasort);
      if (dc > 0)
      {
        for (i = 0; i < dc; i++)
        {
          c = (character *) malloc(sizeof(character));
          memset(c, null, sizeof(character));

          snprintf(name, 256, "%s", de[i]->d_name);
          snprintf(c->firstname, MAX_CHAR_NAME - 1, "%s", name);
          free(de[i]);

          c->next = null_ptr;
          if (!characters_list)
            characters_list = c;
          if (last)
            last->next = c;
          last = c;
        }
        free(de);
      }
    }
  }
}
