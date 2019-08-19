/*
 * IntrepidMUD
 * Common Library
 * Common features between the different MUD executables
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

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/proto.h"

file load_file(char *filename)
{
  /* Just pass on to the verbose version */
  return load_file_verbose(filename, 1);
}

file load_file_verbose(char *filename, int32 verbose)
{
  file f = { null_chr, 0 };
  int32 d;

  /* Try opening the file */
  d = open(filename, O_RDONLY);
  if (d < 0)
  {
    /* Open failed, log fact */
    if (verbose && strcasecmp(filename, "files/logs/log.msg"))
      vflog("error", "Can't find file: \"%s\"", filename);

    /* Create default file and return it */
    f.where = (char *)malloc(1);
    *(f.where) = null_chr;
    f.length = 0;

    return f;
  }

  /* Find the length of the file */
  f.length = lseek(d, 0, SEEK_END);
  if (f.length < 0)
  {
    /* We failed to find the size of the file, log fact */
    vflog("error", "lseek, load_file_verbose() for \"%s\", errno %d: %s",
          filename, errno, strerror(errno));

    /* And once again, return the default empty file */
    f.where = (char *)malloc(1);
    *(f.where) = null_chr;
    f.length = 0;

    return f;
  }

  /* Get to the beginning of the file */
  lseek(d, 0, SEEK_SET);

  /* And reserve enough memory for the contents to be loaded */
  f.where = (char *)malloc(f.length + 1);
  memset(f.where, null_chr, f.length + 1);

  /* Read the file */
  if (read(d, f.where, f.length) < 0)
  {
    /* Error while reading the file, log the fact */
    vflog("error", "Error reading file \"%s\"", filename);

    /* And return the default empty file, this time remembering to free the
       already reserved memory first */
    free(f.where);
    f.where = (char *)malloc(1);
    *(f.where) = null_chr;
    f.length = 0;

    return f;
  }

  /* Close the file and make sure it has the terminating null character
     at the end before returning it */
  close(d);
#ifdef DEBUG_VERBOSE
  vflog("boot", "Loaded file \"%s\"", filename);
#endif
  *(f.where + f.length) = null_chr;

  return f;
}
