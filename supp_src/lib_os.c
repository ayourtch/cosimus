
/**************************************************************************
*
*  Copyright © 2008-2009 Andrew Yourtchenko, ayourtch@gmail.com.
*
*  Permission is hereby granted, free of charge, to any person obtaining 
* a copy of this software and associated documentation files (the "Software"), 
* to deal in the Software without restriction, including without limitation 
* the rights to use, copy, modify, merge, publish, distribute, sublicense, 
* and/or sell copies of the Software, and to permit persons to whom 
* the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included 
* in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
* OR OTHER DEALINGS IN THE SOFTWARE. 
*
*****************************************************************************/

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


