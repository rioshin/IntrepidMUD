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
#ifndef __ARCHANGEL_H
#define __ARCHANGEL_H

#include "common.h"

extern unsigned char *stack_start, *stack;
extern int32 fh, die;

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
extern void sigxfsz(int32);
extern int32 handle_angel(int32, char *[]);

#endif                          /* __ARCHANGEL_H */
#else                           /* __PROTO_H */
#error "Include proto.h instead of archangel.h"
#endif                          /* __PROTO_H */
