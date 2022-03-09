/*
 * $Id: unistd_access.c,v 1.10 2021-03-07 12:04:27 apalmate Exp $
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

int 
access(const char *path_name, int mode)
{
	struct name_translation_info path_name_nti;
	int result = ERROR;
	BPTR lock = ZERO;
	struct ExamineData *status = NULL;

	ENTER();

	SHOWSTRING(path_name);
	SHOWVALUE(mode);

	assert(path_name != NULL);

	if (__check_abort_enabled)
		__check_abort();

	if (path_name == NULL)
	{
		SHOWMSG("invalid path name");

		__set_errno(EFAULT);
		goto out;
	}

	if (mode < 0 || mode > (R_OK | W_OK | X_OK | F_OK))
	{
		SHOWMSG("invalid mode");

		__set_errno(EINVAL);
		goto out;
	}

	STRPTR actual_path_name = NULL;

	if (__global_clib2->__unix_path_semantics)
	{
		{
			if (path_name[0] == '\0')
			{
				SHOWMSG("no name given");

				__set_errno(ENOENT);
				goto out;
			}

			if (__translate_unix_to_amiga_path_name(&path_name, &path_name_nti) != 0)
				goto out;

			if (NOT path_name_nti.is_root)
				actual_path_name = (STRPTR)path_name;
		}

		if (actual_path_name != NULL)
		{
			D(("trying to get a lock on '%s'", actual_path_name));

			lock = Lock(actual_path_name, SHARED_LOCK);
			if (lock == ZERO)
			{
				__set_errno(__translate_access_io_error_to_errno(IoErr()));
				goto out;
			}
		}
	}
	else
	{
		D(("trying to get a lock on '%s'", path_name));

		lock = Lock((STRPTR)path_name, SHARED_LOCK);
		if (lock == ZERO)
		{
			__set_errno(__translate_access_io_error_to_errno(IoErr()));
			goto out;
		}
	}

	if ((mode != F_OK) && (mode & (R_OK | W_OK | X_OK)) != 0)
	{
		if (__global_clib2->__unix_path_semantics)
		{
			if (lock == ZERO)
			{
				memset(status, 0, sizeof(*status));

				/* This is a simulated directory which cannot be
				 * modified under program control.
				 * TODO - CHECK THIS UNDER OS4 AND EXAMINEOBJECT!
				 */
				status->Protection = EXDF_NO_WRITE;
				status->Type = FSO_TYPE_DIRECTORY;
			}
			else
			{
                status = ExamineObjectTags(EX_LockInput, lock, TAG_DONE);
				if (status == NULL)
				{
					SHOWMSG("couldn't examine");

					__set_errno(__translate_io_error_to_errno(IoErr()));
					goto out;
				}
			}
		}
		else
		{
			status = ExamineObjectTags(EX_LockInput, lock, TAG_DONE);
			if (status == DOSFALSE)
			{
				SHOWMSG("couldn't examine");

				__set_errno(__translate_io_error_to_errno(IoErr()));
				goto out;
			}
		}

		if (FLAG_IS_SET(mode, R_OK))
		{
			if (FLAG_IS_SET(status->Protection, EXDB_NO_READ))
			{
				SHOWMSG("not readable");

				__set_errno(EACCES);
				goto out;
			}
		}

		if (FLAG_IS_SET(mode, W_OK))
		{
			if (FLAG_IS_SET(status->Protection, EXDF_NO_WRITE) ||
				FLAG_IS_SET(status->Protection, EXDF_NO_DELETE))
			{
				SHOWMSG("not writable");

				__set_errno(EACCES);
				goto out;
			}
		}

		if (FLAG_IS_SET(mode, X_OK))
		{
			/* Note: 'X' means 'search' for directories, which is always
			 *       permitted on the Amiga.
			 */
			if ((!EXD_IS_DIRECTORY(status)) && FLAG_IS_SET(status->Protection, EXDF_NO_EXECUTE))
			{
				SHOWMSG("not executable");

				__set_errno(EACCES);
				goto out;
			}
		}
	}

	result = OK;

out:

	if (NULL != status)
	{
		FreeDosObject(DOS_EXAMINEDATA, status);
		status = NULL;
	}

	UnLock(lock);

	RETURN(result);
	return (result);
}
