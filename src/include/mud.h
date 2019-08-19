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

#ifdef __PROTO_H
#ifndef __MUD_H
#define __MUD_H

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "common.h"
#include "user.h"
#include "character.h"
#include "room.h"
#include "area.h"
#include "types.h"
#include "pager.h"
#include "class.h"
#include "race.h"
#include "realm.h"

extern unsigned char *stack_start, *stack;
extern char *action;
extern room *current_room;
extern user *users_list;
extern area *areas_list;
extern character *characters_list;
extern int32 max_users, current_users;
extern int32 intermud_port;
extern terminal terms[];

/* afiles.c */
extern void load_area(char *);
extern void load_areas(void);

/* cfiles.c */
extern void create_char_dirs(void);
extern character *load_char(user *, char *);
extern void save_char(user *, character *);
extern void scan_chars(void);

/* character.c */
extern void add_char_to_user(user *, character *);
extern void create_character(user *);
extern void create_timeout(user *);
extern void got_alignment(user *, char *);
extern void got_class(user *, char *);
extern void got_ethos(user *, char *);
extern void got_familyname(user *, char *);
extern void got_firstname(user *, char *);
extern void got_gender(user *, char *);
extern void got_race(user *, char *);
extern void load_characters(user *);
extern int32 is_hero(character *);
extern int32 is_immortal(character *);
extern int32 max_characters(user *);
extern int32 user_has_hero(user *);
extern int32 user_has_immortal(user *);
extern void save_characters(user *);

/* class.c */
extern class *class_load(user *, char *);
extern void class_save(user *, class *);

/* clist.c */

/* commands.c */

/* compression.c */
extern unsigned char *get_int32(int32 *, unsigned char *);
extern unsigned char *get_int32_safe(int32 *, unsigned char *, file);
extern unsigned char *get_int32_safe_explicit(int32 *, char *,
                                              const char *, size_t);
extern unsigned char *get_int64(int64 *, unsigned char *);
extern unsigned char *get_int64_safe(int64 *, unsigned char *, file);
extern unsigned char *get_int64_safe_explicit(int64 *, char *,
                                              const char *, size_t);
extern unsigned char *get_nibble(int32 *, unsigned char *);
extern unsigned char *get_string(char *, unsigned char *);
extern unsigned char *get_string_safe(char *, unsigned char *, file);
extern unsigned char *get_word(pint *, char *);
extern unsigned char *store_int32(unsigned char *, int32);
extern unsigned char *store_int64(unsigned char *, int64);
extern unsigned char *store_nibble(unsigned char *, int32);
extern unsigned char *store_string(unsigned char *, char *);
extern unsigned char *store_word(unsigned char *, int32);

/* connect.c */
extern void connect_to_mud(user *);
extern void first_enter(user *, char *);
extern void got_hc_pass(user *, char *);
extern void got_login_name(user *, char *);
extern void got_new_name(user *, char *);
extern void got_new_password_1(user *, char *);
extern void got_new_password_2(user *, char *);
extern void got_password(user *, char *);
extern void link_to_program(user *);
extern char *list_character_options(user *, char *);
extern void login_timeout(user *);
extern void new_user_start(user *, char *);
extern void pick_character(user *);
extern void send_random_connect_screen(user *);
extern void selected_character(user *, char *);
extern void want_color(user *, char *);

/* main.c */
extern void boot(int32);
extern void close_down(void);
extern void handle_error(char *);
extern void log_pid(void);

/* pager.c */
extern void back_page(user *, pager *);
extern int32 draw_page(user *, char *);
extern void forward_page(user *, pager *);
extern void pager_fn(user *, char *);
extern void quit_pager(user *, pager *);
extern void tell_user_paged(user *, char *);

/* parse.c */
extern void actual_timer(int32);
extern int32 do_match(char *, command *);
extern int32 do_match_exact(char *, command *);
extern char *do_match_modify(char *, command *);
extern void increase_time(realm_data *);
extern void init_commands(void);
extern void input_for_one(user *);
extern char *list_matched_commands(char *, command *);
extern void match_command(user *, char *);
extern void match_command_real(user *, char *, command *);
extern char *next_space(char *);
extern void process_users(void);
extern void timer_function(void);
extern void timing_function(int32);

/* race.c */
extern void race_save(user *, race *);
extern race *race_load(user *, char *);

/* rfiles.c */

/* scripts.c */
extern void lua_runscript(user *, char *);

/* signals.c */
extern void sigbus(int32);
extern void sigchld(int32);
extern void sigfpe(int32);
extern void sighup(int32);
extern void sigill(int32);
extern void sigpipe(int32);
extern void sigquit(int32);
extern void sigsegv(int32);
extern void sigsys(int32);
extern void sigterm(int32);
extern void sigusr1(int32);
extern void sigusr2(int32);
extern void sigxfsz(int32);

/* socket.c */
extern void accept_new_connection(void);
extern void alive_connect(void);
extern void backspace(user *);
extern void do_alive_ping(void);
extern void do_prompt(user *);
extern void get_user_input(user *);
extern void init_socket(int32);
extern void password_mode_off(user *);
extern void password_mode_on(user *);
extern int32 scan_sockets(void);
extern void telnet_options(user *);
extern void telnet_sb_telopt_naws(user *);
extern void telnet_sb_telopt_ttype(user *);

/* string.c */
extern unsigned char *add_line(user *, char *, string_func *);
extern unsigned char *add_line_text(user *, char *, string_func *, char *);
extern unsigned char *vadd_line_text(user *, char *, string_func *,
                                     char *, ...);

/* ufiles.c */
extern void create_user_dirs(void);
extern void load_user(user *);
extern void load_user_characters(user *, char *);
extern user *load_user_name(char *);
extern void save_user(user *);
extern void save_user_characters(user *, char *);
extern file save_user_data(user *);
extern void scan_users(void);
extern int32 user_exists(user *);
extern int32 user_exists_name(char *);

/* user.c */
extern user *create_user(void);
extern void got_curr_password(user *, char *);
extern void got_new_password_change_1(user *, char *);
extern void got_new_password_change_2(user *, char *);
extern void password(user *, char *);
extern void quit_user(user *);
extern int32 user_hardcoded(user *);

/* version.c */
extern void get_hardware_info(void);
extern void version_base(user *);
extern void version_full(user *);
extern void version_short(user *);

#endif                          /* __MUD_H */
#else                           /* __PROTO_H */
#error "Include proto.h instead of mud.h"
#endif                          /* __PROTO_H */
