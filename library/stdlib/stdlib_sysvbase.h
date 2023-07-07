/*
 * $Id: stdlib_sysvbase.h,v 1.0 2021-01-27 23:35:26 clib2devs Exp $
*/

#ifndef _STDLIB_SYSVBASE_H
#define _STDLIB_SYSVBASE_H

#ifndef __NOLIBBASE__
#define __NOLIBBASE__
#endif /* __NOLIBBASE__ */

#ifndef __NOGLOBALIFACE__
#define __NOGLOBALIFACE__
#endif /* __NOGLOBALIFACE__ */

#include "proto/sysvipc.h"
#include "libraries/sysvipc.h"

#ifndef _MACROS_H
#include "macros.h"
#endif /* _MACROS_H */

#include "shared_library/clib2.h"

#define DECLARE_SYSVYBASE() \
	struct Library   UNUSED	*SysVBase    = res->SysVBase; \
	struct SYSVIFace 		*ISysVIPC	 = res->ISysVIPC

#endif /* _STDLIB_SYSVBASE_H */
