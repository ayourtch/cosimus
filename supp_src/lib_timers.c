
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

#include <stdint.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "lib_debug.h"
#include "lib_lists.h"
#include "lib_dbuf.h"
#include "lib_timers.h"
#include "lib_uuid.h"

/**
 * @defgroup timermisc Miscellaneous time functions
 */

int startup_cookie;
uint64_t startup_timestamp = 0;

void
startup_note()
{
  startup_timestamp = get_time_msec();
}

uint64_t
get_uptime_msec()
{
  return get_time_msec() - startup_timestamp;
}

int
restarted(int cookie)
{
  return (cookie != startup_cookie);
}


/*@}*/

/**
 * @defgroup timer Timer wheel infrastructure
 */

/*@{*/

/**
 * the size of the timer wheel in ticks and number of ticks per second
 */
enum {
  TIMER_WHEEL_SIZE = 10000,
  TIMER_WHEEL_TICKS_PER_SEC = 100,
};

/**
 * timer list dbuf parameters - initial size, and growth delta
 */
enum {
  TIMER_DBUF_SIZE = 5,
  TIMER_DBUF_INCREMENT = 5,
};

/**
 * timer wheel - index changes every 1/TIMER_WHEEL_TICKS_PER_SEC'th of a second
 */
listitem_t *timerwheel[TIMER_WHEEL_SIZE];

/**
 * current index into timerwheel
 */
int timerwheel_idx = 0;

typedef struct {
  /*
     when is this entry supposed to be fired next time
     * tv_sec + 1000*tv_usec 
   */
  uint32_t time;
  /*
     period for the recurring timers 
   */
  uint32_t period;
  /*
     state - recurring, onetime or inactive 
   */
  uint8_t state;
  /*
     timer generation - incremented each time 
     * the timer slot is reused for some other timer 
   */
  uint8_t generation;
  /*
     timerwheel index 
   */
  uint32_t wheel_idx;
  /*
     list item ptr in timer wheel 
   */
  listitem_t *wheel_li;
  /*
     user data or the pointer to the next inactive timer 
   */
  struct {
    /*
       callback function to call 
     */
    timer_callback_t callback;
    /*
       integer parameter to pass to callback 
     */
    uint32_t param_int;
    /* UUID to pass to callback */

    uuid_t param_uuid;
  
    /*
       parameter to pass to the callback function 
     */
    void *param_dbuf;
  } user;
  /*
     in case the timer is inactive, we use this 
     * to store the index of the next inactive timer
   */
  uint32_t next_inactive_index;
} timerwheel_entry_t;

/**
 * the array holding all the timers.
 */

dbuf_t *timers_list = NULL;

/**
 * First inactive timer in the list, 0 = unused
 */
uint32_t timer_first_inactive = 0;

/**
 * Last inactive timer in the list, 0 = unused
 */
uint32_t timer_last_inactive = 0;

/**
 * return the current length of the timer list
 */

int
timer_list_length()
{
  if(timers_list == NULL) {
    return 0;
  } else {
    return timers_list->size / sizeof(timerwheel_entry_t);
  }
}

void
check_timer_index(uint32_t idx)
{
  if(idx >= timer_list_length()) {
    debug(DBG_TIMERS, 0, "Timer index %d out of bounds", idx);
    print_backtrace();
    assert(idx < timer_list_length());
  }
}

/**
 * deactivate the timer (it should have been already dequeued!)
 * @param idx index of the timer
 */
void
timer_deactivate_internal(uint32_t idx)
{
  timerwheel_entry_t *timers = (timerwheel_entry_t *) timers_list->buf;

  assert(!lbelongs(&timerwheel[timers[idx].wheel_idx], timers[idx].wheel_li));
  check_timer_index(idx);
  if(timers[idx].user.param_dbuf != NULL) {
    dunlock(timers[idx].user.param_dbuf);
  }
  timers[idx].user.param_dbuf = NULL;
  timers[idx].user.param_int = 0;
  memset(&timers[idx].user.param_uuid, 0, sizeof(timers[idx].user.param_uuid));
  timers[idx].state = TIMER_INACTIVE;
  assert(timers[timer_last_inactive].state == TIMER_INACTIVE);
  timers[timer_last_inactive].next_inactive_index = idx;
  timer_last_inactive = idx;
  timers[idx].next_inactive_index = 0;
}

/** 
 * enqueue the timer onto the timer wheel. All the flags in it should be set.
 */

void
timer_enqueue_internal(uint32_t timer_idx, uint32_t msec)
{
  timerwheel_entry_t *timers = (timerwheel_entry_t *) timers_list->buf;
  uint32_t ticks = (msec * TIMER_WHEEL_TICKS_PER_SEC / 1000);
  uint32_t next_wheel_idx = (timerwheel_idx + ticks) % TIMER_WHEEL_SIZE;

  if(ticks >= TIMER_WHEEL_SIZE) {
    debug(DBG_TIMERS, 0, "%d ticks is bigger than the wheel size", ticks);
    return;
  }
  debug(DBG_TIMERS, 10, "enqueue index %d with interval %d", timer_idx, msec);
  debug(DBG_TIMERS, 10, "current tw index: %d, ticks from it: %d",
        timerwheel_idx, ticks);
  // These two fields are primarily used to remove the timer beforehand,
  // but rpush is important :)
  timers[timer_idx].wheel_li =
    lpush(&timerwheel[next_wheel_idx], (void *) ((long int) timer_idx));
  timers[timer_idx].wheel_idx = next_wheel_idx;
  debug(DBG_TIMERS, 10, "Pushed timer index %d onto wheel index %d",
        timer_idx, next_wheel_idx);
}

/**
 * Delete the timer, assuming it has not fired yet. WARNING: no parameter checking!
 *
 * @param timer_idx the index of the timer within the timer list
 */

void
timer_delete_internal(uint32_t timer_idx)
{
  timerwheel_entry_t *timers = (timerwheel_entry_t *) timers_list->buf;

  debug(DBG_TIMERS, 10, "timer_delete_internal: %d, list: %x, li: %x",
        timer_idx, timerwheel[timers[timer_idx].wheel_idx],
        timers[timer_idx].wheel_li);
  check_timer_index(timer_idx);

  if(!lbelongs(&timerwheel[timers[timer_idx].wheel_idx],
               timers[timer_idx].wheel_li)) {
    // delete the timer from the list 
    assert("The timer is probably being deleted twice" == NULL);
  }
  ldelete(&timerwheel[timers[timer_idx].wheel_idx],
          timers[timer_idx].wheel_li);
  timers[timer_idx].wheel_li = NULL;
  timers[timer_idx].wheel_idx = 0;
}

void
fire_timer(uint32_t idx)
{
  timerwheel_entry_t *timers = (timerwheel_entry_t *) timers_list->buf;
  uint32_t timer_id = idx + (timers[idx].generation << 24);

  // as we are no longer on the list, prevent any erroneous 
  // attempt to delete us from the list by stopping the timer from within handler
  timers[idx].wheel_li = NULL;
  timers[idx].wheel_idx = 0;

  switch (timers[idx].state) {
  case TIMER_ONETIME:
  case TIMER_PERIODIC:
    // Fall through for the two cases
    break;
  case TIMER_INACTIVE:
    debug(DBG_TIMERS, 0, "Attempt to fire an inactive timer %d!", idx);
    assert(0);
    return;
  default:
    debug(DBG_TIMERS, 0, "Invalid timer state %d for timer %x",
          timers[idx].state, timer_id);
    return;
  }
  if(timers[idx].user.callback != NULL) {
    timers[idx].user.callback(timer_id, timers[idx].user.param_int, &timers[idx].user.param_uuid,
                              timers[idx].user.param_dbuf);
  }
  debug(DBG_TIMERS, 3, "Timer %x (%d:%d) fired at wheel index %d",
        timer_id, idx, timers[idx].generation, timerwheel_idx);
  switch (timers[idx].state) {
  case TIMER_ONETIME:
    debug(DBG_TIMERS, 1, "Timer %x onetime, deactivate", timer_id);
    timer_deactivate_internal(idx);
    break;
  case TIMER_PERIODIC:
    debug(DBG_TIMERS, 2, "Timer %x periodic with interval %d, reenqueue",
          timer_id, timers[idx].period);
    timer_enqueue_internal(idx, timers[idx].period);
    break;
  case TIMER_INACTIVE:
    debug(DBG_TIMERS, 1, "Timer %x probably stopped from within the handler",
          timer_id);
    break;
  default:
    debug(DBG_TIMERS, 0, "Invalid after-fire timer state %d for timer %x",
          timers[idx].state, timer_id);
  }
}

/**
 * when did the last timer tick happen
 */

uint64_t last_timer_tick_time = 0;



/**
 * the function to be called with each tick
 */
void
timer_tick_processor(void)
{
  uint32_t timer_idx;

  while((timer_idx = (uint32_t) ((long int)lpop(&timerwheel[timerwheel_idx]))) != 0) {
    fire_timer(timer_idx);
  }
  if(++timerwheel_idx >= TIMER_WHEEL_SIZE) {
    timerwheel_idx = 0;
  }
}

/**
 * initialization routine to be called in the very beginning
 */
int
init_timers(void)
{
  timers_list = dalloczf(sizeof(timerwheel_entry_t) * TIMER_DBUF_SIZE);
  if(timers_list == NULL) {
    return 0;
  } else {
    int i;
    int size = timer_list_length();
    timerwheel_entry_t *timers = (timerwheel_entry_t *) timers_list->buf;

    for(i = 1; i < size; i++) {
      timers[i].state = TIMER_INACTIVE;
      timers[i].generation = 0;
      timers[i].next_inactive_index = i + 1;
    }
    timers[size - 1].next_inactive_index = 0;
    timer_first_inactive = 1;
    timer_last_inactive = size - 1;
    return 1;
  }
}

int
expand_timer_list(void)
{
  int oldsize = timer_list_length();
  int newsize;
  int i;

  if(timer_first_inactive != timer_last_inactive) {
    debug(DBG_TIMERS, 0,
          "Inappropriate call to expand_timer_list: first %d, last %d",
          timer_first_inactive, timer_last_inactive);
    return 0;
  }
  if(dgrow(timers_list, sizeof(timerwheel_entry_t) * TIMER_DBUF_INCREMENT)) {
    timers_list->dsize = timers_list->size;
    timerwheel_entry_t *timers = (timerwheel_entry_t *) timers_list->buf;

    newsize = timer_list_length();
    for(i = oldsize; i < newsize; i++) {
      timers[i].state = TIMER_INACTIVE;
      timers[i].generation = 0;
      timers[i].next_inactive_index = i + 1;
      debug(DBG_TIMERS, 10, "Set next inactive index for %d to %d", i, i + 1);
    }
    timers[newsize - 1].next_inactive_index = 0;
    timers[timer_last_inactive].next_inactive_index = oldsize;
    debug(DBG_TIMERS, 10, "Set next inactive index for %d to %d",
          timer_last_inactive, oldsize);
    timer_last_inactive = newsize - 1;
    debug(DBG_TIMERS, 10, "Grown the timer list, new first: %d, last: %d",
          timer_first_inactive, timer_last_inactive);
    return 1;
  } else {
    return 0;
  }
}

/**
 * Acquire the next inactive timer index
 * and expand the timer list if necessary
 */
uint32_t
grab_timer_index(void)
{
  uint32_t mytimer = 0;
  timerwheel_entry_t *timers;

  if(timers_list == NULL) {
    debug(DBG_TIMERS, 0, "Can not start timer - timer list not initialized");
    return 0;
  } else {
    timers = (timerwheel_entry_t *) timers_list->buf;
    mytimer = timer_first_inactive;
    debug(DBG_TIMERS, 1, "Grabbed new timer index: %d", mytimer);
    if(timers[mytimer].next_inactive_index == 0) {
      // this means we've grabbed last inactive timer - need more
      if(expand_timer_list()) {
        // The pointer to the buffer was changed
        timers = (timerwheel_entry_t *) timers_list->buf;
        timer_first_inactive = timers[mytimer].next_inactive_index;
        debug(DBG_TIMERS, 10, "New first inactive index[%d] after grow: %d",
              mytimer, timer_first_inactive);
      } else {
        debug(DBG_TIMERS, 0, "Could not expand timer list");
        return 0;
      }
    } else {
      timer_first_inactive = timers[mytimer].next_inactive_index;
      debug(DBG_TIMERS, 10, "New first inactive index: %d (taken from %d)",
            timer_first_inactive, mytimer);
    }
    assert(timers[mytimer].state == TIMER_INACTIVE);
    return mytimer;
  }
}

/**
 * start new timer with the given parameters
 */

uint32_t
start_timer(int interval, int timer_kind, timer_callback_t callback,
            uint32_t param_int, uuid_t *param_uuid, void *param_dbuf)
{
  uint32_t mytimer = grab_timer_index();
  uint32_t out;
  timerwheel_entry_t *timers;

  if(mytimer == 0) {
    return 0;
  }
  timers = (timerwheel_entry_t *) timers_list->buf;
  if(param_dbuf != NULL) {
    dlock(param_dbuf);
  }
  timers[mytimer].state = timer_kind;
  // increment the generation - it will automatically overflow when needed
  timers[mytimer].generation++;
  timers[mytimer].user.callback = callback;
  timers[mytimer].user.param_int = param_int;
  if (param_uuid) {
    memcpy(&timers[mytimer].user.param_uuid, param_uuid, sizeof(timers[mytimer].user.param_uuid));
  } else {
    memset(&timers[mytimer].user.param_uuid, 0, sizeof(timers[mytimer].user.param_uuid));
  }

  timers[mytimer].user.param_dbuf = param_dbuf;
  if(timer_kind == TIMER_PERIODIC) {
    timers[mytimer].period = interval;
  }
  timer_enqueue_internal(mytimer, interval);
  out = (timers[mytimer].generation << 24) + mytimer;
  debug(DBG_TIMERS, 10, "Started timer %x of kind %d with interval %d",
        out, timer_kind, interval);
  return out;
}

int
stop_timer(uint32_t timer_id)
{
  timerwheel_entry_t *timers = (timerwheel_entry_t *) timers_list->buf;
  uint32_t idx = timer_id & 0xffffff;
  uint32_t gen = timer_id >> 24;
  uint32_t size = timer_list_length();

  if(idx == 0 || idx >= size) {
    debug(DBG_TIMERS, 0, "Invalid index %d (size is %d)", idx, size);
    return 0;
  }
  if(timers[idx].generation != gen) {
    // possibly a stale timer - that was reallocated for
    // something else
    debug(DBG_TIMERS, 0, "Index %d generation %d is different from %d",
          idx, timers[idx].generation, gen);
    return 0;
  }
  switch (timers[idx].state) {
  case TIMER_PERIODIC:
  case TIMER_ONETIME:
    if(timers[idx].wheel_li != NULL) {
      timer_delete_internal(idx);
    }
    timer_deactivate_internal(idx);
    return 1;
    break;
  case TIMER_INACTIVE:
    debug(DBG_TIMERS, 0, "%d: stopping timer that is already stopped",
          timer_id);
    break;
  default:
    debug(DBG_TIMERS, 0, "%d: inconsistent timer state %x", timer_id,
          timers[idx].state);
  }
  return 0;
}


/**
 * return the current time as a long int
 */

uint64_t
get_time_msec(void)
{
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return (((uint64_t) 1000000) * (uint64_t) tv.tv_sec +
          (uint64_t) tv.tv_usec) / (uint64_t) 1000;
}


/**
 * Check if it is the time to fire up the timers, if yes - fire them,
 * and return the timeout value in msec till the next tick,
 * for use in poll()
 */

int
check_timers(void)
{
  uint64_t now_msec = get_time_msec();
  uint64_t delta = now_msec - last_timer_tick_time;
  uint64_t interval = (1000 / TIMER_WHEEL_TICKS_PER_SEC);

  debug(DBG_TIMERS, 20, "Delta: %lu, interval: %lu, now: %lu",
        delta, interval, now_msec);
  if(delta >= interval) {
    timer_tick_processor();
    last_timer_tick_time = now_msec;
    delta = interval;
  } else {
    delta = interval - delta;
  }
  debug(DBG_TIMERS, 20, "Result delta: %lu", delta);
  return delta;
}


/*@}*/




