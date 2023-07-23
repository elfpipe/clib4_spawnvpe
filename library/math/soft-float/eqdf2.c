/*
 * $Id: math_soft-float_eqdf2.c,v 1.0 2023-07-18 07:54:24 clib2devs Exp $
*/

#ifndef _MATH_HEADERS_H
#include "math_headers.h"
#endif /* _MATH_HEADERS_H */

CMPtype
__eqdf2(DFtype a, DFtype b) {
    FP_DECL_EX;
    FP_DECL_D(A);
    FP_DECL_D(B);
    CMPtype r;

    FP_INIT_EXCEPTIONS;
    FP_UNPACK_RAW_D(A, a);
    FP_UNPACK_RAW_D(B, b);
    FP_CMP_EQ_D(r, A, B, 1);
    FP_HANDLE_EXCEPTIONS;

    return r;
}

CMPtype
__nedf2(DFtype a, DFtype b) {
    return __eqdf2(a, b);
}