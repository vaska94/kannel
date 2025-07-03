/* ==================================================================== 
 * The Kannel Software License, Version 1.0 
 * 
 * Copyright (c) 2001-2018 Kannel Group  
 * Copyright (c) 1998-2001 WapIT Ltd.   
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in 
 *    the documentation and/or other materials provided with the 
 *    distribution. 
 * 
 * 3. The end-user documentation included with the redistribution, 
 *    if any, must include the following acknowledgment: 
 *       "This product includes software developed by the 
 *        Kannel Group (http://www.kannel.org/)." 
 *    Alternately, this acknowledgment may appear in the software itself, 
 *    if and wherever such third-party acknowledgments normally appear. 
 * 
 * 4. The names "Kannel" and "Kannel Group" must not be used to 
 *    endorse or promote products derived from this software without 
 *    prior written permission. For written permission, please  
 *    contact org@kannel.org. 
 * 
 * 5. Products derived from this software may not be called "Kannel", 
 *    nor may "Kannel" appear in their name, without prior written 
 *    permission of the Kannel Group. 
 * 
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED.  IN NO EVENT SHALL THE KANNEL GROUP OR ITS CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,  
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR  
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,  
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE  
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 * ==================================================================== 
 * 
 * This software consists of voluntary contributions made by many 
 * individuals on behalf of the Kannel Group.  For more information on  
 * the Kannel Group, please see <http://www.kannel.org/>. 
 * 
 * Portions of this software are based upon software originally written at  
 * WapIT Ltd., Helsinki, Finland for the Kannel project.  
 */ 

/*
 * test_timerset.c - test Timerset objects
 *
 * Stipe Tolj <stolj at kannel.org>
 */

#include "gwlib/gwlib.h"
#include "gwlib/gw-timer.h"
#include "gw/msg.h"

#define MAX_RETRY   3
#define FREQ        10

typedef struct TimerItem {
    Timer *timer;
    Msg *msg;
} TimerItem;


static Timerset *timerset = NULL;

static List *q_timer = NULL;
static List *q_retry = NULL;


/*
 * This threads consumes the TimerItem events that the Timerset issues to the
 * defined queue. In needs to use the lock-enabled calls, since it is an own
 * thread context.
 */
static void timer_consumer_thread(void *arg)
{
    List *q = arg;
    TimerItem *i;

    while ((i = gwlist_consume(q)) != NULL) {
        if (++(i->msg->sms.resend_try) <= MAX_RETRY) {
            gw_timer_elapsed_start(i->timer, FREQ, i);
            debug("",0,"%s: Msg %p retry %ld", __func__,
                  i->msg, i->msg->sms.resend_try);
        } else {
            warning(0,"%s: Msg %p exceeded %d retries, discard!", __func__,
                    i->msg, MAX_RETRY);
            msg_destroy(i->msg);
            gw_timer_elapsed_destroy(i->timer);
            gw_free(i);
        }
    }
}


/*
 * The callback function used by the Timerset when the event elapses. In case
 * the timer is re-started within this callback function, then the
 * gw_timer_elapsed_start_cb() needs to be used, otherwise we end-up in a
 * dead-lock, due that the callback function runs in a thread that has the lock
 * set already.
 */
static void timer_retry_cb(void *arg)
{
    TimerItem *i = arg;

    gwlist_produce(q_retry, i->msg);

    gw_timer_elapsed_destroy_cb(i->timer);
    gw_free(i);
}


/*
 * The above timer_retry_cb() feeds a separate queue, which is consumed by
 * this own threads, therefore we need to used the locking-enabled
 * gw_timer_elapsed_start() to re-start a timer.
 */
static void retry_consumer_thread(void *arg)
{
    List *q = arg;
    Msg *msg;
    TimerItem *i;

    while ((msg = gwlist_consume(q)) != NULL) {
        if (++(msg->sms.resend_try) <= MAX_RETRY) {
            i = gw_malloc(sizeof(TimerItem));
            i->msg = msg;
            i->timer = gw_timer_create(timerset, NULL, timer_retry_cb);
            gw_timer_elapsed_start(i->timer, FREQ, i);
            debug("",0,"%s: Msg %p retry %ld", __func__,
                  msg, msg->sms.resend_try);
        } else {
            warning(0,"%s: Msg %p exceeded %d retries, discard!", __func__,
                    msg, MAX_RETRY);
            msg_destroy(msg);
        }
    }
}


int main()
{
    long t_timer;
    long t_retry;
    Msg *msg_a, *msg_b;
    TimerItem *i_timer;
    TimerItem *i_retry;

    gwlib_init();

    timerset = gw_timerset_create();

    /* setup timer thread to consume queue */
    q_timer = gwlist_create();
    gwlist_add_producer(q_timer);
    t_timer = gwthread_create(timer_consumer_thread, q_timer);

    /* setup thread to consume retry callback events */
    q_retry = gwlist_create();
    gwlist_add_producer(q_retry);
    t_retry = gwthread_create(retry_consumer_thread, q_retry);

    msg_a = msg_create(sms);
    msg_b = msg_create(sms);

    msg_a->sms.resend_try = msg_b->sms.resend_try = 0;

    i_timer = gw_malloc(sizeof(TimerItem));
    i_timer->msg = msg_a;
    /* timed events are pushed into q_timer queue */
    i_timer->timer = gw_timer_create(timerset, q_timer, NULL);
    gw_timer_elapsed_start(i_timer->timer, FREQ, i_timer);
    debug("",0,"%s: Msg %p retry %ld", __func__, msg_a, msg_a->sms.resend_try);

    i_retry = gw_malloc(sizeof(TimerItem));
    i_retry->msg = msg_b;
    /* timed events trigged the callback function */
    i_retry->timer = gw_timer_create(timerset, NULL, timer_retry_cb);
    gw_timer_elapsed_start(i_retry->timer, FREQ, i_retry);
    debug("",0,"%s: Msg %p retry %ld", __func__, msg_b, msg_b->sms.resend_try);

    debug("",0,"%s: enter main loop", __func__);
    do {
        gwthread_sleep(2);
    } while (gw_timerset_count(timerset) > 0);
    debug("",0,"%s: exit main loop", __func__);

    gw_timerset_destroy(timerset);

    gwlist_remove_producer(q_timer);
    gwthread_join(t_timer);
    gwlist_destroy(q_timer, NULL);

    gwlist_remove_producer(q_retry);
    gwthread_join(t_retry);
    gwlist_destroy(q_retry, NULL);

    gwlib_shutdown();

    return 0;
}
