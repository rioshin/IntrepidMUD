/*
 * IntrepidMUD
 * Header file
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

#ifndef __USER_H
#define __USER_H

#define DEFAULT_TERM_WIDTH	78
#define DEFAULT_TERM_LINES	19

#define MAX_USER_NAME		40
#define MAX_PASSWORD		40
#define MAX_INET_ADDR		80
#define MAX_INPUT_BUFFER	512
#define MAX_PROMPT		64
#define MAX_TTYPE		20
#define MAX_TITLE		40

#include "types.h"
#include "character.h"
#include "pager.h"

#define INPUT_READY		(1<<0)
#define INPUT_LAST_R		(1<<1)
#define INPUT_LAST_N		(1<<2)

#define CONN_IAC_GA		(1<<0)
#define CONN_DEFAULT_IAC_GA	(1<<1)
#define CONN_IAC_EOR		(1<<2)
#define CONN_PASSWORD_MODE	(1<<3)
#define CONN_DO_LOCAL_ECHO	(1<<4)
#define FAILED_COMMAND          (1<<5)

/* bits for residency */
#define HCADMIN			(1<<0)
#define ADMIN			(1<<1)
#define LOWER_ADMIN		(1<<2)
#define ADVANCED_SU		(1<<3)
#define SU			(1<<4)

#define TERM_NONE		0
#define TERM_XTERM		1
#define TERM_VT220		2
#define TERM_VT100		3
#define TERM_VT102		4
#define TERM_ANSI		5
#define TERM_WYSE30		6
#define TERM_TVI912		7
#define TERM_SUN		8
#define TERM_ADM		9
#define TERM_HP2392		10

typedef struct user
{
  /* Information following this needs to be saved */
  char username[MAX_USER_NAME];
  char password[MAX_PASSWORD];
  char normal_prompt[MAX_PROMPT];
  char title[MAX_TITLE];
  int32 default_term_type;
  int32 default_term_width;
  int32 default_term_lines;
  int32 input_flags;
  int32 conn_flags;
  int32 residency;
  int32 time_total_login;
  character *user_chars;

  /* Information following this doesn't need to be saved */
  char special_prompt[MAX_PROMPT];
  char pager_prompt[MAX_PROMPT];
  char temp_prompt[MAX_PROMPT];
  char input_buffer[MAX_INPUT_BUFFER];
  char addr_inet[MAX_INET_ADDR];
  char addr_num[MAX_INET_ADDR];
  char prev_ttype[MAX_TTYPE];
  char new_password[MAX_PASSWORD];
  int32 fd;
  int32 input_pointer;
  int32 logged_in;
  int32 failed_password;
  int32 term_type, detected_term_type, temp_term_type;
  int32 term_width, detected_term_width, temp_term_width;
  int32 term_lines, detected_term_lines, temp_term_lines;
  int32 time_idle;
  int32 time_login;
  character *current_char;
  pager *pager;

  struct user *next;
  struct user *previous;

  command_func *fn_input;
  user_func *fn_timer;
  int32 count_timer;
} user;

#endif                          /* __USER_H */
