/*
 * $Id: math_soft-float_addsf3.c,v 1.0 2023-07-18 07:54:24 clib2devs Exp $
*/

#ifndef _MATH_HEADERS_H
#include "math_headers.h"
#endif /* _MATH_HEADERS_H */

SFtype
__addsf3(SFtype a, SFtype b) {
    FP_DECL_EX;
    FP_DECL_S(A);
    FP_DECL_S(B);
    FP_DECL_S(R);
    SFtype r;

    FP_INIT_ROUNDMODE;
    FP_UNPACK_SEMIRAW_S(A, a);
    FP_UNPACK_SEMIRAW_S(B, b);
    FP_ADD_S(R, A, B);
    FP_PACK_SEMIRAW_S(r, R);
    FP_HANDLE_EXCEPTIONS;

    return r;
}