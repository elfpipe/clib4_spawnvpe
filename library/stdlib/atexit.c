/*
 * $Id: stdlib_atexit.c,v 1.6 2006-01-08 12:04:25 clib4devs Exp $
*/

#ifndef _STDLIB_HEADERS_H
#include "stdlib_headers.h"
#endif /* _STDLIB_HEADERS_H */

#ifndef _STDLIB_MEMORY_H
#include "stdlib_memory.h"
#endif /* _STDLIB_MEMORY_H */

int
atexit(void (*function)(void)) {
    struct ExitTrapNode *etn;
    int result = ERROR;
    struct _clib4 *__clib4 = __CLIB4;

    assert(function != NULL);

    if (function == NULL) {
        __set_errno(EFAULT);
        goto out;
    }

    if (__clib4->atexit_blocked) {
        __set_errno(EACCES);
        goto out;
    }

    /* We always have room for 32 exit trap functions. */
    if (__clib4->num_exit_nodes_used < NUM_ENTRIES(__clib4->exit_node_table)) {
        etn = &__clib4->exit_node_table[__clib4->num_exit_nodes_used++];
    } else {
        /* More than 32 exit trap functions are called
         * for. Let's make room for another one.
         */
        etn = malloc(sizeof(*etn));
        if (etn == NULL) {
            __set_errno(ENOMEM);
            goto out;
        }
    }

    etn->etn_Function = function;

    if (__clib4->exit_trap_list.mlh_Head == NULL)
        NewList((struct List *) &__clib4->exit_trap_list);

    AddHead((struct List *) &__clib4->exit_trap_list, (struct Node *) etn);

    result = OK;

out:

    return (result);
}

void
__exit_trap_trigger(void) {
    struct ExitTrapNode *etn;
    struct _clib4 *__clib4 = __CLIB4;

    /* Disallow further additions to the atexit() list. */
    __clib4->atexit_blocked = TRUE;

    /* Remove the trap entries one at a time and process them.
     * Should one of the routines drop into exit(), processing
     * will continue with the next entry until eventually all
     * list items have been taken care of.
     */
    if (__clib4->exit_trap_list.mlh_Head != NULL) {
        while ((etn = (struct ExitTrapNode *) RemHead((struct List *) &__clib4->exit_trap_list)) != NULL)
            (*etn->etn_Function)();
    }
}
