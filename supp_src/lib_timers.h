
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

#ifndef __LIB_TIMERS_H__
#define __LIB_TIMERS_H__

#include "lib_uuid.h"
#include <stdint.h>

void startup_note();
uint64_t get_uptime_msec();
int restarted(int cookie);

typedef void (*timer_callback_t) (uint32_t timer_id, int param_int, uuid_t *param_uuid,
                                  void *param_dbuf);

/**
 *  * Status of the timer
 *   */
enum {
  TIMER_INACTIVE = 0,
  TIMER_ONETIME,
  TIMER_PERIODIC,
};


uint32_t start_timer(int interval, int timer_kind,
                     timer_callback_t callback, uint32_t int_param, uuid_t *uuid_param,
                     void *dbuf_param);

int stop_timer(uint32_t timer_id);

uint64_t get_time_msec(void);

int init_timers(void);
int check_timers(void);



#endif
