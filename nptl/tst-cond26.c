/* Copyright (C) 2019 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
static int event = 0;

static void *
signaler (void *arg)
{
  (void)arg;

  int ret = pthread_mutex_lock (&mut);
  if (ret)
    {
      puts("signaler: pthread_mutex_lock failed");
      exit (EXIT_FAILURE);
    }

  event = 1;
  ret = pthread_cond_broadcast (&cond);
  if (ret)
    {
      puts("signaler: pthread_cond_broadcast failed");
      exit (EXIT_FAILURE);
    }

  ret = pthread_mutex_unlock (&mut);
  if (ret)
    {
      puts("signaler: pthread_mutex_unlock failed");
      exit (EXIT_FAILURE);
    }

  return NULL;
}

static int
spawn_signaler (void)
{
  int ret;
  pthread_t thread;
  pthread_attr_t attr;

  ret = pthread_attr_init (&attr);
  if (ret)
    {
      puts("pthread_attr_init failed");
      return 1;
    }
  ret = pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
  if (ret)
    {
      puts("pthread_attr_setdetachstate failed");
      return 1;
    }

  ret = pthread_create (&thread, &attr, signaler, NULL);
  if (ret)
    {
      puts("pthread_create failed");
      return 1;
    }

  return 0;
}

static int
do_test (void)
{
  int ret;
  struct timespec ts;

  ret = pthread_mutex_lock (&mut);
  if (ret) {
    puts("pthread_mutex_lock failed");
    return EXIT_FAILURE;
  }

  /* CLOCK_REALTIME accepted */
  if (clock_gettime (CLOCK_REALTIME, &ts) != 0)
    {
      perror("clock_gettime");
      return EXIT_FAILURE;
    }
  ts.tv_sec += 10;

  event = 0;
  ret = spawn_signaler();
  if (ret) return ret;
  while (!event)
    {
      ret = pthread_cond_clockwait (&cond, &mut, CLOCK_REALTIME, &ts);
      if (ret != 0)
        {
          puts("pthread_cond_clockwait failed");
          return EXIT_FAILURE;
        }
    }

  /* CLOCK_MONOTONIC accepted */
  if (clock_gettime (CLOCK_MONOTONIC, &ts) != 0)
    {
      perror("clock_gettime");
      return EXIT_FAILURE;
    }
  ts.tv_sec += 10;

  event = 0;
  ret = spawn_signaler();
  if (ret) return ret;
  while (!event)
    {
      ret = pthread_cond_clockwait (&cond, &mut, CLOCK_MONOTONIC, &ts);
      if (ret != 0)
        {
          puts("pthread_cond_clockwait failed");
          return EXIT_FAILURE;
        }
    }

  /* unsupported clock fails with ENOTSUP */
  ret = pthread_cond_clockwait (&cond, &mut, CLOCK_THREAD_CPUTIME_ID, &ts);
  if (ret != ENOTSUP)
    {
      puts("pthread_cond_clockwait did not fail as expected");
      return EXIT_FAILURE;
    }

  /* wait 100ms with CLOCK_MONOTONIC */
  if (clock_gettime (CLOCK_MONOTONIC, &ts) != 0)
    {
      puts("clock_gettime failed");
      return EXIT_FAILURE;
    }
  ts.tv_nsec += 100000000;
  if (ts.tv_nsec >= 1000000000)
    {
      ++ts.tv_sec;
      ts.tv_nsec -= 1000000000;
    }

  ret = pthread_cond_clockwait (&cond, &mut, CLOCK_MONOTONIC, &ts);
  if (ret != ETIMEDOUT)
    {
      puts("pthread_cond_clockwait did not time out");
      return EXIT_FAILURE;
    }

  struct timespec ts2;
  if (clock_gettime (CLOCK_MONOTONIC, &ts2) != 0)
    {
      perror("clock_gettime");
      return EXIT_FAILURE;
    }

  if (ts2.tv_sec < ts.tv_sec
      || (ts2.tv_sec == ts.tv_sec && ts2.tv_nsec < ts.tv_nsec))
    {
      puts ("timeout too short");
      return 1;
    }

  pthread_mutex_unlock (&mut);

  return EXIT_SUCCESS;
}

#define TEST_FUNCTION do_test ()
#include "../test-skeleton.c"
