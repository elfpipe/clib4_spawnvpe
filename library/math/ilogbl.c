/*
 * $Id: math_ilogbl.c,v 1.0 2022-03-11 11:48:23 clib2devs Exp $
*/

#ifndef _MATH_HEADERS_H
#include "math_headers.h"
#endif /* _MATH_HEADERS_H */

int
ilogbl(long double x)
{
    return ilogb(x);
}
