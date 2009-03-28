
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

#include "lib_poll.h"

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

