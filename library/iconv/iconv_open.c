/*
 * $Id: iconv_open.c,v 1.0 2021-03-09 12:04:25 apalmate Exp $
 *
 * :ts=4
 *
 * Portable ISO 'C' (1994) runtime library for the Amiga computer
 * Copyright (c) 2002-2015 by Olaf Barthel <obarthel (at) gmx.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   - Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   - Neither the name of Olaf Barthel nor the names of contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _STDLIB_HEADERS_H
#include "stdlib_headers.h"
#endif /* _STDLIB_HEADERS_H */

#include <iconv.h>
#include <sys/iconvnls.h>
#include "conv.h"

iconv_t
iconv_open(const char *to, const char *from)
{
    iconv_conversion_t *ic;

    if (to == NULL || from == NULL || *to == '\0' || *from == '\0')
    {
        __set_errno(EINVAL);
        return (iconv_t)-1;
    }

    if ((to = (const char *)_iconv_resolve_encoding_name(to)) == NULL)
    {
        __set_errno(EINVAL);
        return (iconv_t)-1;
    }

    if ((from = (const char *)_iconv_resolve_encoding_name(from)) == NULL)
    {
        free((void *)to);
        __set_errno(EINVAL);
        return (iconv_t)-1;
    }

    ic = (iconv_conversion_t *)malloc(sizeof(iconv_conversion_t));
    if (ic == NULL)
        return (iconv_t)-1;

    /* Select which conversion type to use */
    if (strcmp(from, to) == 0)
    {
        /* Use null conversion */
        ic->handlers = &_iconv_null_conversion_handlers;
        ic->data = ic->handlers->open(to, from);
    }
    else
    {
        /* Use UCS-based conversion */
        ic->handlers = &_iconv_ucs_conversion_handlers;
        ic->data = ic->handlers->open(to, from);
    }

    free((void *)to);
    free((void *)from);

    if (ic->data == NULL)
    {
        free((void *)ic);
        __set_errno(EINVAL);
        return (iconv_t)-1;
    }

    return (void *)ic;
}
