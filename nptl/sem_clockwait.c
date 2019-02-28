/* sem_clockwait -- wait on a semaphore with timeout against specific clock.
   Copyright (C) 2003-2019 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include "sem_waitcommon.c"

/* This is in a separate file because because sem_clockwait is only
   provided if __USE_GNU is defined.  */
int
sem_clockwait (sem_t *sem, clockid_t clock_id, const struct timespec *abstime)
{
  if (__glibc_unlikely (abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000))
    {
      __set_errno (EINVAL);
      return -1;
    }

  if (__glibc_unlikely (clock_id != CLOCK_MONOTONIC
			&& clock_id != CLOCK_REALTIME))
    {
      __set_errno (ENOTSUP);
      return -1;
    }

  /* If we do not support waiting using CLOCK_MONOTONIC, return an error.  */
  if (clock_id == CLOCK_MONOTONIC
      && !futex_supports_exact_relative_timeouts())
    {
      __set_errno (ENOTSUP);
      return -1;
    }

  /* Check sem_wait.c for a more detailed explanation why it is required.  */
  __pthread_testcancel ();

  if (__new_sem_wait_fast ((struct new_sem *) sem, 0) == 0)
    return 0;
  else
    return __new_sem_wait_slow((struct new_sem *) sem, abstime,
			       clock_id == CLOCK_MONOTONIC);
}
