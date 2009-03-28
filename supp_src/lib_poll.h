
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

#ifndef __LIB_POLL_H__
#define __LIB_POLL_H__

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
