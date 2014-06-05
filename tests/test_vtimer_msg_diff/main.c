/*
 * Copyright (C) 2013 INRIA
 * Copyright (C) 2014 Kaspar Schleiser
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup tests
 * @{
 *
 * @file
 * @brief Thread test application
 *
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 * @author      Christian Mehlis <mehlis@inf.fu-berlin.de>
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 *
 * @}
 */

#include <stdio.h>
#include <time.h>

#include "vtimer.h"
#include "thread.h"
#include "msg.h"

#define MAXCOUNT 20
#define MAXDIFF 500

char timer_stack[KERNEL_CONF_STACKSIZE_PRINTF];

struct timer_msg {
    vtimer_t timer;
    timex_t interval;
    char *msg;
    timex_t start;
    int count;
};

timex_t now;

struct timer_msg msg_a = { .interval = { .seconds = 2, .microseconds = 0}, .msg = "T1", .start={0}, .count=0 };
struct timer_msg msg_b = { .interval = { .seconds = 3, .microseconds = 0}, .msg = "T2", .start={0}, .count=0 };
struct timer_msg msg_c = { .interval = { .seconds = 5, .microseconds = 0}, .msg = "T3", .start={0}, .count=0 };

void timer_thread(void)
{
    printf("This is thread %d\n", thread_getpid());

    msg_t msgq[16];
    msg_init_queue(msgq, sizeof msgq);

    while (1) {
        msg_t m;
        msg_receive(&m);
        struct timer_msg *tmsg = (struct timer_msg *) m.content.ptr;

        vtimer_now(&now);

        if (tmsg->start.seconds==0 && tmsg->start.microseconds == 0) {
            printf("Initializing \"%s\".\n", tmsg->msg);
            tmsg->start = now;
        } else {
            tmsg->count++;
        }

        uint64_t should = timex_uint64(tmsg->interval)*tmsg->count+timex_uint64(tmsg->start);
        int64_t diff = should - timex_uint64(now);

        printf("now=%02" PRIu32 ":%02" PRIu32 " -> every %" PRIu32 ".%" PRIu32 "s: %s diff=%" PRId64 "\n",
               now.seconds,
               now.microseconds,
               tmsg->interval.seconds,
               tmsg->interval.microseconds,
               tmsg->msg, diff);

        if (diff > MAXDIFF || diff < -MAXDIFF) {
            printf("WARNING: timer difference %" PRId64 "us exceeds MAXDIFF(%d)!\n", diff, MAXDIFF);
        }

        if (vtimer_set_msg(&tmsg->timer, tmsg->interval, thread_getpid(), tmsg) != 0) {
            puts("something went wrong setting a timer");
        }

        if (tmsg->count >= MAXCOUNT) {
            printf("Maximum count reached. (%d) Exiting.\n", MAXCOUNT);
            break;
        }
    }
}

int main(void)
{
    msg_t m;
    int pid = thread_create(
                  timer_stack,
                  sizeof timer_stack,
                  PRIORITY_MAIN - 1,
                  CREATE_STACKTEST,
                  timer_thread,
                  "timer");

    puts("sending 1st msg");
    m.content.ptr = (char *) &msg_a;
    msg_send(&m, pid, false);

    puts("sending 2nd msg");
    m.content.ptr = (char *) &msg_b;
    msg_send(&m, pid, false);
    
    puts("sending 3nd msg");
    m.content.ptr = (char *) &msg_c;
    msg_send(&m, pid, false);

    return 0;
}
