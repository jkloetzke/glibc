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
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define EXPECT_OK(exp)							    \
  do {									    \
    int ret = (exp);							    \
    if (ret < 0)							    \
      {									    \
	printf ("%s:%d: call failed: %s\n", __FILE__, __LINE__,		    \
		strerror(errno));					    \
	return 1;							    \
      }									    \
  } while (0)

#define SEM_CLOCKWAIT_EXPECT_FAIL(err, clk)				    \
  do {									    \
    errno = 0;								    \
    if (sem_clockwait (&s, (clk), &ts) >= 0)				    \
      {									    \
	printf ("%s:%d: sem_clockwait did not fail\n", __FILE__, __LINE__); \
	return 1;							    \
      }									    \
    if (errno != (err))							    \
      {									    \
	printf("%s:%d: sem_clockwait did not fail with %d but %d\n",	    \
	       __FILE__, __LINE__, (err), errno);			    \
	return 1;							    \
      }									    \
  } while (0)

static int
do_test (void)
{
  sem_t s;
  int val;

  if (sem_init (&s, 0, 2) != 0)
    {
      puts ("sem_init failed");
      return 1;
    }

  struct timespec ts = { 0, 0 };

  /* CLOCK_REALTIME accepted */
  EXPECT_OK (sem_clockwait (&s, CLOCK_REALTIME, &ts));

  /* CLOCK_MONOTONIC accepted */
  EXPECT_OK (sem_clockwait (&s, CLOCK_MONOTONIC, &ts));

  /* unsupported clock fails with ENOTSUP */
  SEM_CLOCKWAIT_EXPECT_FAIL (ENOTSUP, CLOCK_THREAD_CPUTIME_ID);

  /* semaphore value ought to be zero by now */
  EXPECT_OK (sem_getvalue (&s, &val));
  if (val != 0)
    {
      puts("semphore value not zero");
      return 1;
    }

  /* invalid tv_nsec */
  ts.tv_nsec = 1000000001;
  SEM_CLOCKWAIT_EXPECT_FAIL (EINVAL, CLOCK_MONOTONIC);
  ts.tv_nsec = -1;
  SEM_CLOCKWAIT_EXPECT_FAIL (EINVAL, CLOCK_MONOTONIC);

  /* ancient tv_sec */
  ts.tv_sec = -2;
  ts.tv_nsec = 0;
  SEM_CLOCKWAIT_EXPECT_FAIL (ETIMEDOUT, CLOCK_MONOTONIC);

  /* wait 100ms with CLOCK_MONOTONIC */
  EXPECT_OK (clock_gettime (CLOCK_MONOTONIC, &ts));
  ts.tv_nsec += 100000000;
  if (ts.tv_nsec >= 1000000000)
    {
      ++ts.tv_sec;
      ts.tv_nsec -= 1000000000;
    }

  SEM_CLOCKWAIT_EXPECT_FAIL (ETIMEDOUT, CLOCK_MONOTONIC);

  struct timespec ts2;
  EXPECT_OK (clock_gettime (CLOCK_MONOTONIC, &ts2));

  if (ts2.tv_sec < ts.tv_sec
      || (ts2.tv_sec == ts.tv_sec && ts2.tv_nsec < ts.tv_nsec))
    {
      puts ("timeout too short");
      return 1;
    }

  return 0;
}

#define TEST_FUNCTION do_test ()
#include "../test-skeleton.c"
