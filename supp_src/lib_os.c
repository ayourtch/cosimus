
/*
 * Copyright (c) Andrew Yourtchenko <ayourtch@gmail.com>, 2008-2009
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Cosimus Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/**
 * @defgroup osfunc OS-interaction functions (signals and execution)
 */
#include <stdlib.h>

#include "lib_os.h"
#include "lib_debug.h"

/*@{*/


signal_func *
set_signal_handler(int signo, signal_func * func)
{
  struct sigaction act, oact;

  act.sa_handler = func;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if(sigaction(signo, &act, &oact) < 0)
    return SIG_ERR;

  return oact.sa_handler;
}


void
makedaemon(char *logname)
{
  int uid = 32767;
  int gid = 32767;
  char *user = "nobody";
  struct passwd *pwd;
  int logfd;

  logfd = open(logname, O_WRONLY | O_CREAT | O_APPEND);
  if(logfd < 0) {
    debug(DBG_GLOBAL, 0, "Could not open logfile '%s', exiting", logname);
    exit(255);
  }

  if(getuid() == 0) {
    pwd = getpwnam(user);
    if(pwd) {
      uid = pwd->pw_uid;
      gid = pwd->pw_gid;
    }
    debug(DBG_GLOBAL, 0,
          "Launched as root, trying to become %s (uid %d, gid %d)..",
          user, uid, gid);
    notminus(setgroups(0, (const gid_t *) 0), "setgroups");
    initgroups(user, gid);      // not critical if fails
    notminus(setgid(gid), "setgid");
    notminus(setegid(gid), "setegid");
    notminus(setuid(uid), "setuid");
    notminus(seteuid(gid), "seteuid");
    debug(DBG_GLOBAL, 0, "now forking");
  }


  if(fork() != 0)
    exit(0);

  setsid();
  set_signal_handler(SIGHUP, SIG_IGN);
  set_signal_handler(SIGPIPE, SIG_IGN);

  if(fork() != 0)
    exit(0);

  //chdir("/tmp");
  //chroot("/tmp");
  //umask(077);


  close(0);
  close(1);
  close(2);
  dup2(logfd, 1);
  dup2(logfd, 2);
  debug(DBG_GLOBAL, 0, "Started as a daemon");
}

time_t get_file_mtime(char *fname)
{
  struct stat st;
  if (0 == stat(fname, &st)) {
    return st.st_mtime;
  } else {
    return -1;
  }

}


/*@}*/


