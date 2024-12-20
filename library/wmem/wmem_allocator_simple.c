/* wmem_allocator_simple.c
 * Wireshark Memory Manager Simple Allocator
 * Copyright 2012, Evan Huus <eapache@gmail.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _STDLIB_HEADERS_H
#include "stdlib_headers.h"
#endif /* _STDLIB_HEADERS_H */

#ifndef _STDLIB_MEMORY_H
#include "stdlib_memory.h"
#endif /* _STDLIB_MEMORY_H */

#include "wmem_core.h"
#include "wmem_allocator.h"
#include "wmem_allocator_simple.h"

#define DEFAULT_ALLOCS 8192

typedef struct _wmem_simple_allocator_t {
    int size;
    int count;
    void **ptrs;
} wmem_simple_allocator_t;

static void *
wmem_simple_alloc(void *private_data, const size_t size) {
    wmem_simple_allocator_t *allocator;

    allocator = (wmem_simple_allocator_t *) private_data;

    if (__clib4_unlikely(allocator->count == allocator->size)) {
        allocator->size *= 2;
        allocator->ptrs = (void **) wmem_realloc(NULL, allocator->ptrs,
                                                 sizeof(void *) * allocator->size);
    }

    return allocator->ptrs[allocator->count++] = wmem_alloc(NULL, size);
}

static void
wmem_simple_free(void *private_data, void *ptr) {
    int i;
    wmem_simple_allocator_t *allocator;

    allocator = (wmem_simple_allocator_t *) private_data;

    wmem_free(NULL, ptr);
    allocator->count--;

    for (i = allocator->count; i >= 0; i--) {
        if (ptr == allocator->ptrs[i]) {
            if (i < allocator->count) {
                allocator->ptrs[i] = allocator->ptrs[allocator->count];
            }
            return;
        }
    }
}

static void *
wmem_simple_realloc(void *private_data, void *ptr, const size_t size) {
    int i;
    wmem_simple_allocator_t *allocator;

    allocator = (wmem_simple_allocator_t *) private_data;

    for (i = allocator->count - 1; i >= 0; i--) {
        if (ptr == allocator->ptrs[i]) {
            return allocator->ptrs[i] = wmem_realloc(NULL, allocator->ptrs[i], size);
        }
    }

    /* not reached */
    return NULL;
}

static void
wmem_simple_free_all(void *private_data) {
    wmem_simple_allocator_t *allocator;
    int i;

    allocator = (wmem_simple_allocator_t *) private_data;

    for (i = 0; i < allocator->count; i++) {
        wmem_free(NULL, allocator->ptrs[i]);
    }
    allocator->count = 0;
}

static void
wmem_simple_gc(void *private_data) {
    (void) (private_data);
    /* In this simple allocator, there is nothing to garbage-collect */
}

static void
wmem_simple_allocator_cleanup(void *private_data) {
    wmem_simple_allocator_t *allocator;

    allocator = (wmem_simple_allocator_t *) private_data;

    wmem_free(NULL, allocator->ptrs);
    wmem_free(NULL, allocator);
}

void
wmem_simple_allocator_init(wmem_allocator_t *allocator) {
    wmem_simple_allocator_t *simple_allocator;

    simple_allocator = wmem_new(NULL, wmem_simple_allocator_t);

    allocator->walloc = &wmem_simple_alloc;
    allocator->wrealloc = &wmem_simple_realloc;
    allocator->wfree = &wmem_simple_free;

    allocator->free_all = &wmem_simple_free_all;
    allocator->gc = &wmem_simple_gc;
    allocator->cleanup = &wmem_simple_allocator_cleanup;

    allocator->private_data = (void *) simple_allocator;

    simple_allocator->count = 0;
    simple_allocator->size = DEFAULT_ALLOCS;
    simple_allocator->ptrs = wmem_alloc_array(NULL, void*, DEFAULT_ALLOCS);
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
