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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "../include/proto.h"
#include "../include/user.h"
#include "../include/character.h"
#include "../include/class.h"
#include "../include/race.h"

race *races_all = NULL;
class *classes_all = NULL;

void load_characters(user * u)
{
  FILE *f;
  char filename[256], *line;
  unsigned char *oldstack = stack;
  character *c;
  int l;

  TESTVARP(u);

  memset(filename, null_chr, 256);

  line = (char *)malloc(256);
  if (!line)
    return;

  memset(line, null_chr, 256);

  (void)strprintf((char *)stack, end_string,
                  "files/users/%c/%c/%s.chars",
                  u->username[0], u->username[1], u->username);
  f = fopen((char *)oldstack, "r");
  if (!f)
  {
    stack = oldstack;
    free(line);
    return;
  }

  u->user_chars = null_ptr;

  fscanf(f, "%s\n", line);
  while (*line)
  {
    l = strlen(line) - 1;
    if (l > 255)
      l = 255;
    line[l] = null_chr;
    c = load_char(u, line);
    c->user_next = u->user_chars;
    u->user_chars = c;
    fscanf(f, "%s\n", line);
  }

  fclose(f);
  free(line);
  stack = oldstack;
}

void save_characters(user * u)
{
  FILE *f;
  char filename[256], *line, buf[MAX_USER_NAME + 1];
  character *c;
  unsigned char *oldstack = stack;

  TESTVARP(u);

  memset(filename, null_chr, 256);
  memset(buf, null_chr, MAX_USER_NAME + 1);

  line = (char *)malloc(256);
  if (!line)
  {
    stack = oldstack;
    return;
  }
  memset(line, null_chr, 256);

  sprintf(buf, "%s", lower_case(u->username));


  (void)strprintf((char *)stack, end_string,
                  "files/users/%c/%c/%s.chars", buf[0], buf[1], buf);
  f = fopen((char *)oldstack, "w");
  if (!f)
  {
    free(line);
    stack = oldstack;
    return;
  }

  c = u->user_chars;
  while (c)
  {
    line = strprintf((char *)stack, end_string, "%s %s",
                     c->firstname, c->familyname);
    fprintf(f, "%s\n", line);
    save_char(u, c);
    c = c->user_next;
  }

  fflush(f);
  fclose(f);
  free(line);
  stack = oldstack;
}

int32 max_characters(user * u)
{
  int max = 5;
  character *uc;
  int found = 0;

  TESTVARP(u);

  if (u->residency & HCADMIN)
    max += 15;
  else if (u->residency & ADMIN || u->residency & LOWER_ADMIN)
    max += 10;
  else if (u->residency & SU || u->residency & ADVANCED_SU)
    max += 5;

  load_characters(u);
  uc = u->user_chars;

  while (uc)
  {
    if (!found && is_immortal(uc))
    {
      max += 10;
      found = 1;
    }
    else if (!found && is_hero(uc))
    {
      max += 5;
      found = 1;
    }

    uc = uc->user_next;
  }

  return max;
}

int32 is_immortal(character * c)
{
  if (c->level > atoi(get_config_message(config_message, "hero")))
    return 1;

  return 0;
}

int32 is_hero(character * c)
{
  if (c->remorted > 0
      || c->level == atoi(get_config_message(config_message, "hero")))
    return 1;

  return 0;
}

int32 user_has_immortal(user * u)
{
  int ret = 0;
  character *uc;

  load_characters(u);
  uc = u->user_chars;

  while (uc)
  {
    if (is_immortal(uc))
      ret = 1;

    uc = uc->user_next;
  }

  return ret;
}

int32 user_has_hero(user * u)
{
  int ret = 0;
  character *uc;

  load_characters(u);
  uc = u->user_chars;

  while (uc)
  {
    if (is_hero(uc))
      ret = 1;

    uc = uc->user_next;
  }

  return ret;
}

void create_character(user * u)
{
  race *r = races_all;
  race *h;
  class *c = classes_all;
  class *w;
  int num_races = 0, num_classes = 0;
  unsigned char *oldstack = stack;

  TESTVARP(u);

  while (r)
  {
    num_races++;
    r = r->next_race;
  }

  while (c)
  {
    num_classes++;
    c = c->next_class;
  }

#ifdef DEBUGGING
  vflog("character", "Found %d races and %d classes", num_races,
        num_classes);
#endif

  if (!num_races)
  {
    /* only the built-in human is available - add it to the list */
    h = (race *) malloc(sizeof(race));
    if (!h)
      return;
    h->max_abilities[strength] = 18;
    h->max_abilities[dexterity] = 18;
    h->max_abilities[constitution] = 18;
    h->max_abilities[intelligence] = 18;
    h->max_abilities[wisdom] = 18;
    h->max_abilities[charisma] = 18;
    h->default_abilities[strength] = 13;
    h->default_abilities[dexterity] = 13;
    h->default_abilities[constitution] = 13;
    h->default_abilities[intelligence] = 13;
    h->default_abilities[wisdom] = 13;
    h->default_abilities[charisma] = 13;
    memset(h->name, null_chr, MAX_USER_NAME);
    strcpy(h->name, "human");
    h->next_race = null_ptr;

    races_all = h;
    r = races_all;
    race_save(u, h);
  }

  if (!num_classes)
  {
    /* only the built-in warrior is available - add it to the list */
    w = (class *) malloc(sizeof(class));
    if (!w)
      return;
    w->abilitymods[strength] = 0;
    w->abilitymods[dexterity] = 0;
    w->abilitymods[constitution] = 0;
    w->abilitymods[intelligence] = 0;
    w->abilitymods[wisdom] = 0;
    w->abilitymods[charisma] = 0;
    memset(w->name, null_chr, MAX_USER_NAME);
    strcpy(w->name, "warrior");
    w->next_class = null_ptr;

    classes_all = w;
    c = classes_all;
    class_save(u, w);
  }

  stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                         "Choose your race");
  stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                     get_config_message(messages_message,
                                                        "select_race"));
  r = races_all;
  while (r)
  {
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s   ",
                                       r->name);
    r = r->next_race;
  }
  *stack++ = '\n';
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);
  u->fn_timer = create_timeout;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_input = got_race;

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void create_timeout(user * u)
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

void got_race(user * u, char *str)
{
  race *r = races_all;
  class *cl = classes_all;
  character *ch;
  unsigned char *oldstack = stack;

  TESTVARP(u);
  TESTVARP(str);

  if (!str)
  {
    /* we didn't get any race, show options again and put the entry
     * back to this function, then exit the function */
    stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                           "Choose your race");
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       get_config_message(messages_message,
                                                          "select_race"));
    r = races_all;
    while (r)
    {
      stack =
        (unsigned char *)strprintf((char *)stack, add_string, "%s   ",
                                   r->name);
      r = r->next_race;
    }
    *stack++ = '\n';
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);
    u->fn_timer = create_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_race;

    tell_user(u, (char *)oldstack);
    stack = oldstack;
    return;
  }

  /* we got an entry, scan races for a match */
  r = races_all;
  while (r)
  {
    if (!strcasecmp(str, r->name))
      break;
    r = r->next_race;
  }

  /* if we found no match, give options again and set return to here, then
   * exit the function */
  if (!r)
  {
    stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                           "Choose your race");
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       get_config_message(messages_message,
                                                          "select_race"));
    r = races_all;
    while (r)
    {
      stack =
        (unsigned char *)strprintf((char *)stack, add_string, "%s   ",
                                   r->name);
      r = r->next_race;
    }
    *stack++ = '\n';
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);
    u->fn_timer = create_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_race;

    tell_user(u, (char *)oldstack);
    stack = oldstack;
    return;
  }

  /* create a new character, if failing just quit the user and exit */
  ch = (character *) malloc(sizeof(character));
  if (!ch)
  {
    quit_user(u);
    return;
  }

  /* full-wipe the just got character and copy the race info in */
  memset(ch, null_chr, sizeof(character));
  ch->race = r;

#ifdef DEBUGGING
  vflog("character", "Got race %s", ch->race->name);
#endif

  /* set the character as the user's current character */
  u->current_char = ch;

  /* create the options for picking the class, show them to the user
   * and exit the function */
  stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                         "Choose your class");
  stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                     get_config_message(messages_message,
                                                        "select_class"));
  cl = classes_all;
  while (cl)
  {
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s   ",
                                       cl->name);
    cl = cl->next_class;
  }
  *stack++ = '\n';
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);
  u->fn_timer = create_timeout;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_input = got_class;

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void got_class(user * u, char *str)
{
  class *cl = classes_all;
  character *ch = u->current_char;
  unsigned char *oldstack = stack;

  TESTVARP(u);
  TESTVARP(str);

  if (!str)
  {
    /* we didn't get anything, request again */
    stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                           "Choose your class");
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       get_config_message(messages_message,
                                                          "select_class"));
    cl = classes_all;
    while (cl)
    {
      stack =
        (unsigned char *)strprintf((char *)stack, add_string, "%s   ",
                                   cl->name);
      cl = cl->next_class;
    }
    *stack++ = '\n';
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);
    u->fn_timer = create_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_class;

    tell_user(u, (char *)oldstack);
    stack = oldstack;
    return;
  }

  /* we got an entry, scan classes for a match */
  cl = classes_all;
  while (cl)
  {
    if (!strcasecmp(str, cl->name))
      break;
    cl = cl->next_class;
  }

  if (!cl)
  {
    /* we didn't find a match, request again */
    stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                           "Choose your class");
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       get_config_message(messages_message,
                                                          "select_class"));
    cl = classes_all;
    while (cl)
    {
      stack =
        (unsigned char *)strprintf((char *)stack, add_string, "%s   ",
                                   cl->name);
      cl = cl->next_class;
    }
    *stack++ = '\n';
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);
    u->fn_timer = create_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_class;

    tell_user(u, (char *)oldstack);
    stack = oldstack;
    return;
  }

  ch->class = cl;

#ifdef DEBUGGING
  vflog("character", "Got class %s", cl->name);
#endif

  /* get the character's first name */
  stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                         "Pick your firstname");
  stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                     get_config_message(messages_message,
                                                        "pick_firstname"));
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);
  u->fn_timer = create_timeout;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_input = got_firstname;

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void got_firstname(user * u, char *str)
{
  character *ch = u->current_char;
  unsigned char *oldstack = stack;

  if (!str)
  {
    /* we didn't get a first name, ask again and exit */
    stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                           "Pick your firstname");
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       get_config_message(messages_message,
                                                          "pick_firstname"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);
    u->fn_timer = create_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_firstname;

    tell_user(u, (char *)oldstack);
    stack = oldstack;
    return;
  }

  memset(ch->firstname, null_chr, MAX_CHAR_NAME);
  strncpy(ch->firstname, str, MAX_CHAR_NAME - 1);

#ifdef DEBUGGING
  vflog("character", "Got first name (%s)", ch->firstname);
#endif

  /* ask for the family name of the character */
  stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                         "Pick your familyname");
  stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                     get_config_message(messages_message,
                                                        "pick_familyname"));
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);
  u->fn_timer = create_timeout;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_input = got_familyname;

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void got_familyname(user * u, char *str)
{
  character *ch = u->current_char;
  unsigned char *oldstack = stack;

  if (!str || (str && !*str))
  {
    stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                           "Pick your familyname");
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       get_config_message(messages_message,
                                                          "pick_familyname"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);
    u->fn_timer = create_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_familyname;

    tell_user(u, (char *)oldstack);
    stack = oldstack;
    return;
  }

  memset(ch->familyname, null_chr, MAX_CHAR_NAME);
  strncpy(ch->familyname, str, MAX_CHAR_NAME - 1);

#ifdef DEBUGGING
  vflog("character", "Got family name (%s)", ch->familyname);
#endif

  /* ask for the gender of the character */
  stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                         "Pick your gender");
  stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                     get_config_message(messages_message,
                                                        "pick_gender"));
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);
  u->fn_timer = create_timeout;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_input = got_gender;

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void got_gender(user * u, char *str)
{
  character *ch = u->current_char;
  unsigned char *oldstack = stack;

  if (!str || (str && !*str))
  {
    stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                           "Pick your gender");
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       get_config_message(messages_message,
                                                          "pick_gender"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);
    u->fn_timer = create_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_gender;

    tell_user(u, (char *)oldstack);
    stack = oldstack;
    return;
  }

  switch (*str)
  {
  case 'n':
  case 'N':
    ch->gender = 0;
#ifdef DEBUGGING
    vflog("character", "Got gender neuter");
#endif
    break;

  case 'm':
  case 'M':
    ch->gender = 1;
#ifdef DEBUGGING
    vflog("character", "Got gender male");
#endif
    break;

  case 'f':
  case 'F':
    ch->gender = 2;
#ifdef DEBUGGING
    vflog("character", "Got gender female");
#endif
    break;

  default:
    stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                           "Pick your gender");
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       get_config_message(messages_message,
                                                          "pick_gender"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);
    u->fn_timer = create_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_gender;

    tell_user(u, (char *)oldstack);
    stack = oldstack;
    return;
  }

  /* ask for the alignment of the character */
  stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                         "Pick your alignment");
  stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                     get_config_message(messages_message,
                                                        "pick_align"));
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);
  u->fn_timer = create_timeout;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_input = got_alignment;

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void got_alignment(user * u, char *str)
{
  character *ch = u->current_char;
  unsigned char *oldstack = stack;

  if (!str || (str && !*str))
  {
    stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                           "Pick your alignment");
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       get_config_message(messages_message,
                                                          "pick_align"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);
    u->fn_timer = create_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_alignment;

    tell_user(u, (char *)oldstack);
    stack = oldstack;

    return;
  }

  switch (*str)
  {
  case 'g':
  case 'G':
    ch->align = 500;
#ifdef DEBUGGING
    vflog("character", "Got alignment good");
#endif

    break;

  case 'n':
  case 'N':
    ch->align = 0;
#ifdef DEBUGGING
    vflog("character", "Got alignment neutral");
#endif
    break;

  case 'e':
  case 'E':
    ch->align = -500;
#ifdef DEBUGGING
    vflog("character", "Got alignment evil");
#endif
    break;

  default:
    stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                           "Pick your alignment");
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       get_config_message(messages_message,
                                                          "pick_align"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);
    u->fn_timer = create_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_alignment;

    tell_user(u, (char *)oldstack);
    stack = oldstack;

    return;
  }

  stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                         "Pick your ethos");
  stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                     get_config_message(messages_message,
                                                        "pick_ethos"));
  stack = (unsigned char *)add_line(u, (char *)stack, end_string);
  u->fn_timer = create_timeout;
  u->count_timer = 3 * ONE_MINUTE;
  u->fn_input = got_ethos;

  tell_user(u, (char *)oldstack);
  stack = oldstack;
}

void got_ethos(user * u, char *str)
{
  character *ch = u->current_char;
  unsigned char *oldstack = stack;

  if (!str || (str && !*str))
  {
    stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                           "Pick your ethos");
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       get_config_message(messages_message,
                                                          "pick_ethos"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);
    u->fn_timer = create_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_ethos;

    tell_user(u, (char *)oldstack);
    stack = oldstack;
    tell_user(u, "got_ethos failed to recognize entered string\n");

    return;
  }

  switch (*str)
  {
  case 'l':
  case 'L':
    ch->ethos = 500;
#ifdef DEBUGGING
    vflog("character", "Got ethos lawful");
#endif
    break;

  case 'n':
  case 'N':
    ch->ethos = 0;
#ifdef DEBUGGING
    vflog("character", "Got ethos neutral");
#endif
    break;

  case 'c':
  case 'C':
    ch->ethos = -500;
#ifdef DEBUGGING
    vflog("character", "Got ethos chaotic");
#endif
    break;

  default:
    stack = (unsigned char *)add_line_text(u, (char *)stack, add_string,
                                           "Pick your ethos");
    stack = (unsigned char *)strprintf((char *)stack, add_string, "%s",
                                       get_config_message(messages_message,
                                                          "pick_ethos"));
    stack = (unsigned char *)add_line(u, (char *)stack, end_string);
    u->fn_timer = create_timeout;
    u->count_timer = 3 * ONE_MINUTE;
    u->fn_input = got_ethos;

    tell_user(u, (char *)oldstack);
    stack = oldstack;

    return;
  }

  add_char_to_user(u, ch);
  save_characters(u);
}

void add_char_to_user(user * u, character * ch)
{
  TESTVARP(u);

  if (!ch)
  {
    if (u)
      ch = u->current_char;
  }

  if (u && ch)
  {
    u->fn_input = null_ptr;
    if (user_hardcoded(u) && u->user_chars == null_ptr)
      ch->level = atoi(get_config_message(config_message, "hc_level"));
    ch->user_next = u->user_chars;
    u->user_chars = ch;
  }
  else
  {
    if (u)
      vsend_to_debug("%s - character non-existant", u->username);
    else
      vsend_to_debug("%s %s - user non-existant", ch->firstname,
                     ch->familyname);
  }
}
