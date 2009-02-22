#ifndef __LIB_POLL_H__
#define __LIB_POLL_H__

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


#define HAS_POLL_H
#ifdef HAS_POLL_H
#include <sys/poll.h>
#else

/* According to POSIX 1003.1-2001 */
#include <sys/select.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


#define POLLIN          0x001   /* There is data to read.  */
#define POLLPRI         0x002   /* There is urgent data to read.  */
#define POLLOUT         0x004   /* Writing now will not block.  */

/* 
 * Event types always implicitly polled for.  These bits need not be set in
 * `events', but they will appear in `revents' to indicate the status of
 *  the file descriptor.  
 */
#define POLLERR         0x008   /* Error condition.  */
#define POLLHUP         0x010   /* Hung up.  */
#define POLLNVAL        0x020   /* Invalid polling request.  */


struct pollfd {
  int fd;                       /* file descriptor */
  short events;                 /* requested events */
  short revents;                /* returned events */
};


int poll(struct pollfd *ufds, int nfds, int timeout);

#endif /* has poll */

#endif /* lib_poll.h */
