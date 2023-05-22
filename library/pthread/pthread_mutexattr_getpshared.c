/*
  $Id: pthread_mutexattr_getpshared.c,v 1.00 2023-04-16 12:09:49 clib2devs Exp $
*/

#ifndef _UNISTD_HEADERS_H
#include "unistd_headers.h"
#endif /* _UNISTD_HEADERS_H */

int
pthread_mutexattr_getpshared(const pthread_mutexattr_t *attr, int *pshared) {
    *pshared = attr->pshared / 128U % 2;
    return 0;
}