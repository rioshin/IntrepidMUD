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

#ifndef __PROTO_H
#define __PROTO_H

#include "angel.h"
#include "archangel.h"
#include "common.h"
#include "mud.h"
/*
#include "ident.h"
#include "dns.h"
#include "intermud.h"
*/

#ifdef NEED_SIGNAL_DECLS

#define __need_timespec
#define __need_siginfo_t

#include <sys/select.h>
#include <signal.h>
#if __GNUC__ < 7
# include <bits/siginfo.h>
#else
# include <bits/types/siginfo_t.h>
#endif
#include <bits/stat.h>
#include <time.h>
#if __GNUC__ == 7
# include <bits/types/struct_timespec.h>
#endif

typedef __sigset_t sigset_t;

struct sigaction
{
  void (*sa_handler) (int);
  void (*sa_sigaction) (int, siginfo_t *, void *);
  sigset_t sa_mask;
  int sa_flags;
  void (*sa_restorer) (void);
};

extern int sigaction(int, const struct sigaction *, struct sigaction *);
extern int sigpending(sigset_t *);
extern int sigprocmask(int, const sigset_t, sigset_t *);
extern int sigsuspend(const sigset_t *);
extern int sigtimedwait(const sigset_t *, siginfo_t *,
                        const struct timespec *);
extern int sigwait(const sigset_t *, int);
extern int sigwaitinfo(const sigset_t *, siginfo_t *);

extern int sigemptyset(sigset_t *);
extern int sigfillset(sigset_t *);
extern int sigaddset(sigset_t *, int);
extern int sigdelset(sigset_t *, int);
extern int sigismember(const sigset_t *, int);
extern int sigisemptyset(const sigset_t *);
extern int sigandset(sigset_t *, const sigset_t *, const sigset_t *);
extern int sigorset(sigset_t *, const sigset_t *, const sigset_t *);

extern int kill(pid_t, int);

#undef __need_timespec

#endif                          /* NEED_SIGNAL_DECLS */
#endif                          /* __PROTO_H */
