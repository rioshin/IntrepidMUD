/*
 * IntrepidMUD
 * Header file
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

#ifdef __PROTO_H
#ifndef __COMMON_H
#define __COMMON_H

/* DEFINES */
#define TESTVARP(x)	if (!x) \
			  x = null
#define TESTVARV(x)	if (!x) \
			  x = 0

/* TYPE DEFINITIONS */
typedef struct file
{
  char *where;
  int32 length;
} file;

/* INCLUDES */
#include "mud.h"
#include "user.h"

/* VARIABLES */
extern unsigned char *stack;
extern file config_message;
extern file log_message;
extern file messages_message;
extern file prompts_message;
extern user *current_user;

/* FUNCTION PROTOTYPES */
/* common-config.c */
extern char *get_config_message(file, char *);

/* common-debug.c */
extern void send_to_debug(char *);
extern void vsend_to_debug(char *, ...);

/* common-file.c */
extern file load_file(char *);
extern file load_file_verbose(char *, int32);

/* common-log.c */
extern void error(char *);
extern void flog(char *, char *);
extern int32 log_required_priv(char *);
extern void verror(char *, ...);
extern void vflog(char *, char *, ...);

/* common-main.c */
extern int32 init_framework(int32, char *[]);
extern int32 init_root(int32, char *[]);

/* common-socket.c */
extern file process_output(user *, char *);
extern void send_to_user(user *, char *);

/* common-stack.c */
extern unsigned char *create_stack(int32);

/* common-string.c */
extern unsigned char *add_string(unsigned char *);
extern unsigned char *end_string(unsigned char *);
extern char *get_color_code(user *, char);
extern char *get_number(int);
extern char *linestr(char *, char *);
extern char *lineval(char *, char *);
extern char *lower_case(char *);
extern int mud_strcmp(char *, char *);
extern size_t mud_strlen(char *);
extern int mud_strncmp(char *, char *, int);
extern char *strcasestr(char *, char *);
extern char *strprintf(char *, string_func *, char *, ...);
extern char *sys_time(void);
extern ssize_t write_socket(int32 *, const void *, size_t);

/* common-tell.c */
extern void tell_all_users(char *);
extern void tell_all_users_but(user *, char *);
extern void tell_user(user *, char *);
extern void tell_user_normal(user *, char *);
extern void vtell_all_users(char *, ...);
extern void vtell_all_users_but(user *, char *, ...);
extern void vtell_user(user *, char *, ...);

#endif                          /* __COMMON_H */
#else                           /* __PROTO_H */
#error "Include proto.h instead of common.h"
#endif                          /* __PROTO_H */
