
/*
 * Copyright (c) Contributors, http://cosimus-news.blogspot.com/
 * See CONTRIBUTORS.TXT for a full list of copyright holders.
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
