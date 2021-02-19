/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2010-2015 Intel Corporation
 * Copyright (c) 2007,2008 Kip Macy kmacy@freebsd.org
 * All rights reserved.
 * Derived from FreeBSD's bufring.h
 * Used as BSD-3 Licensed with permission from Kip Macy.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/queue.h>

#include <my_global.h>
#include <my_sys.h>
#include <mysqld_error.h>

#include "rte_ring.h"

/* return the size of memory occupied by a ring */
ssize_t rte_ring_get_memsize(unsigned count)
{
  ssize_t sz;

  /* count must be a power of 2 */
  if ((!POWEROF2(count)) || (count > RTE_RING_SZ_MASK )) {
    my_printf_error(ER_UNKNOWN_ERROR,
        "Requested size is invalid, must be power of 2, and "
        "do not exceed the size limit %u",
        MYF(0),
        RTE_RING_SZ_MASK);
    return -EINVAL;
  }

  sz = sizeof(struct rte_ring) + count * sizeof(void *);
  sz = RTE_ALIGN(sz, RTE_CACHE_LINE_SIZE);
  return sz;
}

int rte_ring_init(struct rte_ring *r, unsigned count, unsigned flags)
{
  /* compilation-time checks */
  RTE_BUILD_BUG_ON((sizeof(struct rte_ring) &
        RTE_CACHE_LINE_MASK) != 0);
  RTE_BUILD_BUG_ON((offsetof(struct rte_ring, cons) &
        RTE_CACHE_LINE_MASK) != 0);
  RTE_BUILD_BUG_ON((offsetof(struct rte_ring, prod) &
        RTE_CACHE_LINE_MASK) != 0);

  /* init the ring structure */
  memset(r, 0, sizeof(*r));
  r->flags = flags;
  r->prod.single = (flags & RING_F_SP_ENQ) ? __IS_SP : __IS_MP;
  r->cons.single = (flags & RING_F_SC_DEQ) ? __IS_SC : __IS_MC;

  if (flags & RING_F_EXACT_SZ) {
    r->size = rte_align32pow2(count + 1);
    r->mask = r->size - 1;
    r->capacity = count;
  } else {
    if ((!POWEROF2(count)) || (count > RTE_RING_SZ_MASK)) {
      my_printf_error(ER_UNKNOWN_ERROR,
          "Requested size is invalid, must be power of 2, and not exceed the size limit %u",
          MYF(0),
          RTE_RING_SZ_MASK);
      return -EINVAL;
    }
    r->size = count;
    r->mask = count - 1;
    r->capacity = r->mask;
  }
  r->prod.head = r->cons.head = 0;
  r->prod.tail = r->cons.tail = 0;

  return 0;
}

/* create the ring */
struct rte_ring* rte_ring_create(unsigned count, unsigned flags)
{
  struct rte_ring *r;
  ssize_t ring_size;
  const unsigned int requested_count = count;

  /* for an exact size ring, round up from count to a power of two */
  if (flags & RING_F_EXACT_SZ)
    count = rte_align32pow2(count + 1);

  ring_size = rte_ring_get_memsize(count);
  if (ring_size < 0) {
    return NULL;
  }

  r = (struct rte_ring*)my_malloc(ring_size, MYF(MY_WME));
  if (r == NULL) {
    my_printf_error(ER_UNKNOWN_ERROR,
        "Cannot reserve memory",
        MYF(0));
    return NULL;
  }

  if (rte_ring_init(r, requested_count, flags) != 0)
  {
    rte_ring_free(r);
    return NULL;
  }

  return r;
}

/* free the ring */
void rte_ring_free(struct rte_ring *r)
{
  if (r == NULL)
    return;

  my_free(r);

  return;
}

