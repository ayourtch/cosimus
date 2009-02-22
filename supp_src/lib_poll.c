#include "lib_poll.h"

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



#ifndef HAS_POLL_H

int
poll(struct pollfd *ufds, int nfds, int timeout)
{
  struct timeval tv;
  fd_set readfds;
  fd_set writefds;
  fd_set exceptfds;
  int maxfd = 0;
  int ret;
  int i;

  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
  FD_ZERO(&exceptfds);

  for(i = 0; i < nfds; i++) {
    if(ufds[i].events & POLLIN) {
      FD_SET(ufds[i].fd, &readfds);
    }
    if(ufds[i].events & POLLOUT) {
      FD_SET(ufds[i].fd, &writefds);
    }
    FD_SET(ufds[i].fd, &exceptfds);
    if(ufds[i].fd + 1 > maxfd) {
      maxfd = ufds[i].fd + 1;
    }
  }

  tv.tv_usec = (timeout % 1000) * 1000;
  tv.tv_sec = timeout / 1000;

  ret = select(maxfd, &readfds, &writefds, &exceptfds, &tv);
  for(i = 0; i < nfds; i++) {
    ufds[i].revents = 0;
    if(FD_ISSET(ufds[i].fd, &readfds)) {
      ufds[i].revents |= POLLIN;
    }
    if(FD_ISSET(ufds[i].fd, &writefds)) {
      ufds[i].revents |= POLLOUT;
    }
    if(FD_ISSET(ufds[i].fd, &exceptfds)) {
      ufds[i].revents |= POLLHUP | POLLERR;     // ???
    }
  }

  return ret;

}

#endif /* has poll */

