/*
 * $Id: debug_kgetchar2.c,v 1.3 2006-01-08 12:04:22 clib2devs Exp $
*/

#include "debug_headers.h"

/****************************************************************************/

LONG
KGetChar(VOID)
{
	LONG result;

	result = kgetc();

	return(result);
}
