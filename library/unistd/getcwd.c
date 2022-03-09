/*
 * $Id: unistd_getcwd.c,v 1.10 2006-01-08 12:04:27 obarthel Exp $
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

#ifndef _UNISTD_HEADERS_H
#include "unistd_headers.h"
#endif /* _UNISTD_HEADERS_H */

#ifndef _STDLIB_HEADERS_H
#include "stdlib_headers.h"
#endif /* _STDLIB_HEADERS_H */

#ifndef _STDLIB_MEMORY_H
#include "stdlib_memory.h"
#endif /* _STDLIB_MEMORY_H */

char *
getcwd(char *buffer, size_t buffer_size) {
#if defined(UNIX_PATH_SEMANTICS)
    struct name_translation_info buffer_nti;
#endif /* UNIX_PATH_SEMANTICS */
    char *result = NULL;
    BPTR dir_lock = ZERO;

    ENTER();

    SHOWPOINTER(buffer);
    SHOWVALUE(buffer_size);

    assert(buffer != NULL);
    assert((int) buffer_size > 0);

    if (__check_abort_enabled)
        __check_abort();

    if (buffer_size == 0 || buffer == NULL) {
        /* As an extension to the POSIX.1-2001 standard, glibc's getcwd()
         * allocates the buffer dynamically using malloc(3) if buf is NULL.
         * In this case, the allocated buffer has the length size unless
         * size is zero, when buf is allocated as big as necessary.  The
         * caller should free(3) the returned buffer.
        */
        buffer = malloc(PATH_MAX);
        buffer_size = PATH_MAX;
        if (buffer == NULL) {
            SHOWMSG("not enough memory for result buffer");

            __set_errno(ENOMEM);
            goto out;
        }
    }

    if (buffer_size > PATH_MAX) {
        SHOWMSG("buffer size too large");

        __set_errno(ENAMETOOLONG);
        goto out;
    }

    dir_lock = Lock("", SHARED_LOCK);
    if (dir_lock == ZERO) {
        SHOWMSG("could not get a lock on the current directory");

        __set_errno(__translate_io_error_to_errno(IoErr()));
        goto out;
    }

#if defined(UNIX_PATH_SEMANTICS)
    if (__global_clib2->__unix_path_semantics)
    {
        if (__current_path_name[0] != '\0')
        {
            if (buffer_size < strlen(__current_path_name) + 1)
            {
                SHOWMSG("buffer is too small");

                __set_errno(ERANGE);
                goto out;
            }

            strcpy(buffer,__current_path_name);

            D(("returning absolute path name '%s'",buffer));

            result = buffer;
        }
    }
#endif /* UNIX_PATH_SEMANTICS */

    if (result == NULL) {
        LONG status;

        status = NameFromLock(dir_lock, buffer, (LONG) buffer_size);
        if (status == DOSFALSE) {
            int errno_code;
            LONG io_error;

            SHOWMSG("could not get name from lock");

            io_error = IoErr();

            /* Was the buffer too small? */
            if (io_error == ERROR_LINE_TOO_LONG)
                errno_code = ERANGE;
            else
                errno_code = __translate_io_error_to_errno(io_error);

            __set_errno(errno_code);
            goto out;
        }

#if defined(UNIX_PATH_SEMANTICS)
        if(__global_clib2->__unix_path_semantics)
        {
            const char * path_name = buffer;

            if(__translate_amiga_to_unix_path_name(&path_name,&buffer_nti) != 0)
                goto out;

            if(buffer_size < strlen(path_name) + 1)
            {
                SHOWMSG("buffer is too small");

                __set_errno(ERANGE);
                goto out;
            }

            strcpy(buffer,path_name);
        }
#endif /* UNIX_PATH_SEMANTICS */
    }

    SHOWSTRING(buffer);

    result = buffer;

out:

    UnLock(dir_lock);

    RETURN(result);
    return (result);
}
