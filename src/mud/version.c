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
#include "../include/version.h"

#include <ctype.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/proto.h"

#ifdef NEED_GETLINE_DECL
extern ssize_t getline(char **, size_t *, FILE *);
#endif

typedef struct processor
{
  char vendor_id[15];
  char model_name[60];
  float MHz;
  float bogomips;
} processor;

struct hardware_info
{
  int32 allgood;
  int32 processors;
  int32 ram;
  float bogomips;
  processor *processor;
} CPU;

void get_hardware_info(void)
{
  FILE *f = null_ptr;
  char *red = null_ptr, *scan;
  size_t bufsize;
  float tmp;
  struct stat sbuf;
  int32 cur_proc = 0;

  if (CPU.allgood > 0)
    return;

  if (!(f = fopen("/proc/cpuinfo", "r")))
  {
    CPU.allgood = 0;
    return;
  }

  while (getline(&red, &bufsize, f) > 0)
  {
    if (!strncasecmp(red, "processor", 9))
      CPU.processors++;
  }

  CPU.processor = (processor *) calloc(sizeof(processor), CPU.processors);
  fseek(f, 0, SEEK_SET);

  while (getline(&red, &bufsize, f) > 0)
  {
    if (!strncasecmp(red, "processor", 9))
    {
      sscanf(red, "%*s : %d", &cur_proc);
    }
    else if (!strncasecmp(red, "vendor_id", 9))
    {
      sscanf(red, "%*s : %s", CPU.processor[cur_proc].vendor_id);
      if (!mud_strcmp(CPU.processor[cur_proc].vendor_id, "GenuineIntel"))
        strcpy(CPU.processor[cur_proc].vendor_id, "Intel");
      else
        if (!mud_strcmp(CPU.processor[cur_proc].vendor_id, "AuthenticAMD"))
        strcpy(CPU.processor[cur_proc].vendor_id, "AMD");
      else
        if (!mud_strcmp(CPU.processor[cur_proc].vendor_id, "CyrixInstead"))
        strcpy(CPU.processor[cur_proc].vendor_id, "Cyrix");
    }
    else if (!strncasecmp(red, "model name", 10))
    {
      sscanf(red, "%*s %*s : %59c", CPU.processor[cur_proc].model_name);
      scan = &(CPU.processor[cur_proc].model_name[strlen
                                                  (CPU.processor
                                                   [cur_proc].model_name) -
                                                  1]);
      while (isspace(*scan))
        *scan-- = null_chr;
    }
    else if (!strncasecmp(red, "cpu MHz", 7))
    {
      sscanf(red, "%*s %*s : %f", &tmp);
      CPU.processor[cur_proc].MHz = (int32) (tmp + 0.5);
    }
    else if (!strncasecmp(red, "bogomips", 8))
    {
      sscanf(red, "%*s : %f", &tmp);
      CPU.processor[cur_proc].bogomips = tmp;
      CPU.bogomips += tmp;
    }
  }
  fclose(f);

  if (red)
    free(red);

  stat("/proc/kcore", &sbuf);
  CPU.ram = sbuf.st_size << 20;

  CPU.allgood = 1;
}

void version_full(user * u)
{
  unsigned char *oldstack = stack;
  int32 i;

  get_hardware_info();

  version_base(u);
  stack =
    (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                   "Processor Information");
  if (CPU.processors > 1)
  {
    stack =
      (unsigned char *)strprintf((char *)stack, add_string,
                                 "Number of Processors: %d\n",
                                 CPU.processors);
    stack =
      (unsigned char *)strprintf((char *)stack, add_string,
                                 "Total Bogomips: %.2f\n", CPU.bogomips);
  }
  for (i = 0; i < CPU.processors; i++)
  {
    if (CPU.processors > 1)
      stack = (unsigned char *)vadd_line_text(u, (char *)stack, add_string,
                                              "Processor %d", i + 1);
    stack =
      (unsigned char *)strprintf((char *)stack, add_string,
                                 "Vendor ID: %s\n",
                                 CPU.processor[i].vendor_id);
    stack =
      (unsigned char *)strprintf((char *)stack, add_string,
                                 "Model Name: %s\n",
                                 CPU.processor[i].model_name);
    if (CPU.processor[i].MHz > 1999.49)
      stack =
        (unsigned char *)strprintf((char *)stack, add_string,
                                   "CPU Speed: %.2f GHz\n",
                                   CPU.processor[i].MHz / 1000.0);
    else
      stack =
        (unsigned char *)strprintf((char *)stack, add_string,
                                   "CPU Speed: %d MHz\n",
                                   (int32) (CPU.processor[i].MHz + 0.5));
    stack =
      (unsigned char *)strprintf((char *)stack, add_string,
                                 "Bogomips: %.2f\n",
                                 CPU.processor[i].bogomips);
  }
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void version_short(user * u)
{
  unsigned char *oldstack = stack;

  version_base(u);
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void version_base(user * u)
{
  stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                         "IntrepidMUD Version Information");
  stack = (unsigned char *)strprintf((char *)stack, add_string,
                                     "This MUD is based on IntrepidMUD by Segtor (Mikael "
                                     "Segercrantz), HawkEyE (Dan Griffiths), and beaver (Dave "
                                     "Etheridge).\n");
  stack =
    (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                   "License");
  stack =
    (unsigned char *)strprintf((char *)stack, add_string,
                               "IntrepidMUD is released under the GNU General Public License, "
                               "version 3 or later. For full license text, see "
                               "\"help license\".\n");
  stack =
    (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                   "MUD Code");
  stack =
    (unsigned char *)strprintf((char *)stack, add_string,
                               "Running IntrepidMUD v%s base code.\n",
                               INTREPID_VERSION);
  stack =
    (unsigned char *)strprintf((char *)stack, add_string,
                               "Directory we're running in: %s\n", ROOT);
  stack =
    (unsigned char *)strprintf((char *)stack, add_string,
                               "%s running on %s.\n",
                               get_config_message(config_message,
                                                  "mud_name"),
#ifdef LINUX
                               "Linux"
#else
                               "Unknown O/S"
#endif
    );
  stack = (unsigned char *)strprintf((char *)stack, add_string,
                                     "Executable compiled %s\n",
                                     COMPILE_TIME);
}
