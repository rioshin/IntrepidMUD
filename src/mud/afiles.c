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
#include <sys/types.h>
#include <unistd.h>

#include "../include/proto.h"
#include "../include/room.h"
#include "../include/area.h"

#ifdef NEED_SCANDIR_DECL
extern int scandir(DIRENT_PROTO char *__restrict,
                   struct dirent ***__restrict,
                   int (*)(DIRENT_PROTO struct dirent *),
                   int (*)(DIRENT_PROTO struct dirent **,
                           DIRENT_PROTO struct dirent **));
extern int alphasort(DIRENT_PROTO struct dirent **,
                     DIRENT_PROTO struct dirent **);
#endif                          /* NEED_SCANDIR_DECL */

area *areas_list;

int32 valid_area(DIRENT_PROTO struct dirent *d)
{
  char *dotter, path[320];
  struct stat sbuf;

  if (!(dotter = strstr(d->d_name, ".area")) || strlen(dotter) != 5)
    return 0;

  snprintf(path, 319, "%s", d->d_name);
  dotter = strrchr(path, '.');
  *dotter = null_chr;

  if (stat(path, &sbuf) < 0)
    return 0;

  return 1;
}

void load_areas(void)
{
  struct dirent **de;
  int32 dc = 0, i;
  char path[320], *dot;

  areas_list = null_ptr;

  dc = scandir("files/areas", &de, valid_area, alphasort);
  if (dc < 1)
    return;

  for (i = 0; i < dc; i++)
  {
    snprintf(path, 319, "%s", de[i]->d_name);
    dot = strrchr(path, '.');
    *dot = null_chr;

    load_area(path);
    free(de[i]);
  }
  free(de);
}

void load_area(char *filename)
{
  FILE *f = fopen(filename, "r");

  if (!f)
  {
    vflog("areas", "Areafile '%s' not found", filename);
    return;
  }

  fclose(f);
}
