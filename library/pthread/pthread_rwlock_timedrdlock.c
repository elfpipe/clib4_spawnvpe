/*
  $Id: pthread_rwlock_timedrdlock.c,v 1.00 2022-07-18 12:09:49 clib4devs Exp $

  Copyright (C) 2014 Szilard Biro
  Copyright (C) 2018 Harry Sintonen
  Copyright (C) 2019 Stefan "Bebbo" Franke - AmigaOS 3 port

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _TIME_HEADERS_H
#include "time_headers.h"
#endif /* _TIME_HEADERS_H */

#ifndef _STDIO_HEADERS_H
#include "stdio_headers.h"
#endif /* _STDIO_HEADERS_H */

#ifndef _UNISTD_HEADERS_H
#include "unistd_headers.h"
#endif /* _UNISTD_HEADERS_H */

#include "common.h"
#include "pthread.h"

int
pthread_rwlock_timedrdlock(pthread_rwlock_t *lock, const struct timespec *abstime) {
    int result;

    if (lock == NULL)
        return EINVAL;

    if (abstime == NULL)
        return pthread_rwlock_rdlock(lock);
    else if (abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000)
        return EINVAL;

    pthread_testcancel();

    result = pthread_rwlock_tryrdlock(lock);
    if (result != EBUSY)
        return result;

    return _pthread_obtain_sema_timed((struct SignalSemaphore *)lock->semaphore, abstime, SM_SHARED);
}